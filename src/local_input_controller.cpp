#include "local_input_controller.hpp"

#include "game_command.hpp"
#include "game_constants.hpp"

#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace tinyv1
{
namespace
{

bool first_selected_unit_summary(const GameSimulation &sim,
                                 const LocalPlayerState &local,
                                 UnitSummary &r_summary)
{
  for (UnitId unit_id : local.selected_unit_ids)
  {
    if (sim.get_unit_summary(unit_id, r_summary))
    {
      return true;
    }
  }
  return false;
}

} // namespace

bool LocalInputController::handle_event(const Ref<InputEvent> &event,
                                        const Vector2 &mouse_position,
                                        GameSimulation &sim,
                                        LocalPlayerState &local)
{
  Ref<InputEventMouseButton> mouse_event = event;
  if (mouse_event.is_valid())
  {
    if (mouse_event->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT)
    {
      if (mouse_event->is_pressed())
      {
        if (local.is_placing_barracks)
        {
          GameCommand command;
          command.type = GameCommandType::PLACE_BARRACKS;
          command.owner = PLAYER;
          command.position = mouse_position;
          UnitSummary builder;
          command.builder_unit_id = first_selected_unit_summary(sim, local, builder) ? builder.id : -1;
          const GameCommandResult result = sim.apply_command(command);
          const int32_t building_id = result.placed_building_id;
          if (sim.get_essence(PLAYER) < BARRACKS_COST || building_id != -1)
          {
            local.is_placing_barracks = false;
            if (building_id != -1)
            {
              local.clear_selection();
              local.selected_building_id = building_id;
            }
          }
          return true;
        }

        const UnitId unit_id_at_mouse = sim.find_player_unit_id_at(mouse_position);
        UnitSummary unit_at_mouse;
        const bool has_unit_at_mouse = sim.get_unit_summary(unit_id_at_mouse, unit_at_mouse);
        if (mouse_event->is_double_click() && has_unit_at_mouse)
        {
          local.is_drag_selecting = false;
          local.drag_has_unit_type_filter = false;
          local.set_selection(sim.select_all_units_of_type(PLAYER, unit_at_mouse.type));
          return true;
        }

        local.is_drag_selecting = true;
        local.drag_has_unit_type_filter = has_unit_at_mouse;
        if (has_unit_at_mouse)
        {
          local.drag_unit_type_filter = unit_at_mouse.type;
        }
        local.drag_start = mouse_position;
        local.drag_current = mouse_position;
      }
      else if (local.is_drag_selecting)
      {
        local.is_drag_selecting = false;
        const Vector2 top_left(std::min(local.drag_start.x, local.drag_current.x),
                               std::min(local.drag_start.y, local.drag_current.y));
        const Vector2 size(std::abs(local.drag_current.x - local.drag_start.x),
                           std::abs(local.drag_current.y - local.drag_start.y));
        if (size.length() >= 8.0f)
        {
          local.set_selection(sim.select_units_in_rect(PLAYER, Rect2(top_left, size),
                                                       local.drag_has_unit_type_filter,
                                                       local.drag_unit_type_filter));
        }
        else
        {
          local.set_selection(sim.select_at(PLAYER, mouse_position));
        }
        local.drag_has_unit_type_filter = false;
      }
    }
    else if (mouse_event->get_button_index() == MouseButton::MOUSE_BUTTON_RIGHT &&
             mouse_event->is_pressed())
    {
      local.is_placing_barracks = false;
      GameCommand command;
      command.type = GameCommandType::COMMAND_SELECTED_TO;
      command.owner = PLAYER;
      command.position = mouse_position;
      command.selected_unit_ids = local.selected_unit_ids;
      command.selected_base_owner = local.selected_base_owner;
      command.selected_building_id = local.selected_building_id;
      const GameCommandResult result = sim.apply_command(command);
      if (result.feedback.has_marker)
      {
        local.command_marker_position = result.feedback.marker_position;
        local.command_marker_timer = 0.45f;
      }
    }
    return true;
  }

  Ref<InputEventMouseMotion> motion_event = event;
  if (motion_event.is_valid() && local.is_drag_selecting)
  {
    local.drag_current = mouse_position;
    return true;
  }
  if (motion_event.is_valid() && local.is_placing_barracks)
  {
    return true;
  }

  Ref<InputEventKey> key_event = event;
  if (!key_event.is_valid() || !key_event->is_pressed() || key_event->is_echo())
  {
    return false;
  }

  if (key_event->get_keycode() == Key::KEY_DELETE)
  {
    if (local.is_placing_barracks)
    {
      local.is_placing_barracks = false;
    }
    else
    {
      GameCommand command;
      command.type = GameCommandType::DELETE_BARRACKS;
      command.owner = PLAYER;
      command.selected_building_id = local.selected_building_id;
      sim.apply_command(command);
      local.clear_selection();
    }
    return true;
  }
  if (key_event->get_keycode() == Key::KEY_F && local.selected_building_id != -1)
  {
    GameCommand command;
    command.type = GameCommandType::TRAIN_UNIT;
    command.owner = PLAYER;
    command.unit_type = UnitType::FIGHTER;
    command.selected_building_id = local.selected_building_id;
    sim.apply_command(command);
    return true;
  }
  if (key_event->get_keycode() == Key::KEY_W && local.selected_base_owner == PLAYER)
  {
    GameCommand command;
    command.type = GameCommandType::TRAIN_UNIT;
    command.owner = PLAYER;
    command.unit_type = UnitType::WORKER;
    sim.apply_command(command);
    return true;
  }
  if (key_event->get_keycode() == Key::KEY_B && !local.selected_unit_ids.empty() &&
      sim.can_start_barracks_placement(PLAYER, local.selected_unit_ids.front()))
  {
    local.is_placing_barracks = true;
    return true;
  }

  return false;
}

bool LocalInputController::perform_action_button(int32_t index,
                                                 GameSimulation &sim,
                                                 LocalPlayerState &local)
{
  if (local.selected_base_owner == PLAYER && index == 0)
  {
    GameCommand command;
    command.type = GameCommandType::TRAIN_UNIT;
    command.owner = PLAYER;
    command.unit_type = UnitType::WORKER;
    sim.apply_command(command);
    return true;
  }

  if (local.selected_building_id != -1 && index == 0)
  {
    GameCommand command;
    command.type = GameCommandType::TRAIN_UNIT;
    command.owner = PLAYER;
    command.unit_type = UnitType::FIGHTER;
    command.selected_building_id = local.selected_building_id;
    sim.apply_command(command);
    return true;
  }

  UnitSummary unit;
  if (first_selected_unit_summary(sim, local, unit) && unit.type == UnitType::WORKER && index == 0 &&
      sim.can_start_barracks_placement(PLAYER, unit.id))
  {
    local.is_placing_barracks = true;
    return true;
  }

  return false;
}

} // namespace tinyv1

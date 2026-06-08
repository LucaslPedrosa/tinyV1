#include "tiny_game.hpp"

#include "hud_presenter.hpp"
#include "local_input_controller.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>

using namespace godot;

namespace tinyv1
{

void TinyGame::_bind_methods()
{
  ClassDB::bind_method(D_METHOD("get_version_number"), &TinyGame::get_version_number);
  ClassDB::bind_method(D_METHOD("get_status_text"), &TinyGame::get_status_text);
  ClassDB::bind_method(D_METHOD("get_resource_text"), &TinyGame::get_resource_text);
  ClassDB::bind_method(D_METHOD("get_selected_name"), &TinyGame::get_selected_name);
  ClassDB::bind_method(D_METHOD("get_selected_actions_text"), &TinyGame::get_selected_actions_text);
  ClassDB::bind_method(D_METHOD("get_selected_details_text"), &TinyGame::get_selected_details_text);
  ClassDB::bind_method(D_METHOD("get_selected_portrait_color"),
                       &TinyGame::get_selected_portrait_color);
  ClassDB::bind_method(D_METHOD("get_selected_portrait_path"),
                       &TinyGame::get_selected_portrait_path);
  ClassDB::bind_method(D_METHOD("get_action_button_text", "index"),
                       &TinyGame::get_action_button_text);
  ClassDB::bind_method(D_METHOD("get_action_button_icon_path", "index"),
                       &TinyGame::get_action_button_icon_path);
  ClassDB::bind_method(D_METHOD("is_action_button_enabled", "index"),
                        &TinyGame::is_action_button_enabled);
  ClassDB::bind_method(D_METHOD("get_hud_snapshot"), &TinyGame::get_hud_snapshot);
  ClassDB::bind_method(D_METHOD("perform_action_button", "index"),
                        &TinyGame::perform_action_button);
}

void TinyGame::_ready()
{
  if (Engine::get_singleton()->is_editor_hint())
  {
    return;
  }

  set_process(true);
  set_process_unhandled_input(true);
  set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
  renderer.load_textures();
  reset_match();
  UtilityFunctions::print("tinyV1 native prototype ready");
}

void TinyGame::_process(double p_delta)
{
  animation_time += static_cast<float>(p_delta);
  if (!sim.get_winner_text().is_empty())
  {
    return;
  }

  sim.update_units(p_delta);
  local.command_marker_timer =
      std::max(0.0f, local.command_marker_timer - static_cast<float>(p_delta));

  sim.update_training(p_delta);
  for (const GameCommand &command : bot.update(sim, p_delta))
  {
    sim.apply_command(command);
  }
  sim.remove_dead_units();
  sim.remove_destroyed_barracks();
  sim.check_win_condition();
  queue_redraw();
}

void TinyGame::_draw()
{
  renderer.draw(*this, sim, local, animation_time);
}

void TinyGame::_unhandled_input(const Ref<InputEvent> &p_event)
{
  if (!sim.get_winner_text().is_empty())
  {
    Ref<InputEventKey> key_event = p_event;
    if (key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo())
    {
      reset_match();
    }
    return;
  }

  if (LocalInputController::handle_event(p_event, get_global_mouse_position(), sim, local))
  {
    queue_redraw();
  }
}

int32_t TinyGame::get_version_number() const
{
  return 1;
}

String TinyGame::get_status_text() const
{
  return HudPresenter::get_status_text(sim, local);
}

String TinyGame::get_resource_text() const
{
  return HudPresenter::get_resource_text(sim);
}

String TinyGame::get_selected_name() const
{
  return HudPresenter::get_selected_name(sim, local);
}

String TinyGame::get_selected_actions_text() const
{
  return HudPresenter::get_selected_actions_text(sim, local);
}

String TinyGame::get_selected_details_text() const
{
  return HudPresenter::get_selected_details_text(sim, local);
}

Color TinyGame::get_selected_portrait_color() const
{
  return HudPresenter::get_selected_portrait_color(sim, local);
}

String TinyGame::get_selected_portrait_path() const
{
  return HudPresenter::get_selected_portrait_path(sim, local);
}

String TinyGame::get_action_button_text(int32_t p_index) const
{
  return HudPresenter::get_action_button_text(sim, local, p_index);
}

String TinyGame::get_action_button_icon_path(int32_t p_index) const
{
  return HudPresenter::get_action_button_icon_path(sim, local, p_index);
}

bool TinyGame::is_action_button_enabled(int32_t p_index) const
{
  return HudPresenter::is_action_button_enabled(sim, local, p_index);
}

Dictionary TinyGame::get_hud_snapshot() const
{
  Dictionary snapshot;
  snapshot["resource_text"] = HudPresenter::get_resource_text(sim);
  snapshot["selected_actions_text"] = HudPresenter::get_selected_actions_text(sim, local);
  snapshot["selected_name"] = HudPresenter::get_selected_name(sim, local);
  snapshot["selected_details_text"] = HudPresenter::get_selected_details_text(sim, local);
  snapshot["selected_portrait_path"] = HudPresenter::get_selected_portrait_path(sim, local);

  for (int32_t index = 0; index < 2; ++index)
  {
    const String suffix = String::num_int64(index);
    snapshot[String("action_button_") + suffix + "_text"] =
        HudPresenter::get_action_button_text(sim, local, index);
    snapshot[String("action_button_") + suffix + "_icon_path"] =
        HudPresenter::get_action_button_icon_path(sim, local, index);
    snapshot[String("action_button_") + suffix + "_enabled"] =
        HudPresenter::is_action_button_enabled(sim, local, index);
  }

  return snapshot;
}

void TinyGame::perform_action_button(int32_t p_index)
{
  if (LocalInputController::perform_action_button(p_index, sim, local))
  {
    queue_redraw();
  }
}

void TinyGame::reset_match()
{
  sim.reset_match();
  bot.reset();
  local.reset();
}

} // namespace tinyv1

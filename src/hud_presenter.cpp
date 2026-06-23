#include "hud_presenter.hpp"

#include "definitions/building_definitions.hpp"
#include "definitions/unit_definitions.hpp"
#include "game_constants.hpp"
#include "rendering/asset_contracts.hpp"

using namespace godot;

namespace tinyv1
{
namespace
{

bool first_selected_unit_summary(const GameSimulation &sim, const LocalPlayerState &local,
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

bool selected_building_summary(const GameSimulation &sim, const LocalPlayerState &local,
                                BuildingSummary &r_summary)
{
  return sim.get_building_summary(local.selected_building_id, r_summary);
}

void get_actions(const GameSimulation &sim, const LocalPlayerState &local,
                 std::vector<AvailableAction> &r_actions)
{
  sim.get_available_actions(PLAYER, local.selected_unit_ids, local.selected_building_id, r_actions);
}

bool get_action_at(const GameSimulation &sim, const LocalPlayerState &local, int32_t p_index,
                   AvailableAction &r_action)
{
  std::vector<AvailableAction> actions;
  get_actions(sim, local, actions);
  if (p_index < 0 || p_index >= static_cast<int32_t>(actions.size()))
  {
    return false;
  }
  r_action = actions[p_index];
  return true;
}

String action_label(const AvailableAction &p_action)
{
  if (p_action.type == AvailableActionType::BUILD_BUILDING)
  {
    const BuildingDefinition &definition = get_building_definition(p_action.building_type);
    return "Build " + String(definition.display_name);
  }
  if (p_action.type == AvailableActionType::TRAIN_UNIT)
  {
    const UnitDefinition &definition = get_unit_definition(p_action.unit_type);
    return "Train " + String(definition.display_name);
  }
  return "Delete";
}

String action_icon_path(const AvailableAction &p_action)
{
  if (p_action.type == AvailableActionType::BUILD_BUILDING)
  {
    return asset_contracts::building_face(get_building_definition(p_action.building_type).asset_name);
  }
  if (p_action.type == AvailableActionType::TRAIN_UNIT)
  {
    return asset_contracts::humanoid_face(get_unit_definition(p_action.unit_type).humanoid_asset_folder);
  }
  return "";
}

String format_cost(const ResourceCost &p_cost)
{
  String text;
  if (p_cost.food > 0)
  {
    text += String::num_int64(p_cost.food) + " Food";
  }
  if (p_cost.wood > 0)
  {
    text += (text.is_empty() ? "" : ", ") + String::num_int64(p_cost.wood) + " Wood";
  }
  if (p_cost.gold > 0)
  {
    text += (text.is_empty() ? "" : ", ") + String::num_int64(p_cost.gold) + " Gold";
  }
  if (p_cost.favor > 0)
  {
    text += (text.is_empty() ? "" : ", ") + String::num_int64(p_cost.favor) + " Favor";
  }
  return text.is_empty() ? "Free" : text;
}

String format_queue_count(int32_t p_queue_count)
{
  return String::num_int64(p_queue_count) + "/" + String::num_int64(MAX_TRAIN_QUEUE);
}

} // namespace

String HudPresenter::get_status_text(const GameSimulation &sim, const LocalPlayerState &local)
{
  String selected_text =
      static_cast<int32_t>(local.selected_unit_ids.size()) > 0
          ? " selected units: " +
                String::num_int64(static_cast<int32_t>(local.selected_unit_ids.size()))
          : "";
  if (local.selected_building_id != -1)
  {
    selected_text += " | building selected";
  }
  else if (local.is_placing_building)
  {
    selected_text += " | placing building: left click place, right click cancel";
  }
  if (!sim.get_winner_text().is_empty())
  {
    return sim.get_winner_text() + String(" | press any key to restart");
  }
  return "Gold " + String::num_int64(sim.get_gold(PLAYER)) +
         " | left click select | right click command" + selected_text;
}

String HudPresenter::get_resource_text(const GameSimulation &sim)
{
  const ResourceWallet &resources = sim.get_resources(PLAYER);
  return "Food: " + String::num_int64(resources.food) +
         "\nWood: " + String::num_int64(resources.wood) +
         "\nGold: " + String::num_int64(resources.gold) +
         "\nFavor: " + String::num_int64(resources.favor);
}

String HudPresenter::get_selected_name(const GameSimulation &sim, const LocalPlayerState &local)
{
  const int32_t count = static_cast<int32_t>(local.selected_unit_ids.size());
  if (count > 1)
  {
    return "Unit Group (" + String::num_int64(count) + ")";
  }

  UnitSummary unit;
  if (first_selected_unit_summary(sim, local, unit))
  {
    return get_unit_definition(unit.type).display_name;
  }

  BuildingSummary selected_building;
  if (selected_building_summary(sim, local, selected_building))
  {
    const BuildingDefinition &definition = get_building_definition(selected_building.type);
    if (!selected_building.completed)
    {
      return String(definition.display_name) + " Site";
    }
    return definition.display_name;
  }

  return "No Selection";
}

String HudPresenter::get_selected_actions_text(const GameSimulation &sim,
                                               const LocalPlayerState &local)
{
  UnitSummary unit;
  if (static_cast<int32_t>(local.selected_unit_ids.size()) > 0)
  {
    std::vector<AvailableAction> actions;
    get_actions(sim, local, actions);
    if (!actions.empty())
    {
      String text = "Right Click: contextual command";
      for (const AvailableAction &action : actions)
      {
        text += "\n" + action_label(action) + " (" +
                format_cost(action.cost) + ")";
      }
      return text;
    }
    return "Right Click: Attack-move\nRight Click Building: Attack";
  }

  BuildingSummary selected_building;
  if (selected_building_summary(sim, local, selected_building))
  {
    if (!selected_building.completed)
    {
      return "Under construction\nDel: Cancel refund";
    }
    if (selected_building.training)
    {
      return "Training " + String(get_unit_definition(selected_building.active_unit).display_name) +
             ": " +
             String::num_int64(static_cast<int64_t>(
                 (1.0f - selected_building.train_timer / selected_building.train_duration) *
                 100.0f)) +
             "%\nQueue: " + format_queue_count(selected_building.queue_count);
    }
    std::vector<AvailableAction> actions;
    get_actions(sim, local, actions);
    String text = "Right Click: Set rally action\nQueue: " +
                  format_queue_count(selected_building.queue_count);
    for (const AvailableAction &action : actions)
    {
      text += "\n" + action_label(action) + " (" + format_cost(action.cost) + ")";
    }
    return text;
  }

  return "Select a unit or building.";
}

String HudPresenter::get_selected_details_text(const GameSimulation &sim,
                                               const LocalPlayerState &local)
{
  const int32_t count = static_cast<int32_t>(local.selected_unit_ids.size());
  if (count > 1)
  {
    int32_t workers = 0;
    int32_t fighters = 0;
    for (UnitId unit_id : local.selected_unit_ids)
    {
      UnitSummary unit;
      if (!sim.get_unit_summary(unit_id, unit))
      {
        continue;
      }
      if (unit.type == UnitType::WORKER)
      {
        workers++;
      }
      else
      {
        fighters++;
      }
    }
    return "Selected group\nWorkers: " + String::num_int64(workers) +
           "\nFighters: " + String::num_int64(fighters);
  }

  UnitSummary unit;
  if (first_selected_unit_summary(sim, local, unit))
  {
    String role = get_unit_definition(unit.type).description;
    return "HP: " + String::num_int64(static_cast<int64_t>(unit.hp)) + "\n" + role;
  }

  BuildingSummary selected_building;
  if (selected_building_summary(sim, local, selected_building))
  {
    if (!selected_building.completed)
    {
      return "Progress: " +
             String::num_int64(static_cast<int64_t>(selected_building.build_progress * 100.0f)) +
             "%\nNeeds worker construction.";
    }
    if (selected_building.training)
    {
      return "HP: " + String::num_int64(static_cast<int64_t>(selected_building.hp)) +
             "\nTraining " + String(get_unit_definition(selected_building.active_unit).display_name) +
             "\nQueue: " + format_queue_count(selected_building.queue_count);
    }
    const BuildingDefinition &definition = get_building_definition(selected_building.type);
    return "HP: " + String::num_int64(static_cast<int64_t>(selected_building.hp)) + "\n" +
           definition.description;
  }

  return "No unit selected.\nSelect workers, fighters, or your base.";
}

Color HudPresenter::get_selected_portrait_color(const GameSimulation &sim,
                                                const LocalPlayerState &local)
{
  UnitSummary unit;
  if (static_cast<int32_t>(local.selected_unit_ids.size()) > 1)
  {
    return Color(0.35f, 0.55f, 0.95f);
  }
  if (first_selected_unit_summary(sim, local, unit))
  {
    return unit.type == UnitType::WORKER ? Color(0.55f, 0.78f, 1.0f) : Color(0.25f, 0.55f, 1.0f);
  }
  if (local.selected_building_id != -1)
  {
    return Color(0.20f, 0.45f, 0.95f);
  }
  return Color(0.18f, 0.18f, 0.20f);
}

String HudPresenter::get_selected_portrait_path(const GameSimulation &sim,
                                                const LocalPlayerState &local)
{
  UnitSummary unit;
  if (first_selected_unit_summary(sim, local, unit))
  {
    return asset_contracts::humanoid_face(get_unit_definition(unit.type).humanoid_asset_folder);
  }

  if (local.selected_building_id != -1)
  {
    BuildingSummary building;
    if (selected_building_summary(sim, local, building))
    {
      return asset_contracts::building_face(get_building_definition(building.type).asset_name);
    }
  }

  return "";
}

String HudPresenter::get_action_button_text(const GameSimulation &sim,
                                            const LocalPlayerState &local, int32_t index)
{
  AvailableAction action;
  if (!get_action_at(sim, local, index, action))
  {
    return "";
  }
  return action_label(action) + "\n" + format_cost(action.cost);
}

String HudPresenter::get_action_button_icon_path(const GameSimulation &sim,
                                                 const LocalPlayerState &local, int32_t index)
{
  AvailableAction action;
  return get_action_at(sim, local, index, action) ? action_icon_path(action) : "";
}

bool HudPresenter::is_action_button_enabled(const GameSimulation &sim,
                                            const LocalPlayerState &local, int32_t index)
{
  AvailableAction action;
  return get_action_at(sim, local, index, action) && action.enabled;
}

} // namespace tinyv1

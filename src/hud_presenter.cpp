#include "hud_presenter.hpp"

#include "game_constants.hpp"
#include "rendering/asset_contracts.hpp"

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

bool selected_building_summary(const GameSimulation &sim,
                               const LocalPlayerState &local,
                               BuildingSummary &r_summary)
{
  return sim.get_building_summary(local.selected_building_id, r_summary);
}

const char *humanoid_asset_folder(UnitType p_type)
{
  return p_type == UnitType::WORKER ? "worker" : "hoplite";
}

} // namespace

String HudPresenter::get_status_text(const GameSimulation &sim, const LocalPlayerState &local)
{
  String selected_text =
      static_cast<int32_t>(local.selected_unit_ids.size()) > 0
          ? " selected units: " +
                String::num_int64(static_cast<int32_t>(local.selected_unit_ids.size()))
          : "";
  if (local.selected_base_owner == PLAYER)
  {
    selected_text += " | base selected: W worker";
  }
  else if (local.selected_building_id != -1)
  {
    selected_text += " | barracks selected: F fighter";
  }
  else if (local.is_placing_barracks)
  {
    selected_text += " | placing barracks: left click place, right click cancel";
  }
  if (!sim.get_winner_text().is_empty())
  {
    return sim.get_winner_text() + String(" | press any key to restart");
  }
  return "Gold " + String::num_int64(sim.get_essence(PLAYER)) +
         " | left click select | right click command" + selected_text;
}

String HudPresenter::get_resource_text(const GameSimulation &sim)
{
  return "Food: 0\nWood: 0\nGold: " + String::num_int64(sim.get_essence(PLAYER));
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
    return unit.type == UnitType::WORKER ? "Worker" : "Fighter";
  }

  if (local.selected_base_owner == PLAYER)
  {
    return "Main Base";
  }

  BuildingSummary selected_building;
  if (selected_building_summary(sim, local, selected_building))
  {
    if (!selected_building.completed)
    {
      return "Barracks Site";
    }
    return "Barracks";
  }

  return "No Selection";
}

String HudPresenter::get_selected_actions_text(const GameSimulation &sim,
                                               const LocalPlayerState &local)
{
  UnitSummary unit;
  if (static_cast<int32_t>(local.selected_unit_ids.size()) > 0)
  {
    if (first_selected_unit_summary(sim, local, unit) && unit.type == UnitType::WORKER)
    {
      return "Right Click: Move / Gather\nBuild: Barracks";
    }
    return "Right Click: Attack-move\nRight Click Base: Attack";
  }

  if (local.selected_base_owner == PLAYER)
  {
    BaseSummary base;
    if (sim.get_base_summary(PLAYER, base) && base.training_worker)
    {
      return "Training Worker: " +
             String::num_int64(
                 static_cast<int64_t>((1.0f - base.train_timer / base.train_duration) * 100.0f)) +
             "%\nQueue: " + String::num_int64(base.worker_queue) + "/3";
    }
    return "W: Train Worker (20 Gold)\nRight Click: Set rally action\nQueue: " +
           String::num_int64(base.worker_queue) + "/3";
  }

  BuildingSummary selected_building;
  if (selected_building_summary(sim, local, selected_building))
  {
    if (!selected_building.completed)
    {
      return "Under construction\nDel: Cancel refund";
    }
    if (selected_building.training_fighter)
    {
      return "Training Fighter: " +
             String::num_int64(static_cast<int64_t>(
                 (1.0f - selected_building.train_timer / selected_building.train_duration) *
                 100.0f)) +
             "%\nQueue: " + String::num_int64(selected_building.fighter_queue) + "/3";
    }
    return "F: Train Fighter (25 Gold)\nRight Click: Set rally action\nQueue: " +
           String::num_int64(selected_building.fighter_queue) + "/3";
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
    String role = unit.type == UnitType::WORKER ? "Gathers Gold and builds Barracks."
                                                : "Basic melee combat unit.";
    return "HP: " + String::num_int64(static_cast<int64_t>(unit.hp)) + "\n" + role;
  }

  if (local.selected_base_owner == PLAYER)
  {
    BaseSummary base;
    const int64_t hp = sim.get_base_summary(PLAYER, base) ? static_cast<int64_t>(base.hp) : 0;
    if (base.training_worker)
    {
      return "HP: " + String::num_int64(hp) +
             "\nTraining Worker\nQueue: " + String::num_int64(base.worker_queue) + "/3";
    }
    return "HP: " + String::num_int64(hp) + "\nTrains workers.";
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
    if (selected_building.training_fighter)
    {
      return "HP: " + String::num_int64(static_cast<int64_t>(selected_building.hp)) +
             "\nTraining Fighter\nQueue: " + String::num_int64(selected_building.fighter_queue) +
             "/3";
    }
    return "HP: " + String::num_int64(static_cast<int64_t>(selected_building.hp)) +
           "\nTrains fighters.";
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
  if (local.selected_base_owner == PLAYER)
  {
    return Color(0.15f, 0.35f, 0.85f);
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
    return asset_contracts::humanoid_face(humanoid_asset_folder(unit.type));
  }

  if (local.selected_base_owner == PLAYER)
  {
    return asset_contracts::building_face("towncenter");
  }

  if (local.selected_building_id != -1)
  {
    return asset_contracts::building_face("barracks");
  }

  return "";
}

String HudPresenter::get_action_button_text(const GameSimulation &sim,
                                            const LocalPlayerState &local,
                                            int32_t index)
{
  if (local.selected_base_owner == PLAYER)
  {
    if (index == 0)
    {
      BaseSummary base;
      if (sim.get_base_summary(PLAYER, base) && base.training_worker)
      {
        return "Queue " + String::num_int64(base.worker_queue) + "/3";
      }
      return "20 Gold";
    }
  }

  BuildingSummary selected_building;
  if (selected_building_summary(sim, local, selected_building) && index == 0)
  {
    if (!selected_building.completed)
    {
      return "Building...";
    }
    if (selected_building.training_fighter)
    {
      return "Queue " + String::num_int64(selected_building.fighter_queue) + "/3";
    }
    return "25 Gold";
  }

  UnitSummary unit;
  if (first_selected_unit_summary(sim, local, unit))
  {
    if (unit.type == UnitType::WORKER)
    {
      return index == 0 ? "80 Gold" : "";
    }
    return index == 0 ? "Attack" : "Hold";
  }

  return index == 0 ? "No Action" : "";
}

String HudPresenter::get_action_button_icon_path(const GameSimulation &sim,
                                                 const LocalPlayerState &local,
                                                 int32_t index)
{
  if (local.selected_base_owner == PLAYER && index == 0)
  {
    return asset_contracts::humanoid_face("worker");
  }

  if (local.selected_building_id != -1 && index == 0)
  {
    return asset_contracts::humanoid_face("hoplite");
  }

  UnitSummary unit;
  if (first_selected_unit_summary(sim, local, unit) && unit.type == UnitType::WORKER && index == 0)
  {
    return asset_contracts::building_face("barracks");
  }

  return "";
}

bool HudPresenter::is_action_button_enabled(const GameSimulation &sim,
                                            const LocalPlayerState &local,
                                            int32_t index)
{
  if (local.selected_base_owner == PLAYER)
  {
    if (index == 0)
    {
      return sim.can_train_worker(PLAYER);
    }
  }

  BuildingSummary selected_building;
  if (selected_building_summary(sim, local, selected_building) && index == 0)
  {
    return sim.can_train_fighter(PLAYER, selected_building.id);
  }

  UnitSummary unit;
  if (first_selected_unit_summary(sim, local, unit) && unit.type == UnitType::WORKER && index == 0)
  {
    return sim.can_start_barracks_placement(PLAYER, unit.id);
  }

  return false;
}

} // namespace tinyv1

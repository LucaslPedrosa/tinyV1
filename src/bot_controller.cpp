#include "bot_controller.hpp"

#include "game_constants.hpp"
#include "game_simulation/game_simulation.hpp"

using namespace godot;

namespace tinyv1
{

void BotController::reset()
{
  decision_timer = 0.0f;
}

std::vector<GameCommand> BotController::update(const GameSimulation &p_simulation, double p_delta)
{
  std::vector<GameCommand> commands;

  decision_timer -= static_cast<float>(p_delta);
  if (decision_timer > 0.0f)
  {
    return commands;
  }
  decision_timer = BOT_ATTACK_INTERVAL;

  for (UnitId worker_id : p_simulation.get_idle_worker_ids(BOT))
  {
    Vector2 worker_position;
    if (!p_simulation.get_unit_position(worker_id, worker_position))
    {
      continue;
    }

    const ResourceId resource_id = p_simulation.find_nearest_resource_id(worker_position);
    Vector2 resource_position;
    if (p_simulation.get_resource_position(resource_id, resource_position))
    {
      GameCommand command;
      command.type = GameCommandType::COMMAND_SELECTED_TO;
      command.owner = BOT;
      command.position = resource_position;
      command.selected_unit_ids.push_back(worker_id);
      commands.push_back(command);
    }
  }

  const int32_t bot_workers = p_simulation.count_units(BOT, UnitType::WORKER);
  const int32_t bot_fighters = p_simulation.count_units(BOT, UnitType::FIGHTER);
  const int32_t bot_essence = p_simulation.get_essence(BOT);

  if (bot_workers < 5 && bot_essence >= WORKER_COST)
  {
    GameCommand command;
    command.type = GameCommandType::TRAIN_UNIT;
    command.owner = BOT;
    command.unit_type = UnitType::WORKER;
    commands.push_back(command);
  }
  else if (!p_simulation.has_barracks(BOT) && bot_essence >= BARRACKS_COST)
  {
    Vector2 bot_base_position;
    const UnitId builder_id = p_simulation.find_available_worker_id(BOT);
    if (p_simulation.get_base_position(BOT, bot_base_position) && builder_id != -1)
    {
      GameCommand command;
      command.type = GameCommandType::PLACE_BARRACKS;
      command.owner = BOT;
      command.position = bot_base_position + Vector2(-110, 78);
      command.builder_unit_id = builder_id;
      commands.push_back(command);
    }
  }
  else if (p_simulation.has_completed_barracks(BOT) && bot_essence >= FIGHTER_COST)
  {
    GameCommand command;
    command.type = GameCommandType::TRAIN_UNIT;
    command.owner = BOT;
    command.unit_type = UnitType::FIGHTER;
    commands.push_back(command);
  }

  if (bot_fighters >= 4)
  {
    Vector2 player_base_position;
    if (!p_simulation.get_base_position(PLAYER, player_base_position))
    {
      return commands;
    }

    GameCommand command;
    command.type = GameCommandType::COMMAND_SELECTED_TO;
    command.owner = BOT;
    command.position = player_base_position;
    for (UnitId fighter_id : p_simulation.get_unit_ids(BOT, UnitType::FIGHTER))
    {
      command.selected_unit_ids.push_back(fighter_id);
    }
    commands.push_back(command);
  }

  return commands;
}

} // namespace tinyv1

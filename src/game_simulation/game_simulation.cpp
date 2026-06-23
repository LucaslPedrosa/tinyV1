#include "game_simulation.hpp"

#include "definitions/building_definitions.hpp"
#include "game_constants.hpp"

using namespace godot;

namespace tinyv1
{

void GameSimulation::reset_match()
{
  map_rect = Rect2(Vector2(0, 0), Vector2(3200, 2200));
  resources.clear();
  buildings.clear();
  units.clear();
  player_resources = ResourceWallet{0, 0, 50, 0};
  bot_resources = ResourceWallet{0, 0, 50, 0};
  next_unit_id = 1;
  next_building_id = 1;
  winner_text = String();

  const BuildingDefinition &town_center_definition =
      get_building_definition(BuildingType::TOWN_CENTER);

  Building player_town_center;
  player_town_center.id = next_building_id++;
  player_town_center.type = BuildingType::TOWN_CENTER;
  player_town_center.owner_component.owner = PLAYER;
  player_town_center.transform_component.position = Vector2(420, 1100);
  player_town_center.health_component.hp = town_center_definition.max_hp;
  player_town_center.health_component.max_hp = town_center_definition.max_hp;
  player_town_center.construction_component.completed = true;
  buildings.push_back(player_town_center);

  Building bot_town_center;
  bot_town_center.id = next_building_id++;
  bot_town_center.type = BuildingType::TOWN_CENTER;
  bot_town_center.owner_component.owner = BOT;
  bot_town_center.transform_component.position = Vector2(2780, 1100);
  bot_town_center.health_component.hp = town_center_definition.max_hp;
  bot_town_center.health_component.max_hp = town_center_definition.max_hp;
  bot_town_center.construction_component.completed = true;
  buildings.push_back(bot_town_center);

  resources.push_back({1, ResourceType::GOLD, Vector2(700, 940), 700});
  resources.push_back({2, ResourceType::GOLD, Vector2(700, 1260), 700});
  resources.push_back({3, ResourceType::GOLD, Vector2(2500, 940), 700});
  resources.push_back({4, ResourceType::GOLD, Vector2(2500, 1260), 700});
  resources.push_back({5, ResourceType::GOLD, Vector2(1600, 1100), 1200});

  for (int32_t i = 0; i < 3; ++i)
  {
    Unit player_worker =
        create_unit(next_unit_id++, PLAYER, UnitType::WORKER, Vector2(740, 1054 + i * 34.0f));
    player_worker.order = UnitOrder::IDLE;
    units.push_back(player_worker);

    Unit bot_worker =
        create_unit(next_unit_id++, BOT, UnitType::WORKER, Vector2(2460, 1054 + i * 34.0f));
    bot_worker.order = UnitOrder::GATHER;
    bot_worker.movement_component.target_position = Vector2(2500, 940);
    bot_worker.gather_component.gathering_resource = false;
    bot_worker.gather_component.carrying = 0;
    bot_worker.gather_component.target_resource = 3;
    units.push_back(bot_worker);
  }
}

} // namespace tinyv1

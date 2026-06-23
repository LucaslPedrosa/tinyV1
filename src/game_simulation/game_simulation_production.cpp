#include "game_simulation.hpp"

#include "definitions/building_definitions.hpp"
#include "definitions/unit_definitions.hpp"
#include "game_constants.hpp"

#include <algorithm>

using namespace godot;

namespace tinyv1
{

namespace
{

bool can_gather_resource(const Unit &p_unit, const ResourceNode &p_resource)
{
  for (const GatherRule &rule : p_unit.gather_component.rules)
  {
    if (rule.resource_type == p_resource.type)
    {
      return true;
    }
  }
  return false;
}

bool building_can_train_unit(const Building &p_building, UnitType p_type)
{
  const BuildingDefinition &definition = get_building_definition(p_building.type);
  return std::find(definition.trainable_units.begin(), definition.trainable_units.end(), p_type) !=
         definition.trainable_units.end();
}

bool unit_can_build(const Unit &p_unit, BuildingType p_type)
{
  const UnitDefinition &definition = get_unit_definition(p_unit.type);
  return std::find(definition.buildable_buildings.begin(), definition.buildable_buildings.end(),
                   p_type) != definition.buildable_buildings.end();
}

} // namespace

Unit GameSimulation::create_unit(UnitId p_id, PlayerId p_owner, UnitType p_type,
                                 const Vector2 &p_position) const
{
  Unit unit;
  unit.object.id = p_id;
  unit.object.owner_component.owner = p_owner;
  unit.object.transform_component.position = p_position;
  unit.type = p_type;
  unit.movement_component.target_position = p_position;
  const UnitDefinition &definition = get_unit_definition(p_type);
  unit.object.health_component.hp = definition.max_hp;
  unit.object.health_component.max_hp = definition.max_hp;
  if (definition.can_gather)
  {
    unit.gather_component.rules.push_back({ResourceType::GOLD, GATHER_TIME, WORKER_CARRY_LIMIT});
  }
  return unit;
}

void GameSimulation::update_training(double p_delta)
{
  for (Building &building : buildings)
  {
    if (!building.production_component.training && building.construction_component.completed &&
        !building.production_component.is_empty())
    {
      UnitType active_unit = UnitType::WORKER;
      if (!building.production_component.pop(active_unit))
      {
        continue;
      }
      const UnitDefinition &unit_definition = get_unit_definition(active_unit);
      building.production_component.training = true;
      building.production_component.active_unit = active_unit;
      building.production_component.train_timer = unit_definition.train_time;
      building.production_component.train_duration = unit_definition.train_time;
    }

    if (!building.production_component.training)
    {
      continue;
    }

    building.production_component.train_timer -= static_cast<float>(p_delta);
    if (building.production_component.train_timer <= 0.0f)
    {
      building.production_component.training = false;
      building.production_component.train_timer = 0.0f;
      building.production_component.train_duration = 0.0f;
      spawn_unit(building.owner_component.owner, building.production_component.active_unit,
                 building.transform_component.position, BARRACKS_RADIUS + 24.0f,
                 &building.rally_component);
    }
  }
}

void GameSimulation::train_unit(int32_t p_owner, UnitType p_type, BuildingId p_source_building_id)
{
  ResourceWallet &resources = p_owner == PLAYER ? player_resources : bot_resources;
  const ResourceCost cost = get_unit_definition(p_type).cost;
  if (!resources.can_afford(cost))
  {
    return;
  }

  Building *source_building = nullptr;

  Building *selected_building = find_building_by_id(p_source_building_id);
  if (selected_building != nullptr && selected_building->owner_component.owner == p_owner &&
      selected_building->construction_component.completed &&
      building_can_train_unit(*selected_building, p_type))
  {
    source_building = selected_building;
  }
  else
  {
    for (Building &building : buildings)
    {
      if (building.owner_component.owner == p_owner && building.construction_component.completed &&
          building_can_train_unit(building, p_type))
      {
        source_building = &building;
        break;
      }
    }
  }

  if (source_building == nullptr || source_building->production_component.is_full())
  {
    return;
  }

  resources.spend(cost);
  source_building->production_component.push(p_type);
}

RallyActionComponent
GameSimulation::resolve_rally_action_for_production(PlayerId p_owner, UnitType p_produced_type,
                                                    const Vector2 &p_position) const
{
  RallyActionComponent rally;
  rally.position = p_position;
  rally.has_rally_action = true;

  if (p_produced_type == UnitType::WORKER)
  {
    const ResourceNode *resource = nullptr;
    for (const ResourceNode &candidate : resources)
    {
      if (candidate.amount > 0 &&
          distance_to(p_position, candidate.position) <= RESOURCE_RADIUS + 14.0f)
      {
        resource = &candidate;
        break;
      }
    }
    if (resource != nullptr)
    {
      rally.type = RallyActionType::GATHER;
      rally.target_resource = resource->id;
      rally.position = resource->position;
      return rally;
    }
    rally.type = RallyActionType::MOVE;
    return rally;
  }

  const int32_t enemy_unit_id = find_enemy_unit_id_at(p_owner, p_position);
  const BuildingId enemy_building_id = find_enemy_building_id_at(p_owner, p_position);
  if (enemy_unit_id != -1)
  {
    rally.type = RallyActionType::ATTACK_UNIT;
    rally.target_unit_id = enemy_unit_id;
    const Unit *enemy_unit = find_unit(enemy_unit_id);
    rally.position =
        enemy_unit != nullptr ? enemy_unit->object.transform_component.position : p_position;
    return rally;
  }
  if (enemy_building_id != -1)
  {
    rally.type = RallyActionType::ATTACK_BUILDING;
    rally.target_building_id = enemy_building_id;
    const Building *enemy_building = find_building_by_id(enemy_building_id);
    rally.position =
        enemy_building != nullptr ? enemy_building->transform_component.position : p_position;
    return rally;
  }
  rally.type = RallyActionType::ATTACK_MOVE;
  return rally;
}

void GameSimulation::apply_rally_action_to_unit(Unit &p_unit,
                                                const RallyActionComponent &p_rally_action)
{
  if (!p_rally_action.has_rally_action || p_rally_action.type == RallyActionType::NONE)
  {
    return;
  }

  p_unit.combat_component.target_unit_id = -1;
  p_unit.combat_component.target_building_id = -1;
  p_unit.build_component.target_building_id = -1;
  p_unit.gather_component.gathering_resource = false;

  switch (p_rally_action.type)
  {
  case RallyActionType::MOVE:
    p_unit.movement_component.target_position = p_rally_action.position;
    p_unit.order = UnitOrder::MOVE;
    break;
  case RallyActionType::GATHER:
  {
    const ResourceNode *resource = find_resource(p_rally_action.target_resource);
    if (resource != nullptr && can_gather_resource(p_unit, *resource))
    {
      p_unit.gather_component.target_resource = resource->id;
      p_unit.order = UnitOrder::GATHER;
      break;
    }
    p_unit.movement_component.target_position = p_rally_action.position;
    p_unit.order = UnitOrder::MOVE;
    break;
  }
  case RallyActionType::ATTACK_UNIT:
  {
    const Unit *enemy_unit = find_unit(p_rally_action.target_unit_id);
    if (enemy_unit != nullptr &&
        enemy_unit->object.owner_component.owner != p_unit.object.owner_component.owner)
    {
      p_unit.combat_component.target_unit_id = enemy_unit->object.id;
      p_unit.movement_component.target_position = enemy_unit->object.transform_component.position;
      p_unit.order = UnitOrder::ATTACK;
      break;
    }
    p_unit.movement_component.target_position = p_rally_action.position;
    p_unit.order = UnitOrder::ATTACK;
    break;
  }
  case RallyActionType::ATTACK_BUILDING:
  {
    const Building *target = find_building_by_id(p_rally_action.target_building_id);
    if (target != nullptr && target->owner_component.owner != p_unit.object.owner_component.owner)
    {
      p_unit.combat_component.target_building_id = target->id;
      p_unit.movement_component.target_position = target->transform_component.position;
      p_unit.order = UnitOrder::ATTACK;
      break;
    }
    p_unit.movement_component.target_position = p_rally_action.position;
    p_unit.order = UnitOrder::ATTACK;
    break;
  }
  case RallyActionType::ATTACK_MOVE:
    p_unit.movement_component.target_position = p_rally_action.position;
    p_unit.order = UnitOrder::ATTACK;
    break;
  case RallyActionType::NONE:
    break;
  }
}

void GameSimulation::spawn_unit(int32_t p_owner, UnitType p_type, const Vector2 &p_source_position,
                                float p_spawn_distance, const RallyActionComponent *p_rally_action)
{
  const float side = p_owner == PLAYER ? 1.0f : -1.0f;
  const UnitId unit_id = next_unit_id++;
  Unit unit = create_unit(unit_id, p_owner, p_type,
                          p_source_position +
                              Vector2(p_spawn_distance * side, 24.0f * ((unit_id % 3) - 1)));
  if (p_rally_action != nullptr && p_rally_action->has_rally_action)
  {
    apply_rally_action_to_unit(unit, *p_rally_action);
  }
  else if (p_owner == BOT && p_type == UnitType::WORKER)
  {
    unit.gather_component.target_resource =
        find_nearest_resource(unit.object.transform_component.position);
    unit.order = UnitOrder::GATHER;
  }
  units.push_back(unit);
}

BuildingId GameSimulation::place_building(int32_t p_owner, BuildingType p_type,
                                          const Vector2 &p_position, Unit *p_builder)
{
  const BuildingDefinition &definition = get_building_definition(p_type);
  ResourceWallet &resources = p_owner == PLAYER ? player_resources : bot_resources;
  if (p_builder == nullptr || p_builder->object.owner_component.owner != p_owner ||
      !unit_can_build(*p_builder, p_type))
  {
    return -1;
  }

  if (!resources.can_afford(definition.cost))
  {
    return -1;
  }

  if (!map_rect.has_point(p_position))
  {
    return -1;
  }

  if (does_overlap(p_position, definition.footprint))
  {
    return -1;
  }

  resources.spend(definition.cost);
  const BuildingId building_id = next_building_id++;
  Building building;
  building.id = building_id;
  building.type = p_type;
  building.owner_component.owner = p_owner;
  building.transform_component.position = p_position;
  building.health_component.hp = definition.max_hp;
  building.health_component.max_hp = definition.max_hp;
  building.construction_component.build_progress = 0.0f;
  building.construction_component.completed = false;
  buildings.push_back(building);
  p_builder->gather_component.gathering_resource = false;
  p_builder->order = UnitOrder::BUILD;
  p_builder->build_component.target_building_id = building_id;
  p_builder->movement_component.target_position = p_position;
  return building_id;
}

void GameSimulation::delete_building(BuildingId p_building_id)
{
  int32_t building_index = -1;
  for (int32_t i = 0; i < static_cast<int32_t>(buildings.size()); ++i)
  {
    if (buildings[i].id == p_building_id)
    {
      building_index = i;
      break;
    }
  }
  if (building_index == -1)
  {
    return;
  }

  const Building removed = buildings[building_index];
  if (removed.type == BuildingType::TOWN_CENTER)
  {
    return;
  }
  const BuildingDefinition &definition = get_building_definition(removed.type);
  if (removed.owner_component.owner == PLAYER)
  {
    player_resources.add(removed.construction_component.completed
                             ? ResourceCost{definition.cost.food / 2, definition.cost.wood / 2,
                                            definition.cost.gold / 2, definition.cost.favor / 2}
                             : definition.cost);
  }
  else
  {
    bot_resources.add(removed.construction_component.completed
                          ? ResourceCost{definition.cost.food / 2, definition.cost.wood / 2,
                                         definition.cost.gold / 2, definition.cost.favor / 2}
                          : definition.cost);
  }

  buildings.erase(buildings.begin() + building_index);
  for (Unit &unit : units)
  {
    if (unit.build_component.target_building_id == p_building_id)
    {
      unit.build_component.target_building_id = -1;
      if (unit.order == UnitOrder::BUILD)
      {
        unit.order = UnitOrder::IDLE;
      }
    }
    if (unit.combat_component.target_building_id == p_building_id)
    {
      unit.combat_component.target_building_id = -1;
      if (unit.order == UnitOrder::ATTACK)
      {
        unit.order = UnitOrder::IDLE;
      }
    }
  }
}

} // namespace tinyv1

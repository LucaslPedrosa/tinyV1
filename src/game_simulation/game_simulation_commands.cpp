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

bool has_gather_rule(const GatherComponent &p_gather_component, ResourceType p_resource_type)
{
  for (const GatherRule &rule : p_gather_component.rules)
  {
    if (rule.resource_type == p_resource_type)
    {
      return true;
    }
  }
  return false;
}

bool unit_can_build(const Unit &p_unit, BuildingType p_type)
{
  const UnitDefinition &definition = get_unit_definition(p_unit.type);
  return std::find(definition.buildable_buildings.begin(), definition.buildable_buildings.end(),
                   p_type) != definition.buildable_buildings.end();
}

UnitType first_trainable_unit(BuildingType p_type, UnitType p_fallback)
{
  const BuildingDefinition &definition = get_building_definition(p_type);
  return definition.trainable_units.empty() ? p_fallback : definition.trainable_units.front();
}

} // namespace

GameCommandResult GameSimulation::apply_command(const GameCommand &p_command)
{
  GameCommandResult result;
  switch (p_command.type)
  {
  case GameCommandType::COMMAND_SELECTED_TO:
    result.feedback = command_selected_to(p_command.owner, p_command.selected_unit_ids,
                                           p_command.selected_building_id, p_command.position);
    break;
  case GameCommandType::TRAIN_UNIT:
    train_unit(p_command.owner, p_command.unit_type, p_command.selected_building_id);
    break;
  case GameCommandType::PLACE_BUILDING:
  {
    Unit *builder = find_unit(p_command.builder_unit_id);
    result.placed_building_id =
        place_building(p_command.owner, p_command.building_type, p_command.position, builder);
    break;
  }
  case GameCommandType::DELETE_OBJECT:
    delete_building(p_command.selected_building_id);
    break;
  }
  return result;
}

bool GameSimulation::set_selected_production_rally_point(int32_t p_owner,
                                                         BuildingId p_selected_building_id,
                                                         const Vector2 &p_position)
{
  Building *building = find_building_by_id(p_selected_building_id);
  if (building != nullptr)
  {
    if (building->owner_component.owner == p_owner)
    {
      const UnitType produced_type = first_trainable_unit(building->type, UnitType::FIGHTER);
      building->rally_component =
          resolve_rally_action_for_production(p_owner, produced_type, p_position);
    }
    return true;
  }

  return false;
}


CommandFeedback GameSimulation::command_selected_to(int32_t p_owner,
                                                     const std::vector<int32_t> &p_selected_unit_ids,
                                                     BuildingId p_selected_building_id,
                                                     const Vector2 &p_position)
{
  CommandFeedback feedback;
  if (set_selected_production_rally_point(p_owner, p_selected_building_id, p_position))
  {
    feedback.has_marker = true;
    Building *building = find_building_by_id(p_selected_building_id);
    feedback.marker_position =
        building != nullptr ? building->rally_component.position : p_position;
    return feedback;
  }

  int32_t resource_id = -1;
  ResourceType resource_type = ResourceType::GOLD;
  Vector2 marker_position = p_position;
  for (const ResourceNode &resource : resources)
  {
    if (resource.amount > 0 &&
        distance_to(p_position, resource.position) <= RESOURCE_RADIUS + 14.0f)
    {
      resource_id = resource.id;
      resource_type = resource.type;
      marker_position = resource.position;
      break;
    }
  }
  feedback.has_marker = true;
  feedback.marker_position = marker_position;

  BuildingId build_building_id = -1;
  for (const Building &building : buildings)
  {
    if (building.owner_component.owner == p_owner &&
        distance_to(p_position, building.transform_component.position) <= BARRACKS_RADIUS + 18.0f)
    {
      build_building_id = building.id;
      break;
    }
  }
  const BuildingId enemy_building_id = find_enemy_building_id_at(p_owner, p_position);
  const int32_t enemy_unit_id = find_enemy_unit_id_at(p_owner, p_position);

  const int32_t selected_count = static_cast<int32_t>(p_selected_unit_ids.size());

  int32_t selected_index = 0;
  for (int32_t unit_id : p_selected_unit_ids)
  {
    Unit *selected_unit = find_unit(unit_id);
    if (selected_unit == nullptr || selected_unit->object.owner_component.owner != p_owner)
    {
      continue;
    }
    Unit &unit = *selected_unit;

    const Vector2 offset = formation_offset(selected_index, selected_count);
    selected_index++;

    Building *build_target = find_building_by_id(build_building_id);
    if (build_target != nullptr && unit_can_build(unit, build_target->type) &&
        !build_target->construction_component.completed)
    {
      unit.gather_component.gathering_resource = false;
      unit.combat_component.target_unit_id = -1;
      unit.build_component.target_building_id = build_building_id;
      unit.movement_component.target_position = build_target->transform_component.position;
      unit.order = UnitOrder::BUILD;
    }
    else if (resource_id != -1 && has_gather_rule(unit.gather_component, resource_type))
    {
      unit.gather_component.gathering_resource = false;
      unit.combat_component.target_unit_id = -1;
      unit.build_component.target_building_id = -1;
      unit.gather_component.target_resource = resource_id;
      unit.order = UnitOrder::GATHER;
    }
    else if (enemy_building_id != -1 || enemy_unit_id != -1 ||
             get_unit_definition(unit.type).can_attack_move)
    {
      unit.gather_component.gathering_resource = false;
      unit.combat_component.target_unit_id = enemy_unit_id;
      unit.combat_component.target_building_id = enemy_building_id;
      if (enemy_building_id != -1)
      {
        const Building *enemy_building = find_building_by_id(enemy_building_id);
        unit.movement_component.target_position =
            enemy_building != nullptr ? enemy_building->transform_component.position + offset
                                      : p_position + offset;
      }
      else if (enemy_unit_id != -1)
      {
        Unit *enemy_unit = find_unit(enemy_unit_id);
        unit.movement_component.target_position =
            enemy_unit != nullptr ? enemy_unit->object.transform_component.position + offset
                                  : p_position + offset;
      }
      else
      {
        unit.movement_component.target_position = p_position + offset;
      }
      unit.order = UnitOrder::ATTACK;
    }
    else
    {
      unit.gather_component.gathering_resource = false;
      unit.combat_component.target_unit_id = -1;
      unit.build_component.target_building_id = -1;
      unit.movement_component.target_position = p_position + offset;
      unit.order = UnitOrder::MOVE;
    }
  }
  return feedback;
}

} // namespace tinyv1

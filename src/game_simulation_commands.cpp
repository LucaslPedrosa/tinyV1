#include "game_simulation.hpp"

#include "game_constants.hpp"

using namespace godot;

namespace tinyv1 {

GameCommandResult GameSimulation::apply_command(const GameCommand &p_command) {
	GameCommandResult result;
	switch (p_command.type) {
		case GameCommandType::COMMAND_SELECTED_TO:
			result.feedback = command_selected_to(p_command.owner, p_command.selected_unit_ids, p_command.selected_base_owner, p_command.selected_building_id, p_command.position);
			break;
		case GameCommandType::TRAIN_UNIT:
			train_unit(p_command.owner, p_command.unit_type, p_command.selected_building_id);
			break;
		case GameCommandType::PLACE_BARRACKS: {
			Unit *builder = find_unit(p_command.builder_unit_id);
			result.placed_building_id = place_barracks(p_command.owner, p_command.position, builder);
			break;
		}
		case GameCommandType::DELETE_BARRACKS:
			delete_barracks(p_command.selected_building_id);
			break;
	}
	return result;
}

bool GameSimulation::set_selected_production_rally_point(int32_t p_owner, int32_t p_selected_base_owner, BuildingId p_selected_building_id, const Vector2 &p_position) {
	if (p_selected_base_owner == p_owner) {
		Base *base = find_base(p_owner);
		if (base != nullptr) {
			base->rally_component.has_rally_point = true;
			base->rally_component.rally_point = p_position;
		}
		return true;
	}

	Barracks *building = find_barracks_by_id(p_selected_building_id);
	if (building != nullptr) {
		if (building->owner_component.owner == p_owner) {
			building->rally_component.has_rally_point = true;
			building->rally_component.rally_point = p_position;
		}
		return true;
	}

	return false;
}


CommandFeedback GameSimulation::command_selected_to(int32_t p_owner, const std::vector<int32_t> &p_selected_unit_ids, int32_t p_selected_base_owner, BuildingId p_selected_building_id, const Vector2 &p_position) {
	CommandFeedback feedback;
	if (set_selected_production_rally_point(p_owner, p_selected_base_owner, p_selected_building_id, p_position)) {
		feedback.has_marker = true;
		feedback.marker_position = p_position;
		return feedback;
	}

	int32_t resource_id = -1;
	Vector2 marker_position = p_position;
	for (const ResourceNode &resource : resources) {
		if (resource.amount > 0 && distance_to(p_position, resource.position) <= RESOURCE_RADIUS + 14.0f) {
			resource_id = resource.id;
			marker_position = resource.position;
			break;
		}
	}
	feedback.has_marker = true;
	feedback.marker_position = marker_position;

	const int32_t enemy_owner = p_owner == PLAYER ? BOT : PLAYER;
	bool attack_enemy_base = false;
	BuildingId build_barracks_id = -1;
	for (const Barracks &building : barracks) {
		if (building.owner_component.owner == p_owner && distance_to(p_position, building.transform_component.position) <= BARRACKS_RADIUS + 18.0f) {
			build_barracks_id = building.id;
			break;
		}
	}
	const BuildingId enemy_barracks_id = find_enemy_barracks_id_at(p_owner, p_position);
	const int32_t enemy_unit_id = find_enemy_unit_id_at(p_owner, p_position);
	Base *enemy_base = find_base(enemy_owner);
	if (enemy_base != nullptr && distance_to(p_position, enemy_base->transform_component.position) <= BASE_RADIUS + 24.0f) {
		attack_enemy_base = true;
	}

	const int32_t selected_count = static_cast<int32_t>(p_selected_unit_ids.size());

	int32_t selected_index = 0;
	for (int32_t unit_id : p_selected_unit_ids) {
		Unit *selected_unit = find_unit(unit_id);
		if (selected_unit == nullptr || selected_unit->owner_component.owner != p_owner) {
			continue;
		}
		Unit &unit = *selected_unit;

		const Vector2 offset = formation_offset(selected_index, selected_count);
		selected_index++;

		Barracks *build_barracks = find_barracks_by_id(build_barracks_id);
		if (build_barracks != nullptr && unit.type == UnitType::WORKER && !build_barracks->construction_component.completed) {
			unit.gather_component.gathering_resource = false;
			unit.combat_component.target_unit_id = -1;
			unit.combat_component.target_base_owner = -1;
			unit.build_component.target_building_id = build_barracks_id;
			unit.movement_component.target_position = build_barracks->transform_component.position;
			unit.order = UnitOrder::BUILD;
		} else if (resource_id != -1 && unit.type == UnitType::WORKER) {
			unit.gather_component.gathering_resource = false;
			unit.combat_component.target_unit_id = -1;
			unit.combat_component.target_base_owner = -1;
			unit.build_component.target_building_id = -1;
			unit.gather_component.target_resource = resource_id;
			unit.order = UnitOrder::GATHER;
		} else if (attack_enemy_base || enemy_barracks_id != -1 || enemy_unit_id != -1 || unit.type == UnitType::FIGHTER) {
			unit.gather_component.gathering_resource = false;
			unit.combat_component.target_unit_id = enemy_unit_id;
			unit.combat_component.target_base_owner = attack_enemy_base ? enemy_owner : -1;
			unit.combat_component.target_building_id = enemy_barracks_id;
			if (enemy_barracks_id != -1) {
				const Barracks *enemy_barracks = find_barracks_by_id(enemy_barracks_id);
				unit.movement_component.target_position = enemy_barracks != nullptr ? enemy_barracks->transform_component.position + offset : p_position + offset;
			} else if (enemy_unit_id != -1) {
				Unit *enemy_unit = find_unit(enemy_unit_id);
				unit.movement_component.target_position = enemy_unit != nullptr ? enemy_unit->transform_component.position + offset : p_position + offset;
			} else {
				unit.movement_component.target_position = attack_enemy_base && enemy_base != nullptr ? enemy_base->transform_component.position + offset : p_position + offset;
			}
			unit.order = UnitOrder::ATTACK;
		} else {
			unit.gather_component.gathering_resource = false;
			unit.combat_component.target_unit_id = -1;
			unit.build_component.target_building_id = -1;
			unit.combat_component.target_base_owner = -1;
			unit.movement_component.target_position = p_position + offset;
			unit.order = UnitOrder::MOVE;
		}
	}
	return feedback;
}

} // namespace tinyv1

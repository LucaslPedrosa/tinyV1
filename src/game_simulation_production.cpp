#include "game_simulation.hpp"

#include "game_constants.hpp"

#include <algorithm>

using namespace godot;

namespace tinyv1 {

void GameSimulation::update_training(double p_delta) {
	for (Base &base : bases) {
		if (!base.production_component.training && base.production_component.queue > 0) {
			base.production_component.training = true;
			base.production_component.train_timer = WORKER_TRAIN_TIME;
			base.production_component.train_duration = WORKER_TRAIN_TIME;
		}

		if (!base.production_component.training) {
			continue;
		}

		base.production_component.train_timer -= static_cast<float>(p_delta);
		if (base.production_component.train_timer <= 0.0f) {
			base.production_component.training = false;
			base.production_component.train_timer = 0.0f;
			base.production_component.train_duration = 0.0f;
			base.production_component.queue = std::max(0, base.production_component.queue - 1);
			spawn_unit(base.owner_component.owner, UnitType::WORKER, base.transform_component.position, base.rally_component.has_rally_point, base.rally_component.rally_point);
		}
	}

	for (Barracks &building : barracks) {
		if (!building.production_component.training && building.construction_component.completed && building.production_component.queue > 0) {
			building.production_component.training = true;
			building.production_component.train_timer = FIGHTER_TRAIN_TIME;
			building.production_component.train_duration = FIGHTER_TRAIN_TIME;
		}

		if (!building.production_component.training) {
			continue;
		}

		building.production_component.train_timer -= static_cast<float>(p_delta);
		if (building.production_component.train_timer <= 0.0f) {
			building.production_component.training = false;
			building.production_component.train_timer = 0.0f;
			building.production_component.train_duration = 0.0f;
			building.production_component.queue = std::max(0, building.production_component.queue - 1);
			spawn_unit(building.owner_component.owner, UnitType::FIGHTER, building.transform_component.position, building.rally_component.has_rally_point, building.rally_component.rally_point);
		}
	}
}

void GameSimulation::train_unit(int32_t p_owner, UnitType p_type, BuildingId p_source_building_id) {
	int32_t &essence = p_owner == PLAYER ? player_essence : bot_essence;
	const int32_t cost = p_type == UnitType::WORKER ? static_cast<int32_t>(WORKER_COST) : static_cast<int32_t>(FIGHTER_COST);
	if (essence < cost) {
		return;
	}

	Base *base = find_base(p_owner);
	Barracks *source_barracks = nullptr;
	if (p_type == UnitType::FIGHTER) {
		Barracks *selected_barracks = find_barracks_by_id(p_source_building_id);
		if (selected_barracks != nullptr && selected_barracks->owner_component.owner == p_owner && selected_barracks->construction_component.completed) {
			source_barracks = selected_barracks;
		} else {
			source_barracks = find_completed_barracks(p_owner);
		}
	}
	if (p_type == UnitType::WORKER && (base == nullptr || base->production_component.queue >= MAX_TRAIN_QUEUE)) {
		return;
	}
	if (p_type == UnitType::FIGHTER && (source_barracks == nullptr || source_barracks->production_component.queue >= MAX_TRAIN_QUEUE)) {
		return;
	}

	essence -= cost;
	if (p_type == UnitType::WORKER) {
		base->production_component.queue++;
	} else {
		source_barracks->production_component.queue++;
	}
}

void GameSimulation::spawn_unit(int32_t p_owner, UnitType p_type, const Vector2 &p_source_position, bool p_has_rally_point, const Vector2 &p_rally_point) {
	const float side = p_owner == PLAYER ? 1.0f : -1.0f;
	Unit unit;
	unit.id = next_unit_id++;
	unit.owner_component.owner = p_owner;
	unit.type = p_type;
	unit.transform_component.position = p_source_position + Vector2(70.0f * side, 24.0f * ((unit.id % 3) - 1));
	unit.movement_component.target_position = unit.transform_component.position;
	unit.health_component.hp = p_type == UnitType::WORKER ? 45.0f : 80.0f;
	unit.health_component.max_hp = p_type == UnitType::WORKER ? 45.0f : 80.0f;
	if (p_has_rally_point) {
		unit.movement_component.target_position = p_rally_point;
		unit.order = UnitOrder::MOVE;
	} else if (p_owner == BOT && p_type == UnitType::WORKER) {
		unit.gather_component.target_resource = find_nearest_resource(unit.transform_component.position);
		unit.order = UnitOrder::GATHER;
	}
	units.push_back(unit);
}

BuildingId GameSimulation::place_barracks(int32_t p_owner, const Vector2 &p_position, Unit *p_builder) {
	int32_t &essence = p_owner == PLAYER ? player_essence : bot_essence;
	if (essence < BARRACKS_COST) {
		return -1;
	}

	if (!map_rect.has_point(p_position)) {
		return -1;
	}

	essence -= static_cast<int32_t>(BARRACKS_COST);
	const BuildingId building_id = next_building_id++;
	Barracks building;
	building.id = building_id;
	building.owner_component.owner = p_owner;
	building.transform_component.position = p_position;
	building.health_component.hp = 250.0f;
	building.health_component.max_hp = 250.0f;
	building.construction_component.build_progress = 0.0f;
	building.construction_component.completed = false;
	barracks.push_back(building);
	if (p_builder != nullptr && p_builder->owner_component.owner == p_owner && p_builder->type == UnitType::WORKER) {
		p_builder->gather_component.gathering_resource = false;
		p_builder->order = UnitOrder::BUILD;
		p_builder->build_component.target_building_id = building_id;
		p_builder->movement_component.target_position = p_position;
	}
	return building_id;
}

void GameSimulation::delete_barracks(BuildingId p_building_id) {
	int32_t barracks_index = -1;
	for (int32_t i = 0; i < static_cast<int32_t>(barracks.size()); ++i) {
		if (barracks[i].id == p_building_id) {
			barracks_index = i;
			break;
		}
	}
	if (barracks_index == -1) {
		return;
	}

	const Barracks removed = barracks[barracks_index];
	if (removed.owner_component.owner == PLAYER) {
		player_essence += removed.construction_component.completed ? static_cast<int32_t>(BARRACKS_COST * 0.5f) : static_cast<int32_t>(BARRACKS_COST);
	} else {
		bot_essence += removed.construction_component.completed ? static_cast<int32_t>(BARRACKS_COST * 0.5f) : static_cast<int32_t>(BARRACKS_COST);
	}

	barracks.erase(barracks.begin() + barracks_index);
	for (Unit &unit : units) {
		if (unit.build_component.target_building_id == p_building_id) {
			unit.build_component.target_building_id = -1;
			if (unit.order == UnitOrder::BUILD) {
				unit.order = UnitOrder::IDLE;
			}
		}
		if (unit.combat_component.target_building_id == p_building_id) {
			unit.combat_component.target_building_id = -1;
			if (unit.order == UnitOrder::ATTACK) {
				unit.order = UnitOrder::IDLE;
			}
		}
	}
}

} // namespace tinyv1

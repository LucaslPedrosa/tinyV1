#include "game_simulation.hpp"

#include "game_constants.hpp"

#include <algorithm>

using namespace godot;

namespace tinyv1 {

namespace {

bool can_gather_resource(const Unit &p_unit, const ResourceNode &p_resource) {
	for (const GatherRule &rule : p_unit.gather_component.rules) {
		if (rule.resource_type == p_resource.type) {
			return true;
		}
	}
	return false;
}

} // namespace

Unit GameSimulation::create_unit(UnitId p_id, PlayerId p_owner, UnitType p_type, const Vector2 &p_position) const {
	Unit unit;
	unit.object.id = p_id;
	unit.object.owner_component.owner = p_owner;
	unit.object.transform_component.position = p_position;
	unit.type = p_type;
	unit.movement_component.target_position = p_position;
	unit.object.health_component.hp = p_type == UnitType::WORKER ? 45.0f : 80.0f;
	unit.object.health_component.max_hp = p_type == UnitType::WORKER ? 45.0f : 80.0f;
	if (p_type == UnitType::WORKER) {
		unit.gather_component.rules.push_back({ ResourceType::ESSENCE, GATHER_TIME, WORKER_CARRY_LIMIT });
	}
	return unit;
}

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
			spawn_unit(base.owner_component.owner, UnitType::WORKER, base.transform_component.position, BASE_RADIUS + 24.0f, &base.rally_component);
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
			spawn_unit(building.owner_component.owner, UnitType::FIGHTER, building.transform_component.position, BARRACKS_RADIUS + 24.0f, &building.rally_component);
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

RallyActionComponent GameSimulation::resolve_rally_action_for_production(PlayerId p_owner, UnitType p_produced_type, const Vector2 &p_position) const {
	RallyActionComponent rally;
	rally.position = p_position;
	rally.has_rally_action = true;

	if (p_produced_type == UnitType::WORKER) {
		const ResourceNode *resource = nullptr;
		for (const ResourceNode &candidate : resources) {
			if (candidate.amount > 0 && distance_to(p_position, candidate.position) <= RESOURCE_RADIUS + 14.0f) {
				resource = &candidate;
				break;
			}
		}
		if (resource != nullptr) {
			rally.type = RallyActionType::GATHER;
			rally.target_resource = resource->id;
			rally.position = resource->position;
			return rally;
		}
		rally.type = RallyActionType::MOVE;
		return rally;
	}

	const int32_t enemy_owner = p_owner == PLAYER ? BOT : PLAYER;
	const int32_t enemy_unit_id = find_enemy_unit_id_at(p_owner, p_position);
	const BuildingId enemy_barracks_id = find_enemy_barracks_id_at(p_owner, p_position);
	const Base *enemy_base = find_base(enemy_owner);
	if (enemy_unit_id != -1) {
		rally.type = RallyActionType::ATTACK_UNIT;
		rally.target_unit_id = enemy_unit_id;
		const Unit *enemy_unit = find_unit(enemy_unit_id);
		rally.position = enemy_unit != nullptr ? enemy_unit->object.transform_component.position : p_position;
		return rally;
	}
	if (enemy_barracks_id != -1) {
		rally.type = RallyActionType::ATTACK_BUILDING;
		rally.target_building_id = enemy_barracks_id;
		const Barracks *enemy_barracks = find_barracks_by_id(enemy_barracks_id);
		rally.position = enemy_barracks != nullptr ? enemy_barracks->transform_component.position : p_position;
		return rally;
	}
	if (enemy_base != nullptr && distance_to(p_position, enemy_base->transform_component.position) <= BASE_RADIUS + 24.0f) {
		rally.type = RallyActionType::ATTACK_BASE;
		rally.target_base_owner = enemy_owner;
		rally.position = enemy_base->transform_component.position;
		return rally;
	}

	rally.type = RallyActionType::ATTACK_MOVE;
	return rally;
}

void GameSimulation::apply_rally_action_to_unit(Unit &p_unit, const RallyActionComponent &p_rally_action) {
	if (!p_rally_action.has_rally_action || p_rally_action.type == RallyActionType::NONE) {
		return;
	}

	p_unit.combat_component.target_unit_id = -1;
	p_unit.combat_component.target_base_owner = -1;
	p_unit.combat_component.target_building_id = -1;
	p_unit.build_component.target_building_id = -1;
	p_unit.gather_component.gathering_resource = false;

	switch (p_rally_action.type) {
		case RallyActionType::MOVE:
			p_unit.movement_component.target_position = p_rally_action.position;
			p_unit.order = UnitOrder::MOVE;
			break;
		case RallyActionType::GATHER: {
			const ResourceNode *resource = find_resource(p_rally_action.target_resource);
			if (resource != nullptr && can_gather_resource(p_unit, *resource)) {
				p_unit.gather_component.target_resource = resource->id;
				p_unit.order = UnitOrder::GATHER;
				break;
			}
			p_unit.movement_component.target_position = p_rally_action.position;
			p_unit.order = UnitOrder::MOVE;
			break;
		}
		case RallyActionType::ATTACK_UNIT: {
			const Unit *enemy_unit = find_unit(p_rally_action.target_unit_id);
			if (enemy_unit != nullptr && enemy_unit->object.owner_component.owner != p_unit.object.owner_component.owner) {
				p_unit.combat_component.target_unit_id = enemy_unit->object.id;
				p_unit.movement_component.target_position = enemy_unit->object.transform_component.position;
				p_unit.order = UnitOrder::ATTACK;
				break;
			}
			p_unit.movement_component.target_position = p_rally_action.position;
			p_unit.order = UnitOrder::ATTACK;
			break;
		}
		case RallyActionType::ATTACK_BUILDING: {
			const Barracks *target = find_barracks_by_id(p_rally_action.target_building_id);
			if (target != nullptr && target->owner_component.owner != p_unit.object.owner_component.owner) {
				p_unit.combat_component.target_building_id = target->id;
				p_unit.movement_component.target_position = target->transform_component.position;
				p_unit.order = UnitOrder::ATTACK;
				break;
			}
			p_unit.movement_component.target_position = p_rally_action.position;
			p_unit.order = UnitOrder::ATTACK;
			break;
		}
		case RallyActionType::ATTACK_BASE: {
			const Base *enemy_base = find_base(p_rally_action.target_base_owner);
			if (enemy_base != nullptr && enemy_base->owner_component.owner != p_unit.object.owner_component.owner) {
				p_unit.combat_component.target_base_owner = enemy_base->owner_component.owner;
				p_unit.movement_component.target_position = enemy_base->transform_component.position;
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

void GameSimulation::spawn_unit(int32_t p_owner, UnitType p_type, const Vector2 &p_source_position, float p_spawn_distance, const RallyActionComponent *p_rally_action) {
	const float side = p_owner == PLAYER ? 1.0f : -1.0f;
	const UnitId unit_id = next_unit_id++;
	Unit unit = create_unit(unit_id, p_owner, p_type, p_source_position + Vector2(p_spawn_distance * side, 24.0f * ((unit_id % 3) - 1)));
	if (p_rally_action != nullptr && p_rally_action->has_rally_action) {
		apply_rally_action_to_unit(unit, *p_rally_action);
	} else if (p_owner == BOT && p_type == UnitType::WORKER) {
		unit.gather_component.target_resource = find_nearest_resource(unit.object.transform_component.position);
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
	if (p_builder != nullptr && p_builder->object.owner_component.owner == p_owner && p_builder->type == UnitType::WORKER) {
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

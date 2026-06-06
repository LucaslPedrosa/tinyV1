#include "game_simulation.hpp"

#include "game_constants.hpp"

#include <algorithm>

namespace tinyv1 {

void GameSimulation::update_units(double p_delta) {
	for (Unit &unit : units) {
		update_unit(unit, p_delta);
	}
}

void GameSimulation::update_unit_timers(Unit &p_unit, double p_delta) {
	p_unit.combat_component.attack_timer = std::max(0.0f, p_unit.combat_component.attack_timer - static_cast<float>(p_delta));
	p_unit.gather_component.gather_timer = std::max(0.0f, p_unit.gather_component.gather_timer - static_cast<float>(p_delta));
}

void GameSimulation::update_unit_auto_aggro(Unit &p_unit) {
	if (p_unit.type == UnitType::FIGHTER && p_unit.order != UnitOrder::ATTACK) {
		const int32_t enemy_id = find_enemy_unit_id(p_unit.owner_component.owner, p_unit.transform_component.position, 140.0f);
		if (enemy_id != -1) {
			p_unit.order = UnitOrder::ATTACK;
			p_unit.combat_component.target_unit_id = enemy_id;
			p_unit.combat_component.target_base_owner = -1;
			p_unit.combat_component.target_building_id = -1;
		}
	}
}

void GameSimulation::update_unit_build(Unit &p_unit, double p_delta) {
	Barracks *building = find_barracks_by_id(p_unit.build_component.target_building_id);
	if (building == nullptr) {
		p_unit.order = UnitOrder::IDLE;
		return;
	}

	if (building->construction_component.completed) {
		p_unit.order = UnitOrder::IDLE;
		return;
	}

	if (distance_to(p_unit.transform_component.position, building->transform_component.position) > BUILD_DISTANCE) {
		p_unit.transform_component.position = move_toward(p_unit.transform_component.position, building->transform_component.position, unit_speed(p_unit) * static_cast<float>(p_delta));
		return;
	}

	building->construction_component.build_progress = std::min(1.0f, building->construction_component.build_progress + static_cast<float>(p_delta) / BARRACKS_BUILD_TIME);
	if (building->construction_component.build_progress >= 1.0f) {
		building->construction_component.completed = true;
		p_unit.order = UnitOrder::IDLE;
	}
}

void GameSimulation::update_unit_move(Unit &p_unit, double p_delta) {
	p_unit.transform_component.position = move_toward(p_unit.transform_component.position, p_unit.movement_component.target_position, unit_speed(p_unit) * static_cast<float>(p_delta));
	if (distance_to(p_unit.transform_component.position, p_unit.movement_component.target_position) <= 2.0f) {
		p_unit.order = UnitOrder::IDLE;
	}
}

void GameSimulation::update_unit_gather(Unit &p_unit, double p_delta) {
	ResourceNode *resource = find_resource(p_unit.gather_component.target_resource);
	if (resource == nullptr || resource->amount <= 0) {
		p_unit.gather_component.gathering_resource = false;
		p_unit.order = UnitOrder::IDLE;
		return;
	}

	if (distance_to(p_unit.transform_component.position, resource->position) > GATHER_DISTANCE) {
		p_unit.gather_component.gathering_resource = false;
		p_unit.transform_component.position = move_toward(p_unit.transform_component.position, resource->position, unit_speed(p_unit) * static_cast<float>(p_delta));
		return;
	}

	if (!p_unit.gather_component.gathering_resource) {
		p_unit.gather_component.gathering_resource = true;
		p_unit.gather_component.gather_timer = GATHER_TIME;
		return;
	}

	if (p_unit.gather_component.gather_timer <= 0.0f) {
		const int32_t gathered = std::min(WORKER_CARRY_LIMIT, resource->amount);
		resource->amount -= gathered;
		p_unit.gather_component.carrying = gathered;
		p_unit.gather_component.gathering_resource = false;
		p_unit.order = UnitOrder::RETURN;
	}
}

void GameSimulation::update_unit_return(Unit &p_unit, double p_delta) {
	Base *base = find_base(p_unit.owner_component.owner);
	if (base == nullptr) {
		p_unit.order = UnitOrder::IDLE;
		return;
	}

	if (distance_to(p_unit.transform_component.position, base->transform_component.position) > RETURN_DISTANCE) {
		p_unit.transform_component.position = move_toward(p_unit.transform_component.position, base->transform_component.position, unit_speed(p_unit) * static_cast<float>(p_delta));
		return;
	}

	if (p_unit.owner_component.owner == PLAYER) {
		player_essence += p_unit.gather_component.carrying;
	} else {
		bot_essence += p_unit.gather_component.carrying;
	}
	p_unit.gather_component.carrying = 0;
	p_unit.gather_component.gathering_resource = false;
	p_unit.order = UnitOrder::GATHER;
}

void GameSimulation::update_unit_attack(Unit &p_unit, double p_delta) {
	Unit *enemy_unit = find_unit(p_unit.combat_component.target_unit_id);
	if (enemy_unit == nullptr) {
		const int32_t nearby_enemy_id = find_enemy_unit_id(p_unit.owner_component.owner, p_unit.transform_component.position, unit_attack_range(p_unit));
		enemy_unit = find_unit(nearby_enemy_id);
		p_unit.combat_component.target_unit_id = nearby_enemy_id;
	}
	if (enemy_unit != nullptr) {
		if (distance_to(p_unit.transform_component.position, enemy_unit->transform_component.position) > unit_attack_range(p_unit)) {
			p_unit.transform_component.position = move_toward(p_unit.transform_component.position, enemy_unit->transform_component.position, unit_speed(p_unit) * static_cast<float>(p_delta));
			return;
		}
		if (p_unit.combat_component.attack_timer <= 0.0f) {
			enemy_unit->health_component.hp -= unit_attack_damage(p_unit);
			p_unit.combat_component.attack_timer = 0.8f;
		}
		return;
	}

	if (p_unit.combat_component.target_building_id != -1) {
		Barracks *target = find_barracks_by_id(p_unit.combat_component.target_building_id);
		if (target != nullptr && target->owner_component.owner != p_unit.owner_component.owner) {
			if (distance_to(p_unit.transform_component.position, target->transform_component.position) > unit_attack_range(p_unit) + BARRACKS_RADIUS) {
				p_unit.transform_component.position = move_toward(p_unit.transform_component.position, target->transform_component.position, unit_speed(p_unit) * static_cast<float>(p_delta));
				return;
			}

			if (p_unit.combat_component.attack_timer <= 0.0f) {
				target->health_component.hp -= unit_attack_damage(p_unit);
				p_unit.combat_component.attack_timer = 0.8f;
			}
			return;
		}
	}

	if (p_unit.combat_component.target_base_owner == -1) {
		p_unit.transform_component.position = move_toward(p_unit.transform_component.position, p_unit.movement_component.target_position, unit_speed(p_unit) * static_cast<float>(p_delta));
		if (distance_to(p_unit.transform_component.position, p_unit.movement_component.target_position) <= 2.0f) {
			p_unit.order = UnitOrder::IDLE;
		}
		return;
	}

	Base *enemy_base = find_base(p_unit.combat_component.target_base_owner);
	if (enemy_base == nullptr) {
		p_unit.order = UnitOrder::IDLE;
		return;
	}

	if (distance_to(p_unit.transform_component.position, enemy_base->transform_component.position) > unit_attack_range(p_unit) + BASE_RADIUS) {
		p_unit.transform_component.position = move_toward(p_unit.transform_component.position, p_unit.movement_component.target_position, unit_speed(p_unit) * static_cast<float>(p_delta));
		return;
	}

	if (p_unit.combat_component.attack_timer <= 0.0f) {
		enemy_base->health_component.hp -= unit_attack_damage(p_unit);
		p_unit.combat_component.attack_timer = 0.8f;
	}
}

void GameSimulation::update_unit(Unit &p_unit, double p_delta) {
	update_unit_timers(p_unit, p_delta);
	update_unit_auto_aggro(p_unit);

	if (p_unit.order == UnitOrder::BUILD && p_unit.type == UnitType::WORKER) {
		update_unit_build(p_unit, p_delta);
		return;
	}

	if (p_unit.order == UnitOrder::MOVE) {
		update_unit_move(p_unit, p_delta);
		return;
	}

	if (p_unit.order == UnitOrder::GATHER && p_unit.type == UnitType::WORKER) {
		update_unit_gather(p_unit, p_delta);
		return;
	}

	if (p_unit.order == UnitOrder::RETURN && p_unit.type == UnitType::WORKER) {
		update_unit_return(p_unit, p_delta);
		return;
	}

	if (p_unit.order == UnitOrder::ATTACK) {
		update_unit_attack(p_unit, p_delta);
	}
}

} // namespace tinyv1

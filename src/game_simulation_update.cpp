#include "game_simulation.hpp"

#include "game_constants.hpp"

#include <algorithm>

namespace tinyv1 {

void GameSimulation::update_units(double p_delta) {
	for (Unit &unit : units) {
		update_unit(unit, p_delta);
	}
}

void GameSimulation::update_unit(Unit &p_unit, double p_delta) {
	p_unit.attack_timer = std::max(0.0f, p_unit.attack_timer - static_cast<float>(p_delta));
	p_unit.gather_timer = std::max(0.0f, p_unit.gather_timer - static_cast<float>(p_delta));

	if (p_unit.type == UnitType::FIGHTER && p_unit.order != UnitOrder::ATTACK) {
		const int32_t enemy_id = find_enemy_unit_id(p_unit.owner, p_unit.position, 140.0f);
		if (enemy_id != -1) {
			p_unit.order = UnitOrder::ATTACK;
			p_unit.target_unit_id = enemy_id;
			p_unit.target_base_owner = -1;
			p_unit.target_building_id = -1;
		}
	}

	if (p_unit.order == UnitOrder::BUILD && p_unit.type == UnitType::WORKER) {
		Barracks *building = find_barracks_by_id(p_unit.target_building_id);
		if (building == nullptr) {
			p_unit.order = UnitOrder::IDLE;
			return;
		}

		if (building->completed) {
			p_unit.order = UnitOrder::IDLE;
			return;
		}

		if (distance_to(p_unit.position, building->position) > BUILD_DISTANCE) {
			p_unit.position = move_toward(p_unit.position, building->position, unit_speed(p_unit) * static_cast<float>(p_delta));
			return;
		}

		building->build_progress = std::min(1.0f, building->build_progress + static_cast<float>(p_delta) / BARRACKS_BUILD_TIME);
		if (building->build_progress >= 1.0f) {
			building->completed = true;
			p_unit.order = UnitOrder::IDLE;
		}
		return;
	}

	if (p_unit.order == UnitOrder::MOVE) {
		p_unit.position = move_toward(p_unit.position, p_unit.target_position, unit_speed(p_unit) * static_cast<float>(p_delta));
		if (distance_to(p_unit.position, p_unit.target_position) <= 2.0f) {
			p_unit.order = UnitOrder::IDLE;
		}
		return;
	}

	if (p_unit.order == UnitOrder::GATHER && p_unit.type == UnitType::WORKER) {
		ResourceNode *resource = find_resource(p_unit.target_resource);
		if (resource == nullptr || resource->amount <= 0) {
			p_unit.gathering_resource = false;
			p_unit.order = UnitOrder::IDLE;
			return;
		}

		if (distance_to(p_unit.position, resource->position) > GATHER_DISTANCE) {
			p_unit.gathering_resource = false;
			p_unit.position = move_toward(p_unit.position, resource->position, unit_speed(p_unit) * static_cast<float>(p_delta));
			return;
		}

		if (!p_unit.gathering_resource) {
			p_unit.gathering_resource = true;
			p_unit.gather_timer = GATHER_TIME;
			return;
		}

		if (p_unit.gather_timer <= 0.0f) {
			const int32_t gathered = std::min(WORKER_CARRY_LIMIT, resource->amount);
			resource->amount -= gathered;
			p_unit.carrying = gathered;
			p_unit.gathering_resource = false;
			p_unit.order = UnitOrder::RETURN;
		}
		return;
	}

	if (p_unit.order == UnitOrder::RETURN && p_unit.type == UnitType::WORKER) {
		Base *base = find_base(p_unit.owner);
		if (base == nullptr) {
			p_unit.order = UnitOrder::IDLE;
			return;
		}

		if (distance_to(p_unit.position, base->position) > RETURN_DISTANCE) {
			p_unit.position = move_toward(p_unit.position, base->position, unit_speed(p_unit) * static_cast<float>(p_delta));
			return;
		}

		if (p_unit.owner == PLAYER) {
			player_essence += p_unit.carrying;
		} else {
			bot_essence += p_unit.carrying;
		}
		p_unit.carrying = 0;
		p_unit.gathering_resource = false;
		p_unit.order = UnitOrder::GATHER;
		return;
	}

	if (p_unit.order == UnitOrder::ATTACK) {
		Unit *enemy_unit = find_unit(p_unit.target_unit_id);
		if (enemy_unit == nullptr) {
			const int32_t nearby_enemy_id = find_enemy_unit_id(p_unit.owner, p_unit.position, unit_attack_range(p_unit));
			enemy_unit = find_unit(nearby_enemy_id);
			p_unit.target_unit_id = nearby_enemy_id;
		}
		if (enemy_unit != nullptr) {
			if (distance_to(p_unit.position, enemy_unit->position) > unit_attack_range(p_unit)) {
				p_unit.position = move_toward(p_unit.position, enemy_unit->position, unit_speed(p_unit) * static_cast<float>(p_delta));
				return;
			}
			if (p_unit.attack_timer <= 0.0f) {
				enemy_unit->hp -= unit_attack_damage(p_unit);
				p_unit.attack_timer = 0.8f;
			}
			return;
		}

		if (p_unit.target_building_id != -1) {
			Barracks *target = find_barracks_by_id(p_unit.target_building_id);
			if (target != nullptr && target->owner != p_unit.owner) {
				if (distance_to(p_unit.position, target->position) > unit_attack_range(p_unit) + BARRACKS_RADIUS) {
					p_unit.position = move_toward(p_unit.position, target->position, unit_speed(p_unit) * static_cast<float>(p_delta));
					return;
				}

				if (p_unit.attack_timer <= 0.0f) {
					target->hp -= unit_attack_damage(p_unit);
					p_unit.attack_timer = 0.8f;
				}
				return;
			}
		}

		if (p_unit.target_base_owner == -1) {
			p_unit.position = move_toward(p_unit.position, p_unit.target_position, unit_speed(p_unit) * static_cast<float>(p_delta));
			if (distance_to(p_unit.position, p_unit.target_position) <= 2.0f) {
				p_unit.order = UnitOrder::IDLE;
			}
			return;
		}

		Base *enemy_base = find_base(p_unit.target_base_owner);
		if (enemy_base == nullptr) {
			p_unit.order = UnitOrder::IDLE;
			return;
		}

		if (distance_to(p_unit.position, enemy_base->position) > unit_attack_range(p_unit) + BASE_RADIUS) {
			p_unit.position = move_toward(p_unit.position, p_unit.target_position, unit_speed(p_unit) * static_cast<float>(p_delta));
			return;
		}

		if (p_unit.attack_timer <= 0.0f) {
			enemy_base->hp -= unit_attack_damage(p_unit);
			p_unit.attack_timer = 0.8f;
		}
	}
}

} // namespace tinyv1

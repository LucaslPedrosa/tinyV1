#include "game_simulation.hpp"

#include "game_constants.hpp"

#include <algorithm>
#include <cmath>

namespace tinyv1 {

namespace {

constexpr float STATIC_OBSTACLE_MARGIN = 6.0f;
constexpr float STATIC_AVOIDANCE_WEIGHT = 1.6f;

const GatherRule *find_gather_rule(const GatherComponent &p_gather_component, ResourceType p_resource_type) {
	for (const GatherRule &rule : p_gather_component.rules) {
		if (rule.resource_type == p_resource_type) {
			return &rule;
		}
	}
	return nullptr;
}

godot::Vector2 safe_normalized(const godot::Vector2 &p_vector) {
	if (p_vector.length() <= 0.001f) {
		return godot::Vector2();
	}
	return p_vector.normalized();
}

float unit_collision_radius(const Unit &p_unit) {
	return p_unit.type == UnitType::WORKER ? 5.0f : 7.0f;
}

godot::Vector2 avoidance_from_circle(const godot::Vector2 &p_position, const godot::Vector2 &p_obstacle_position, float p_avoid_radius) {
	const godot::Vector2 away = p_position - p_obstacle_position;
	const float distance_squared = away.x * away.x + away.y * away.y;
	if (distance_squared <= 0.000001f) {
		return godot::Vector2(1.0f, 0.0f);
	}
	if (distance_squared >= p_avoid_radius * p_avoid_radius) {
		return godot::Vector2();
	}

	const float distance = std::sqrt(distance_squared);
	const float strength = (p_avoid_radius - distance) / p_avoid_radius;
	return away / distance * strength;
}

} // namespace

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
	(void)p_unit;
	// Melee units should not break formation or override explicit orders just
	// because an enemy walks nearby. Attack-move handles opportunistic targets
	// inside update_unit_attack when no explicit target is assigned.
}

void GameSimulation::move_unit_toward_with_avoidance(Unit &p_unit, const godot::Vector2 &p_target, float p_max_distance, ResourceId p_ignored_resource, BuildingId p_ignored_building, PlayerId p_ignored_base_owner) {
	const godot::Vector2 position = p_unit.object.transform_component.position;
	godot::Vector2 desired = safe_normalized(p_target - position);
	godot::Vector2 avoidance;
	const float unit_radius_value = unit_collision_radius(p_unit);

	for (const ResourceNode &resource : resources) {
		if (resource.id == p_ignored_resource || resource.amount <= 0) {
			continue;
		}
		avoidance += avoidance_from_circle(position, resource.position, RESOURCE_RADIUS + unit_radius_value + STATIC_OBSTACLE_MARGIN) * STATIC_AVOIDANCE_WEIGHT;
	}

	for (const Base &base : bases) {
		if (base.owner_component.owner == p_ignored_base_owner) {
			continue;
		}
		avoidance += avoidance_from_circle(position, base.transform_component.position, BASE_RADIUS + unit_radius_value + STATIC_OBSTACLE_MARGIN) * STATIC_AVOIDANCE_WEIGHT;
	}

	for (const Barracks &building : barracks) {
		if (building.id == p_ignored_building) {
			continue;
		}
		avoidance += avoidance_from_circle(position, building.transform_component.position, BARRACKS_RADIUS + unit_radius_value + STATIC_OBSTACLE_MARGIN) * STATIC_AVOIDANCE_WEIGHT;
	}

	godot::Vector2 final_direction = safe_normalized(desired + avoidance);
	if (final_direction.length() <= 0.001f) {
		final_direction = desired;
	}

	godot::Vector2 next_position = position + final_direction * p_max_distance;
	if (distance_to(position, p_target) <= p_max_distance) {
		next_position = p_target;
	}
	p_unit.object.transform_component.position = next_position;
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

	if (distance_to(p_unit.object.transform_component.position, building->transform_component.position) > BUILD_DISTANCE) {
		move_unit_toward_with_avoidance(p_unit, building->transform_component.position, unit_speed(p_unit) * static_cast<float>(p_delta), -1, building->id);
		return;
	}

	building->construction_component.build_progress = std::min(1.0f, building->construction_component.build_progress + static_cast<float>(p_delta) / BARRACKS_BUILD_TIME);
	if (building->construction_component.build_progress >= 1.0f) {
		building->construction_component.completed = true;
		p_unit.order = UnitOrder::IDLE;
	}
}

void GameSimulation::update_unit_move(Unit &p_unit, double p_delta) {
	move_unit_toward_with_avoidance(p_unit, p_unit.movement_component.target_position, unit_speed(p_unit) * static_cast<float>(p_delta));
	if (distance_to(p_unit.object.transform_component.position, p_unit.movement_component.target_position) <= 2.0f) {
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
	const GatherRule *gather_rule = find_gather_rule(p_unit.gather_component, resource->type);
	if (gather_rule == nullptr) {
		p_unit.gather_component.gathering_resource = false;
		p_unit.order = UnitOrder::IDLE;
		return;
	}

	if (distance_to(p_unit.object.transform_component.position, resource->position) > GATHER_DISTANCE) {
		p_unit.gather_component.gathering_resource = false;
		move_unit_toward_with_avoidance(p_unit, resource->position, unit_speed(p_unit) * static_cast<float>(p_delta), resource->id);
		return;
	}

	if (!p_unit.gather_component.gathering_resource) {
		p_unit.gather_component.gathering_resource = true;
		p_unit.gather_component.gather_timer = gather_rule->gather_time;
		return;
	}

	if (p_unit.gather_component.gather_timer <= 0.0f) {
		const int32_t gathered = std::min(gather_rule->amount_per_trip, resource->amount);
		resource->amount -= gathered;
		p_unit.gather_component.carrying = gathered;
		p_unit.gather_component.carrying_resource_type = resource->type;
		p_unit.gather_component.gathering_resource = false;
		p_unit.order = UnitOrder::RETURN;
	}
}

void GameSimulation::update_unit_return(Unit &p_unit, double p_delta) {
	Base *base = find_base(p_unit.object.owner_component.owner);
	if (base == nullptr) {
		p_unit.order = UnitOrder::IDLE;
		return;
	}

	if (distance_to(p_unit.object.transform_component.position, base->transform_component.position) > RETURN_DISTANCE) {
		move_unit_toward_with_avoidance(p_unit, base->transform_component.position, unit_speed(p_unit) * static_cast<float>(p_delta), -1, -1, base->owner_component.owner);
		return;
	}

	if (p_unit.object.owner_component.owner == PLAYER) {
		player_essence += p_unit.gather_component.carrying;
	} else {
		bot_essence += p_unit.gather_component.carrying;
	}
	p_unit.gather_component.carrying = 0;
	p_unit.gather_component.gathering_resource = false;
	p_unit.order = UnitOrder::GATHER;
}

void GameSimulation::update_unit_attack(Unit &p_unit, double p_delta) {
	const bool has_explicit_structure_target = p_unit.combat_component.target_base_owner != -1 || p_unit.combat_component.target_building_id != -1;
	Unit *enemy_unit = find_unit(p_unit.combat_component.target_unit_id);
	if (enemy_unit == nullptr && !has_explicit_structure_target) {
		const int32_t nearby_enemy_id = find_enemy_unit_id(p_unit.object.owner_component.owner, p_unit.object.transform_component.position, unit_attack_range(p_unit));
		enemy_unit = find_unit(nearby_enemy_id);
		p_unit.combat_component.target_unit_id = nearby_enemy_id;
	}
	if (enemy_unit != nullptr) {
		if (distance_to(p_unit.object.transform_component.position, enemy_unit->object.transform_component.position) > unit_attack_range(p_unit)) {
			move_unit_toward_with_avoidance(p_unit, enemy_unit->object.transform_component.position, unit_speed(p_unit) * static_cast<float>(p_delta));
			return;
		}
		if (p_unit.combat_component.attack_timer <= 0.0f) {
			enemy_unit->object.health_component.hp -= unit_attack_damage(p_unit);
			p_unit.combat_component.attack_timer = 0.8f;
		}
		return;
	}

	if (p_unit.combat_component.target_building_id != -1) {
		Barracks *target = find_barracks_by_id(p_unit.combat_component.target_building_id);
		if (target != nullptr && target->owner_component.owner != p_unit.object.owner_component.owner) {
			if (distance_to(p_unit.object.transform_component.position, target->transform_component.position) > unit_attack_range(p_unit) + BARRACKS_RADIUS) {
				move_unit_toward_with_avoidance(p_unit, target->transform_component.position, unit_speed(p_unit) * static_cast<float>(p_delta), -1, target->id);
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
		move_unit_toward_with_avoidance(p_unit, p_unit.movement_component.target_position, unit_speed(p_unit) * static_cast<float>(p_delta));
		if (distance_to(p_unit.object.transform_component.position, p_unit.movement_component.target_position) <= 2.0f) {
			p_unit.order = UnitOrder::IDLE;
		}
		return;
	}

	Base *enemy_base = find_base(p_unit.combat_component.target_base_owner);
	if (enemy_base == nullptr) {
		p_unit.order = UnitOrder::IDLE;
		return;
	}

	if (distance_to(p_unit.object.transform_component.position, enemy_base->transform_component.position) > unit_attack_range(p_unit) + BASE_RADIUS) {
		move_unit_toward_with_avoidance(p_unit, enemy_base->transform_component.position, unit_speed(p_unit) * static_cast<float>(p_delta), -1, -1, enemy_base->owner_component.owner);
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

	if (p_unit.order == UnitOrder::GATHER) {
		update_unit_gather(p_unit, p_delta);
		return;
	}

	if (p_unit.order == UnitOrder::RETURN) {
		update_unit_return(p_unit, p_delta);
		return;
	}

	if (p_unit.order == UnitOrder::ATTACK) {
		update_unit_attack(p_unit, p_delta);
	}
}

} // namespace tinyv1

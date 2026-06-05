#include "game_simulation.hpp"

#include "game_constants.hpp"

using namespace godot;

namespace tinyv1 {

Unit *GameSimulation::find_unit(int32_t p_id) {
	if (p_id == -1) {
		return nullptr;
	}
	for (Unit &unit : units) {
		if (unit.id == p_id) {
			return &unit;
		}
	}
	return nullptr;
}

Unit *GameSimulation::find_player_unit_at(const Vector2 &p_position) {
	float best_distance = 999999.0f;
	Unit *best_unit = nullptr;
	for (Unit &unit : units) {
		if (unit.owner != PLAYER) {
			continue;
		}

		const float distance = distance_to(p_position, unit.position);
		if (distance <= unit_radius(unit) + 10.0f && distance < best_distance) {
			best_distance = distance;
			best_unit = &unit;
		}
	}
	return best_unit;
}

ResourceNode *GameSimulation::find_resource(int32_t p_id) {
	for (ResourceNode &resource : resources) {
		if (resource.id == p_id) {
			return &resource;
		}
	}
	return nullptr;
}

Base *GameSimulation::find_base(int32_t p_owner) {
	for (Base &base : bases) {
		if (base.owner == p_owner) {
			return &base;
		}
	}
	return nullptr;
}

Barracks *GameSimulation::find_barracks(int32_t p_owner) {
	for (Barracks &building : barracks) {
		if (building.owner == p_owner) {
			return &building;
		}
	}
	return nullptr;
}

Barracks *GameSimulation::find_barracks_by_id(BuildingId p_id) {
	if (p_id == -1) {
		return nullptr;
	}
	for (Barracks &building : barracks) {
		if (building.id == p_id) {
			return &building;
		}
	}
	return nullptr;
}

const Barracks *GameSimulation::find_barracks_by_id(BuildingId p_id) const {
	if (p_id == -1) {
		return nullptr;
	}
	for (const Barracks &building : barracks) {
		if (building.id == p_id) {
			return &building;
		}
	}
	return nullptr;
}

Barracks *GameSimulation::find_completed_barracks(int32_t p_owner) {
	for (Barracks &building : barracks) {
		if (building.owner == p_owner && building.completed) {
			return &building;
		}
	}
	return nullptr;
}

int32_t GameSimulation::get_essence(PlayerId p_owner) const {
	return p_owner == PLAYER ? player_essence : bot_essence;
}

int32_t GameSimulation::count_units(PlayerId p_owner, UnitType p_type) const {
	int32_t count = 0;
	for (const Unit &unit : units) {
		if (unit.owner == p_owner && unit.type == p_type) {
			count++;
		}
	}
	return count;
}

std::vector<UnitId> GameSimulation::get_unit_ids(PlayerId p_owner, UnitType p_type) const {
	std::vector<UnitId> result;
	for (const Unit &unit : units) {
		if (unit.owner == p_owner && unit.type == p_type) {
			result.push_back(unit.id);
		}
	}
	return result;
}

std::vector<UnitId> GameSimulation::get_idle_worker_ids(PlayerId p_owner) const {
	std::vector<UnitId> result;
	for (const Unit &unit : units) {
		if (unit.owner == p_owner && unit.type == UnitType::WORKER && unit.order == UnitOrder::IDLE) {
			result.push_back(unit.id);
		}
	}
	return result;
}

bool GameSimulation::get_unit_position(UnitId p_id, Vector2 &r_position) const {
	for (const Unit &unit : units) {
		if (unit.id == p_id) {
			r_position = unit.position;
			return true;
		}
	}
	return false;
}

bool GameSimulation::get_resource_position(ResourceId p_id, Vector2 &r_position) const {
	for (const ResourceNode &resource : resources) {
		if (resource.id == p_id && resource.amount > 0) {
			r_position = resource.position;
			return true;
		}
	}
	return false;
}

bool GameSimulation::get_base_position(PlayerId p_owner, Vector2 &r_position) const {
	for (const Base &base : bases) {
		if (base.owner == p_owner) {
			r_position = base.position;
			return true;
		}
	}
	return false;
}

bool GameSimulation::has_barracks(PlayerId p_owner) const {
	for (const Barracks &building : barracks) {
		if (building.owner == p_owner) {
			return true;
		}
	}
	return false;
}

bool GameSimulation::has_completed_barracks(PlayerId p_owner) const {
	for (const Barracks &building : barracks) {
		if (building.owner == p_owner && building.completed) {
			return true;
		}
	}
	return false;
}

UnitId GameSimulation::find_available_worker_id(PlayerId p_owner) const {
	UnitId fallback_worker_id = -1;
	for (const Unit &unit : units) {
		if (unit.owner != p_owner || unit.type != UnitType::WORKER) {
			continue;
		}

		if (unit.order == UnitOrder::IDLE) {
			return unit.id;
		}

		if (fallback_worker_id == -1 && (unit.order == UnitOrder::GATHER || unit.order == UnitOrder::RETURN || unit.order == UnitOrder::MOVE)) {
			fallback_worker_id = unit.id;
		}
	}
	return fallback_worker_id;
}

ResourceId GameSimulation::find_nearest_resource_id(const Vector2 &p_position) const {
	return find_nearest_resource(p_position);
}

BuildingId GameSimulation::find_player_barracks_id_at(const Vector2 &p_position) const {
	for (const Barracks &building : barracks) {
		if (building.owner == PLAYER && distance_to(p_position, building.position) <= BARRACKS_RADIUS + 18.0f) {
			return building.id;
		}
	}
	return -1;
}

BuildingId GameSimulation::find_enemy_barracks_id_at(int32_t p_owner, const Vector2 &p_position) const {
	for (const Barracks &building : barracks) {
		if (building.owner != p_owner && distance_to(p_position, building.position) <= BARRACKS_RADIUS + 18.0f) {
			return building.id;
		}
	}
	return -1;
}

int32_t GameSimulation::find_enemy_unit_id_at(int32_t p_owner, const Vector2 &p_position) const {
	float best_distance = 999999.0f;
	int32_t best_id = -1;
	for (const Unit &unit : units) {
		if (unit.owner == p_owner || unit.hp <= 0.0f) {
			continue;
		}
		const float distance = distance_to(p_position, unit.position);
		if (distance <= unit_radius(unit) + 10.0f && distance < best_distance) {
			best_distance = distance;
			best_id = unit.id;
		}
	}
	return best_id;
}

Unit *GameSimulation::find_available_worker(int32_t p_owner) {
	Unit *fallback_worker = nullptr;
	for (Unit &unit : units) {
		if (unit.owner != p_owner || unit.type != UnitType::WORKER) {
			continue;
		}

		if (unit.order == UnitOrder::IDLE) {
			return &unit;
		}

		if (fallback_worker == nullptr && (unit.order == UnitOrder::GATHER || unit.order == UnitOrder::RETURN || unit.order == UnitOrder::MOVE)) {
			fallback_worker = &unit;
		}
	}
	return fallback_worker;
}

int32_t GameSimulation::find_nearest_resource(const Vector2 &p_position) const {
	float best_distance = 999999.0f;
	int32_t best_id = -1;
	for (const ResourceNode &resource : resources) {
		if (resource.amount <= 0) {
			continue;
		}
		const float distance = distance_to(p_position, resource.position);
		if (distance < best_distance) {
			best_distance = distance;
			best_id = resource.id;
		}
	}
	return best_id;
}

int32_t GameSimulation::find_enemy_unit_id(int32_t p_owner, const Vector2 &p_position, float p_range) const {
	float best_distance = 999999.0f;
	int32_t best_id = -1;
	for (const Unit &unit : units) {
		if (unit.owner == p_owner || unit.hp <= 0.0f) {
			continue;
		}
		const float distance = distance_to(p_position, unit.position);
		if (distance <= p_range && distance < best_distance) {
			best_distance = distance;
			best_id = unit.id;
		}
	}
	return best_id;
}

} // namespace tinyv1

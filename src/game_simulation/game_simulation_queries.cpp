#include "game_simulation.hpp"

#include "game_constants.hpp"

using namespace godot;

namespace tinyv1 {

Unit *GameSimulation::find_unit(int32_t p_id) {
	if (p_id == -1) {
		return nullptr;
	}
	for (Unit &unit : units) {
		if (unit.object.id == p_id) {
			return &unit;
		}
	}
	return nullptr;
}

const Unit *GameSimulation::find_unit(int32_t p_id) const {
	if (p_id == -1) {
		return nullptr;
	}
	for (const Unit &unit : units) {
		if (unit.object.id == p_id) {
			return &unit;
		}
	}
	return nullptr;
}

Unit *GameSimulation::find_player_unit_at(const Vector2 &p_position) {
	float best_distance = 999999.0f;
	Unit *best_unit = nullptr;
	for (Unit &unit : units) {
		if (unit.object.owner_component.owner != PLAYER) {
			continue;
		}

		const float distance = distance_to(p_position, unit.object.transform_component.position);
		if (distance <= unit_radius(unit) + 10.0f && distance < best_distance) {
			best_distance = distance;
			best_unit = &unit;
		}
	}
	return best_unit;
}

UnitId GameSimulation::find_player_unit_id_at(const Vector2 &p_position) const {
	float best_distance = 999999.0f;
	UnitId best_id = -1;
	for (const Unit &unit : units) {
		if (unit.object.owner_component.owner != PLAYER) {
			continue;
		}

		const float distance = distance_to(p_position, unit.object.transform_component.position);
		if (distance <= unit_radius(unit) + 10.0f && distance < best_distance) {
			best_distance = distance;
			best_id = unit.object.id;
		}
	}
	return best_id;
}

ResourceNode *GameSimulation::find_resource(int32_t p_id) {
	for (ResourceNode &resource : resources) {
		if (resource.id == p_id) {
			return &resource;
		}
	}
	return nullptr;
}

const ResourceNode *GameSimulation::find_resource(int32_t p_id) const {
	for (const ResourceNode &resource : resources) {
		if (resource.id == p_id) {
			return &resource;
		}
	}
	return nullptr;
}

Base *GameSimulation::find_base(int32_t p_owner) {
	for (Base &base : bases) {
		if (base.owner_component.owner == p_owner) {
			return &base;
		}
	}
	return nullptr;
}

const Base *GameSimulation::find_base(int32_t p_owner) const {
	for (const Base &base : bases) {
		if (base.owner_component.owner == p_owner) {
			return &base;
		}
	}
	return nullptr;
}

Barracks *GameSimulation::find_barracks(int32_t p_owner) {
	for (Barracks &building : barracks) {
		if (building.owner_component.owner == p_owner) {
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
		if (building.owner_component.owner == p_owner && building.construction_component.completed) {
			return &building;
		}
	}
	return nullptr;
}

const Rect2 &GameSimulation::get_map_rect() const {
	return map_rect;
}

const String &GameSimulation::get_winner_text() const {
	return winner_text;
}

int32_t GameSimulation::get_essence(PlayerId p_owner) const {
	return p_owner == PLAYER ? player_essence : bot_essence;
}

std::vector<ResourceSummary> GameSimulation::get_render_resources() const {
	std::vector<ResourceSummary> result;
	for (const ResourceNode &resource : resources) {
		result.push_back({ resource.id, resource.position, resource.amount });
	}
	return result;
}

std::vector<BaseSummary> GameSimulation::get_render_bases() const {
	std::vector<BaseSummary> result;
	for (const Base &base : bases) {
		BaseSummary summary;
		summary.owner = base.owner_component.owner;
		summary.position = base.transform_component.position;
		summary.hp = base.health_component.hp;
		summary.max_hp = 400.0f;
		summary.train_timer = base.production_component.train_timer;
		summary.train_duration = base.production_component.train_duration;
		summary.training_worker = base.production_component.training;
		summary.worker_queue = base.production_component.queue;
		summary.has_rally_point = base.rally_component.has_rally_action;
		summary.rally_point = base.rally_component.position;
		result.push_back(summary);
	}
	return result;
}

std::vector<BuildingSummary> GameSimulation::get_render_buildings() const {
	std::vector<BuildingSummary> result;
	for (const Barracks &building : barracks) {
		BuildingSummary summary;
		summary.id = building.id;
		summary.owner = building.owner_component.owner;
		summary.position = building.transform_component.position;
		summary.hp = building.health_component.hp;
		summary.max_hp = 250.0f;
		summary.build_progress = building.construction_component.build_progress;
		summary.completed = building.construction_component.completed;
		summary.train_timer = building.production_component.train_timer;
		summary.train_duration = building.production_component.train_duration;
		summary.training_fighter = building.production_component.training;
		summary.fighter_queue = building.production_component.queue;
		summary.has_rally_point = building.rally_component.has_rally_action;
		summary.rally_point = building.rally_component.position;
		result.push_back(summary);
	}
	return result;
}

std::vector<UnitSummary> GameSimulation::get_render_units() const {
	std::vector<UnitSummary> result;
	for (const Unit &unit : units) {
		UnitSummary summary;
		summary.id = unit.object.id;
		summary.owner = unit.object.owner_component.owner;
		summary.type = unit.type;
		summary.order = unit.order;
		summary.position = unit.object.transform_component.position;
		summary.hp = unit.object.health_component.hp;
		summary.max_hp = unit.type == UnitType::WORKER ? 45.0f : 80.0f;
		summary.moving = unit.order == UnitOrder::MOVE || unit.order == UnitOrder::RETURN || unit.order == UnitOrder::ATTACK || unit.order == UnitOrder::BUILD || (unit.order == UnitOrder::GATHER && !unit.gather_component.gathering_resource);
		result.push_back(summary);
	}
	return result;
}

int32_t GameSimulation::count_units(PlayerId p_owner, UnitType p_type) const {
	int32_t count = 0;
	for (const Unit &unit : units) {
		if (unit.object.owner_component.owner == p_owner && unit.type == p_type) {
			count++;
		}
	}
	return count;
}

std::vector<UnitId> GameSimulation::get_unit_ids(PlayerId p_owner, UnitType p_type) const {
	std::vector<UnitId> result;
	for (const Unit &unit : units) {
		if (unit.object.owner_component.owner == p_owner && unit.type == p_type) {
			result.push_back(unit.object.id);
		}
	}
	return result;
}

std::vector<UnitId> GameSimulation::get_idle_worker_ids(PlayerId p_owner) const {
	std::vector<UnitId> result;
	for (const Unit &unit : units) {
		if (unit.object.owner_component.owner == p_owner && unit.type == UnitType::WORKER && unit.order == UnitOrder::IDLE) {
			result.push_back(unit.object.id);
		}
	}
	return result;
}

bool GameSimulation::get_unit_position(UnitId p_id, Vector2 &r_position) const {
	for (const Unit &unit : units) {
		if (unit.object.id == p_id) {
			 r_position = unit.object.transform_component.position;
			return true;
		}
	}
	return false;
}

bool GameSimulation::get_unit_summary(UnitId p_id, UnitSummary &r_summary) const {
	for (const Unit &unit : units) {
		if (unit.object.id == p_id) {
			r_summary.id = unit.object.id;
			 r_summary.owner = unit.object.owner_component.owner;
			r_summary.type = unit.type;
			r_summary.order = unit.order;
			r_summary.position = unit.object.transform_component.position;
			r_summary.hp = unit.object.health_component.hp;
			r_summary.max_hp = unit.type == UnitType::WORKER ? 45.0f : 80.0f;
			r_summary.moving = unit.order == UnitOrder::MOVE || unit.order == UnitOrder::RETURN || unit.order == UnitOrder::ATTACK || unit.order == UnitOrder::BUILD || (unit.order == UnitOrder::GATHER && !unit.gather_component.gathering_resource);
			return true;
		}
	}
	return false;
}

bool GameSimulation::get_base_summary(PlayerId p_owner, BaseSummary &r_summary) const {
	for (const Base &base : bases) {
		if (base.owner_component.owner == p_owner) {
			r_summary.owner = base.owner_component.owner;
			r_summary.position = base.transform_component.position;
			r_summary.hp = base.health_component.hp;
			r_summary.max_hp = 400.0f;
			r_summary.train_timer = base.production_component.train_timer;
			r_summary.train_duration = base.production_component.train_duration;
			r_summary.training_worker = base.production_component.training;
			r_summary.worker_queue = base.production_component.queue;
			r_summary.has_rally_point = base.rally_component.has_rally_action;
			r_summary.rally_point = base.rally_component.position;
			return true;
		}
	}
	return false;
}

bool GameSimulation::get_building_summary(BuildingId p_id, BuildingSummary &r_summary) const {
	for (const Barracks &building : barracks) {
		if (building.id == p_id) {
			r_summary.id = building.id;
			r_summary.owner = building.owner_component.owner;
			r_summary.position = building.transform_component.position;
			r_summary.hp = building.health_component.hp;
			r_summary.max_hp = 250.0f;
			r_summary.build_progress = building.construction_component.build_progress;
			r_summary.completed = building.construction_component.completed;
			r_summary.train_timer = building.production_component.train_timer;
			r_summary.train_duration = building.production_component.train_duration;
			r_summary.training_fighter = building.production_component.training;
			r_summary.fighter_queue = building.production_component.queue;
			r_summary.has_rally_point = building.rally_component.has_rally_action;
			r_summary.rally_point = building.rally_component.position;
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
		if (base.owner_component.owner == p_owner) {
			r_position = base.transform_component.position;
			return true;
		}
	}
	return false;
}

bool GameSimulation::has_barracks(PlayerId p_owner) const {
	for (const Barracks &building : barracks) {
		if (building.owner_component.owner == p_owner) {
			return true;
		}
	}
	return false;
}

bool GameSimulation::has_completed_barracks(PlayerId p_owner) const {
	for (const Barracks &building : barracks) {
		if (building.owner_component.owner == p_owner && building.construction_component.completed) {
			return true;
		}
	}
	return false;
}

bool GameSimulation::can_train_worker(PlayerId p_owner) const {
	BaseSummary base;
	return get_base_summary(p_owner, base) && base.worker_queue < MAX_TRAIN_QUEUE && get_essence(p_owner) >= WORKER_COST;
}

bool GameSimulation::can_train_fighter(PlayerId p_owner, BuildingId p_source_building_id) const {
	BuildingSummary building;
	if (get_building_summary(p_source_building_id, building) && building.owner == p_owner) {
		return building.completed && building.fighter_queue < MAX_TRAIN_QUEUE && get_essence(p_owner) >= FIGHTER_COST;
	}
	return has_completed_barracks(p_owner) && get_essence(p_owner) >= FIGHTER_COST;
}

bool GameSimulation::can_start_barracks_placement(PlayerId p_owner, UnitId p_builder_unit_id) const {
	UnitSummary builder;
	return get_unit_summary(p_builder_unit_id, builder) && builder.owner == p_owner && builder.type == UnitType::WORKER && get_essence(p_owner) >= BARRACKS_COST;
}

UnitId GameSimulation::find_available_worker_id(PlayerId p_owner) const {
	UnitId fallback_worker_id = -1;
	for (const Unit &unit : units) {
		if (unit.object.owner_component.owner != p_owner || unit.type != UnitType::WORKER) {
			continue;
		}

		if (unit.order == UnitOrder::IDLE) {
			return unit.object.id;
		}

		if (fallback_worker_id == -1 && (unit.order == UnitOrder::GATHER || unit.order == UnitOrder::RETURN || unit.order == UnitOrder::MOVE)) {
			fallback_worker_id = unit.object.id;
		}
	}
	return fallback_worker_id;
}

ResourceId GameSimulation::find_nearest_resource_id(const Vector2 &p_position) const {
	return find_nearest_resource(p_position);
}

BuildingId GameSimulation::find_player_barracks_id_at(const Vector2 &p_position) const {
	for (const Barracks &building : barracks) {
		if (building.owner_component.owner == PLAYER && distance_to(p_position, building.transform_component.position) <= BARRACKS_RADIUS + 18.0f) {
			return building.id;
		}
	}
	return -1;
}

BuildingId GameSimulation::find_enemy_barracks_id_at(int32_t p_owner, const Vector2 &p_position) const {
	for (const Barracks &building : barracks) {
		if (building.owner_component.owner != p_owner && distance_to(p_position, building.transform_component.position) <= BARRACKS_RADIUS + 18.0f) {
			return building.id;
		}
	}
	return -1;
}

int32_t GameSimulation::find_enemy_unit_id_at(int32_t p_owner, const Vector2 &p_position) const {
	float best_distance = 999999.0f;
	int32_t best_id = -1;
	for (const Unit &unit : units) {
		if (unit.object.owner_component.owner == p_owner || unit.object.health_component.hp <= 0.0f) {
			continue;
		}
		const float distance = distance_to(p_position, unit.object.transform_component.position);
		if (distance <= unit_radius(unit) + 10.0f && distance < best_distance) {
			best_distance = distance;
			best_id = unit.object.id;
		}
	}
	return best_id;
}

Unit *GameSimulation::find_available_worker(int32_t p_owner) {
	Unit *fallback_worker = nullptr;
	for (Unit &unit : units) {
		if (unit.object.owner_component.owner != p_owner || unit.type != UnitType::WORKER) {
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
		if (unit.object.owner_component.owner == p_owner || unit.object.health_component.hp <= 0.0f) {
			continue;
		}
		const float distance = distance_to(p_position, unit.object.transform_component.position);
		if (distance <= p_range && distance < best_distance) {
			best_distance = distance;
			best_id = unit.object.id;
		}
	}
	return best_id;
}

} // namespace tinyv1

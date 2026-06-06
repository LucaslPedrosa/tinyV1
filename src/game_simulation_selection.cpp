#include "game_simulation.hpp"

#include "game_constants.hpp"

using namespace godot;

namespace tinyv1 {

SelectionResult GameSimulation::select_at(int32_t p_owner, const Vector2 &p_position) const {
	SelectionResult selection;
	float best_distance = 999999.0f;
	const Unit *best_unit = nullptr;
	for (const Unit &unit : units) {
		if (unit.owner_component.owner != p_owner) {
			continue;
		}
		const float distance = distance_to(p_position, unit.transform_component.position);
		if (distance <= unit_radius(unit) + 10.0f && distance < best_distance) {
			best_distance = distance;
			best_unit = &unit;
		}
	}

	if (best_unit != nullptr) {
		selection.unit_ids.push_back(best_unit->id);
		return selection;
	}

	for (const Base &base : bases) {
		if (base.owner_component.owner == p_owner && distance_to(p_position, base.transform_component.position) <= BASE_RADIUS + 18.0f) {
			selection.base_owner = p_owner;
			return selection;
		}
	}

	for (int32_t i = 0; i < static_cast<int32_t>(barracks.size()); ++i) {
		if (barracks[i].owner_component.owner == p_owner && distance_to(p_position, barracks[i].transform_component.position) <= BARRACKS_RADIUS + 18.0f) {
			selection.building_id = barracks[i].id;
			return selection;
		}
	}
	return selection;
}

SelectionResult GameSimulation::select_units_in_rect(int32_t p_owner, const Rect2 &p_rect, bool p_has_unit_type_filter, UnitType p_unit_type_filter) const {
	SelectionResult selection;
	if (!p_has_unit_type_filter) {
		for (const Unit &unit : units) {
			if (unit.owner_component.owner == p_owner && p_rect.has_point(unit.transform_component.position)) {
				p_unit_type_filter = unit.type;
				p_has_unit_type_filter = true;
				break;
			}
		}
	}
	for (const Unit &unit : units) {
		if (unit.owner_component.owner == p_owner && p_rect.has_point(unit.transform_component.position) && (!p_has_unit_type_filter || unit.type == p_unit_type_filter)) {
			selection.unit_ids.push_back(unit.id);
		}
	}
	return selection;
}

SelectionResult GameSimulation::select_all_units_of_type(int32_t p_owner, UnitType p_type) const {
	SelectionResult selection;
	for (const Unit &unit : units) {
		if (unit.owner_component.owner == p_owner && unit.type == p_type) {
			selection.unit_ids.push_back(unit.id);
		}
	}
	return selection;
}

} // namespace tinyv1

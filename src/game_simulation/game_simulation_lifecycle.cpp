#include "game_simulation.hpp"

#include "game_constants.hpp"

#include <algorithm>

namespace tinyv1 {

void GameSimulation::remove_dead_units() {
	units.erase(std::remove_if(units.begin(), units.end(), [](const Unit &unit) { return unit.object.health_component.hp <= 0.0f; }), units.end());
}

void GameSimulation::remove_destroyed_buildings() {
	for (int32_t i = static_cast<int32_t>(buildings.size()) - 1; i >= 0; --i) {
		if (buildings[i].health_component.hp > 0.0f) {
			continue;
		}

		const BuildingId removed_id = buildings[i].id;
		for (Unit &unit : units) {
			if (unit.build_component.target_building_id == removed_id) {
				unit.build_component.target_building_id = -1;
				if (unit.order == UnitOrder::BUILD) {
					unit.order = UnitOrder::IDLE;
				}
			}
			if (unit.combat_component.target_building_id == removed_id) {
				unit.combat_component.target_building_id = -1;
				if (unit.order == UnitOrder::ATTACK) {
					unit.order = UnitOrder::IDLE;
				}
			}
		}
		buildings.erase(buildings.begin() + i);
	}
}

void GameSimulation::check_win_condition() {
	Building *player_town_center = find_building(PLAYER, BuildingType::TOWN_CENTER);
	Building *bot_town_center = find_building(BOT, BuildingType::TOWN_CENTER);
	if (player_town_center == nullptr || player_town_center->health_component.hp <= 0.0f) {
		winner_text = "Bot wins";
	} else if (bot_town_center == nullptr || bot_town_center->health_component.hp <= 0.0f) {
		winner_text = "Player wins";
	}
}

} // namespace tinyv1

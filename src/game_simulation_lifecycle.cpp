#include "game_simulation.hpp"

#include "game_constants.hpp"

#include <algorithm>

namespace tinyv1 {

void GameSimulation::remove_dead_units() {
	units.erase(std::remove_if(units.begin(), units.end(), [](const Unit &unit) { return unit.hp <= 0.0f; }), units.end());
}

void GameSimulation::remove_destroyed_barracks() {
	for (int32_t i = static_cast<int32_t>(barracks.size()) - 1; i >= 0; --i) {
		if (barracks[i].hp > 0.0f) {
			continue;
		}

		const BuildingId removed_id = barracks[i].id;
		for (Unit &unit : units) {
			if (unit.target_building_id == removed_id) {
				unit.target_building_id = -1;
				if (unit.order == UnitOrder::BUILD || unit.order == UnitOrder::ATTACK) {
					unit.order = UnitOrder::IDLE;
				}
			}
		}
		barracks.erase(barracks.begin() + i);
	}
}

void GameSimulation::check_win_condition() {
	Base *player_base = find_base(PLAYER);
	Base *bot_base = find_base(BOT);
	if (player_base != nullptr && player_base->hp <= 0.0f) {
		winner_text = "Bot wins";
	} else if (bot_base != nullptr && bot_base->hp <= 0.0f) {
		winner_text = "Player wins";
	}
}

} // namespace tinyv1

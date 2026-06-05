#include "game_simulation.hpp"

#include "game_constants.hpp"

using namespace godot;

namespace tinyv1 {

void GameSimulation::reset_match() {
	map_rect = Rect2(Vector2(0, 0), Vector2(1280, 570));
	resources.clear();
	bases.clear();
	barracks.clear();
	units.clear();
	player_essence = 50;
	bot_essence = 50;
	next_unit_id = 1;
	next_building_id = 1;
	winner_text = String();

	bases.push_back({ PLAYER, Vector2(160, 316), 400.0f, 0.0f });
	bases.push_back({ BOT, Vector2(1120, 316), 400.0f, 0.0f });

	resources.push_back({ 1, Vector2(300, 210), 700 });
	resources.push_back({ 2, Vector2(300, 430), 700 });
	resources.push_back({ 3, Vector2(980, 210), 700 });
	resources.push_back({ 4, Vector2(980, 430), 700 });
	resources.push_back({ 5, Vector2(640, 316), 1200 });

	for (int32_t i = 0; i < 3; ++i) {
		units.push_back({ next_unit_id++, PLAYER, UnitType::WORKER, UnitOrder::IDLE, Vector2(210, 270 + i * 34.0f), Vector2(), 45.0f });
		units.push_back({ next_unit_id++, BOT, UnitType::WORKER, UnitOrder::GATHER, Vector2(1070, 270 + i * 34.0f), Vector2(980, 210), 45.0f, 0.0f, 0.0f, false, 0, 3 });
	}
}

} // namespace tinyv1

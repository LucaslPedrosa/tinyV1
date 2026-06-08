#include "game_simulation.hpp"

#include "game_constants.hpp"

using namespace godot;

namespace tinyv1 {

void GameSimulation::reset_match() {
	map_rect = Rect2(Vector2(0, 0), Vector2(3200, 2200));
	resources.clear();
	bases.clear();
	barracks.clear();
	units.clear();
	player_essence = 50;
	bot_essence = 50;
	next_unit_id = 1;
	next_building_id = 1;
	winner_text = String();

	Base player_base;
	player_base.owner_component.owner = PLAYER;
	player_base.transform_component.position = Vector2(420, 1100);
	player_base.health_component.hp = 400.0f;
	player_base.health_component.max_hp = 400.0f;
	player_base.production_component.queue = 0;
	player_base.production_component.training = false;
	player_base.production_component.train_timer = 0.0f;
	player_base.production_component.train_duration = 0.0f;
	bases.push_back(player_base);

	Base bot_base;
	bot_base.owner_component.owner = BOT;
	bot_base.transform_component.position = Vector2(2780, 1100);
	bot_base.health_component.hp = 400.0f;
	bot_base.health_component.max_hp = 400.0f;
	bot_base.production_component.queue = 0;
	bot_base.production_component.training = false;
	bot_base.production_component.train_timer = 0.0f;
	bot_base.production_component.train_duration = 0.0f;
	bases.push_back(bot_base);

	resources.push_back({ 1, ResourceType::ESSENCE, Vector2(700, 940), 700 });
	resources.push_back({ 2, ResourceType::ESSENCE, Vector2(700, 1260), 700 });
	resources.push_back({ 3, ResourceType::ESSENCE, Vector2(2500, 940), 700 });
	resources.push_back({ 4, ResourceType::ESSENCE, Vector2(2500, 1260), 700 });
	resources.push_back({ 5, ResourceType::ESSENCE, Vector2(1600, 1100), 1200 });

	for (int32_t i = 0; i < 3; ++i) {
		Unit player_worker = create_unit(next_unit_id++, PLAYER, UnitType::WORKER, Vector2(740, 1054 + i * 34.0f));
		player_worker.order = UnitOrder::IDLE;
		units.push_back(player_worker);

		Unit bot_worker = create_unit(next_unit_id++, BOT, UnitType::WORKER, Vector2(2460, 1054 + i * 34.0f));
		bot_worker.order = UnitOrder::GATHER;
		bot_worker.movement_component.target_position = Vector2(2500, 940);
		bot_worker.gather_component.gathering_resource = false;
		bot_worker.gather_component.carrying = 0;
		bot_worker.gather_component.target_resource = 3;
		units.push_back(bot_worker);
	}
}

} // namespace tinyv1

#pragma once

#include "game_command.hpp"
#include "game_types.hpp"

#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <vector>

namespace tinyv1 {

class GameSimulation {
public:
	godot::Rect2 map_rect;
	std::vector<ResourceNode> resources;
	std::vector<Base> bases;
	std::vector<Barracks> barracks;
	std::vector<Unit> units;
	int32_t player_essence = 50;
	int32_t bot_essence = 50;
	int32_t next_unit_id = 1;
	int32_t next_building_id = 1;
	godot::String winner_text;

	void reset_match();

	Unit *find_unit(int32_t p_id);
	Unit *find_player_unit_at(const godot::Vector2 &p_position);
	ResourceNode *find_resource(int32_t p_id);
	Base *find_base(int32_t p_owner);
	Barracks *find_barracks(int32_t p_owner);
	Barracks *find_barracks_by_id(BuildingId p_id);
	const Barracks *find_barracks_by_id(BuildingId p_id) const;
	Barracks *find_completed_barracks(int32_t p_owner);
	int32_t get_essence(PlayerId p_owner) const;
	int32_t count_units(PlayerId p_owner, UnitType p_type) const;
	std::vector<UnitId> get_unit_ids(PlayerId p_owner, UnitType p_type) const;
	std::vector<UnitId> get_idle_worker_ids(PlayerId p_owner) const;
	bool get_unit_position(UnitId p_id, godot::Vector2 &r_position) const;
	bool get_resource_position(ResourceId p_id, godot::Vector2 &r_position) const;
	bool get_base_position(PlayerId p_owner, godot::Vector2 &r_position) const;
	bool has_barracks(PlayerId p_owner) const;
	bool has_completed_barracks(PlayerId p_owner) const;
	UnitId find_available_worker_id(PlayerId p_owner) const;
	ResourceId find_nearest_resource_id(const godot::Vector2 &p_position) const;
	BuildingId find_player_barracks_id_at(const godot::Vector2 &p_position) const;
	BuildingId find_enemy_barracks_id_at(int32_t p_owner, const godot::Vector2 &p_position) const;
	int32_t find_enemy_unit_id_at(int32_t p_owner, const godot::Vector2 &p_position) const;
	Unit *find_available_worker(int32_t p_owner);
	int32_t find_nearest_resource(const godot::Vector2 &p_position) const;
	int32_t find_enemy_unit_id(int32_t p_owner, const godot::Vector2 &p_position, float p_range) const;

	void remove_dead_units();
	void remove_destroyed_barracks();
	void check_win_condition();
	void update_units(double p_delta);
	void update_unit(Unit &p_unit, double p_delta);
	void update_training(double p_delta);
	GameCommandResult apply_command(const GameCommand &p_command);
	void train_unit(int32_t p_owner, UnitType p_type, BuildingId p_source_building_id = -1);
	void spawn_unit(int32_t p_owner, UnitType p_type, const godot::Vector2 &p_source_position, bool p_has_rally_point = false, const godot::Vector2 &p_rally_point = godot::Vector2());
	BuildingId place_barracks(int32_t p_owner, const godot::Vector2 &p_position, Unit *p_builder = nullptr);
	void delete_barracks(BuildingId p_building_id);
	bool set_selected_production_rally_point(int32_t p_owner, int32_t p_selected_base_owner, BuildingId p_selected_building_id, const godot::Vector2 &p_position);
	CommandFeedback command_selected_to(int32_t p_owner, const std::vector<int32_t> &p_selected_unit_ids, int32_t p_selected_base_owner, BuildingId p_selected_building_id, const godot::Vector2 &p_position);
	SelectionResult select_at(int32_t p_owner, const godot::Vector2 &p_position) const;
	SelectionResult select_units_in_rect(int32_t p_owner, const godot::Rect2 &p_rect, bool p_has_unit_type_filter = false, UnitType p_unit_type_filter = UnitType::WORKER) const;
	SelectionResult select_all_units_of_type(int32_t p_owner, UnitType p_type) const;
};

} // namespace tinyv1

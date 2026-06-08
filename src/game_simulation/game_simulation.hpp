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
	void reset_match();

	const godot::Rect2 &get_map_rect() const;
	const godot::String &get_winner_text() const;
	int32_t get_essence(PlayerId p_owner) const;
	void build_render_snapshot(RenderSnapshot &r_snapshot) const;
	int32_t count_units(PlayerId p_owner, UnitType p_type) const;
	std::vector<UnitId> get_unit_ids(PlayerId p_owner, UnitType p_type) const;
	std::vector<UnitId> get_idle_worker_ids(PlayerId p_owner) const;
	bool get_unit_position(UnitId p_id, godot::Vector2 &r_position) const;
	bool get_unit_summary(UnitId p_id, UnitSummary &r_summary) const;
	bool get_base_summary(PlayerId p_owner, BaseSummary &r_summary) const;
	bool get_building_summary(BuildingId p_id, BuildingSummary &r_summary) const;
	bool get_resource_position(ResourceId p_id, godot::Vector2 &r_position) const;
	bool get_base_position(PlayerId p_owner, godot::Vector2 &r_position) const;
	bool has_barracks(PlayerId p_owner) const;
	bool has_completed_barracks(PlayerId p_owner) const;
	bool can_train_worker(PlayerId p_owner) const;
	bool can_train_fighter(PlayerId p_owner, BuildingId p_source_building_id) const;
	bool can_start_barracks_placement(PlayerId p_owner, UnitId p_builder_unit_id) const;
	UnitId find_available_worker_id(PlayerId p_owner) const;
	ResourceId find_nearest_resource_id(const godot::Vector2 &p_position) const;
	UnitId find_player_unit_id_at(const godot::Vector2 &p_position) const;

	void remove_dead_units();
	void remove_destroyed_barracks();
	void check_win_condition();
	void update_units(double p_delta);
	void update_training(double p_delta);
	GameCommandResult apply_command(const GameCommand &p_command);
	SelectionResult select_at(int32_t p_owner, const godot::Vector2 &p_position) const;
	SelectionResult select_units_in_rect(int32_t p_owner, const godot::Rect2 &p_rect, bool p_has_unit_type_filter = false, UnitType p_unit_type_filter = UnitType::WORKER) const;
	SelectionResult select_all_units_of_type(int32_t p_owner, UnitType p_type) const;

private:
	Unit *find_unit(int32_t p_id);
	const Unit *find_unit(int32_t p_id) const;
	Unit *find_player_unit_at(const godot::Vector2 &p_position);
	ResourceNode *find_resource(int32_t p_id);
	const ResourceNode *find_resource(int32_t p_id) const;
	Base *find_base(int32_t p_owner);
	const Base *find_base(int32_t p_owner) const;
	Barracks *find_barracks(int32_t p_owner);
	Barracks *find_barracks_by_id(BuildingId p_id);
	const Barracks *find_barracks_by_id(BuildingId p_id) const;
	Barracks *find_completed_barracks(int32_t p_owner);
	BuildingId find_player_barracks_id_at(const godot::Vector2 &p_position) const;
	BuildingId find_enemy_barracks_id_at(int32_t p_owner, const godot::Vector2 &p_position) const;
	int32_t find_enemy_unit_id_at(int32_t p_owner, const godot::Vector2 &p_position) const;
	Unit *find_available_worker(int32_t p_owner);
	int32_t find_nearest_resource(const godot::Vector2 &p_position) const;
	int32_t find_enemy_unit_id(int32_t p_owner, const godot::Vector2 &p_position, float p_range) const;

	void update_unit_timers(Unit &p_unit, double p_delta);
	void update_unit_auto_aggro(Unit &p_unit);
	void update_unit_build(Unit &p_unit, double p_delta);
	void update_unit_move(Unit &p_unit, double p_delta);
	void update_unit_gather(Unit &p_unit, double p_delta);
	void update_unit_return(Unit &p_unit, double p_delta);
	void update_unit_attack(Unit &p_unit, double p_delta);
	void update_unit(Unit &p_unit, double p_delta);
	void move_unit_toward_with_avoidance(Unit &p_unit, const godot::Vector2 &p_target, float p_max_distance, ResourceId p_ignored_resource = -1, BuildingId p_ignored_building = -1, PlayerId p_ignored_base_owner = -1);
	Unit create_unit(UnitId p_id, PlayerId p_owner, UnitType p_type, const godot::Vector2 &p_position) const;
	RallyActionComponent resolve_rally_action_for_production(PlayerId p_owner, UnitType p_produced_type, const godot::Vector2 &p_position) const;
	void apply_rally_action_to_unit(Unit &p_unit, const RallyActionComponent &p_rally_action);
	void train_unit(int32_t p_owner, UnitType p_type, BuildingId p_source_building_id = -1);
	void spawn_unit(int32_t p_owner, UnitType p_type, const godot::Vector2 &p_source_position, float p_spawn_distance, const RallyActionComponent *p_rally_action = nullptr);
	BuildingId place_barracks(int32_t p_owner, const godot::Vector2 &p_position, Unit *p_builder = nullptr);
	void delete_barracks(BuildingId p_building_id);
	bool set_selected_production_rally_point(int32_t p_owner, int32_t p_selected_base_owner, BuildingId p_selected_building_id, const godot::Vector2 &p_position);
	CommandFeedback command_selected_to(int32_t p_owner, const std::vector<int32_t> &p_selected_unit_ids, int32_t p_selected_base_owner, BuildingId p_selected_building_id, const godot::Vector2 &p_position);

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
};

} // namespace tinyv1

#pragma once

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <vector>

namespace tinyv1 {

class TinyGame : public godot::Node2D {
	GDCLASS(TinyGame, godot::Node2D)

	enum class UnitType {
		WORKER,
		FIGHTER,
	};

	enum class UnitOrder {
		IDLE,
		MOVE,
		GATHER,
		RETURN,
		ATTACK,
	};

	struct ResourceNode {
		int32_t id = 0;
		godot::Vector2 position;
		int32_t amount = 0;
	};

	struct Base {
		int32_t owner = 0;
		godot::Vector2 position;
		float hp = 400.0f;
		float train_timer = 0.0f;
	};

	struct Barracks {
		int32_t owner = 0;
		godot::Vector2 position;
		float hp = 250.0f;
	};

	struct Unit {
		int32_t id = 0;
		int32_t owner = 0;
		UnitType type = UnitType::WORKER;
		UnitOrder order = UnitOrder::IDLE;
		godot::Vector2 position;
		godot::Vector2 target_position;
		float hp = 40.0f;
		float attack_timer = 0.0f;
		float gather_timer = 0.0f;
		int32_t carrying = 0;
		int32_t target_resource = -1;
		int32_t target_base_owner = -1;
		bool selected = false;
	};

	godot::Rect2 map_rect;
	std::vector<ResourceNode> resources;
	std::vector<Base> bases;
	std::vector<Barracks> barracks;
	std::vector<Unit> units;
	int32_t player_essence = 50;
	int32_t bot_essence = 50;
	int32_t next_unit_id = 1;
	int32_t selected_base_owner = -1;
	int32_t selected_barracks_index = -1;
	float bot_decision_timer = 0.0f;
	godot::String winner_text;
	bool is_placing_barracks = false;
	bool is_drag_selecting = false;
	bool drag_has_unit_type_filter = false;
	UnitType drag_unit_type_filter = UnitType::WORKER;
	godot::Vector2 drag_start;
	godot::Vector2 drag_current;
	godot::Ref<godot::Texture2D> worker_texture;
	godot::Ref<godot::Texture2D> fighter_texture;
	godot::Ref<godot::Texture2D> base_texture;
	godot::Ref<godot::Texture2D> barracks_texture;
	godot::Ref<godot::Texture2D> goldmine_texture;

protected:
	static void _bind_methods();

public:
	TinyGame() = default;
	~TinyGame() override = default;

	void _ready() override;
	void _process(double p_delta) override;
	void _draw() override;
	void _unhandled_input(const godot::Ref<godot::InputEvent> &p_event) override;

	int32_t get_version_number() const;
	godot::String get_status_text() const;
	godot::String get_resource_text() const;
	godot::String get_selected_name() const;
	godot::String get_selected_actions_text() const;
	godot::String get_selected_details_text() const;
	godot::Color get_selected_portrait_color() const;
	godot::String get_action_button_text(int32_t p_index) const;
	bool is_action_button_enabled(int32_t p_index) const;
	void perform_action_button(int32_t p_index);

private:
	void reset_match();
	void load_textures();
	void update_unit(Unit &p_unit, double p_delta);
	void update_bot(double p_delta);
	void train_unit(int32_t p_owner, UnitType p_type);
	void place_barracks(int32_t p_owner, const godot::Vector2 &p_position);
	void command_selected_to(const godot::Vector2 &p_position);
	void select_at(const godot::Vector2 &p_position);
	void select_units_in_rect(const godot::Rect2 &p_rect);
	void select_all_units_of_type(UnitType p_type);
	void remove_dead_units();
	void check_win_condition();
	Unit *find_unit(int32_t p_id);
	Unit *find_player_unit_at(const godot::Vector2 &p_position);
	ResourceNode *find_resource(int32_t p_id);
	Base *find_base(int32_t p_owner);
	Barracks *find_barracks(int32_t p_owner);
	int32_t find_nearest_resource(const godot::Vector2 &p_position) const;
	int32_t find_enemy_unit_id(int32_t p_owner, const godot::Vector2 &p_position, float p_range) const;
	int32_t selected_unit_count() const;
	const Unit *first_selected_unit() const;
	godot::Vector2 formation_offset(int32_t p_index, int32_t p_count) const;
	float unit_speed(const Unit &p_unit) const;
	float unit_radius(const Unit &p_unit) const;
	float unit_attack_range(const Unit &p_unit) const;
	float unit_attack_damage(const Unit &p_unit) const;
	godot::Color owner_color(int32_t p_owner) const;
};

} // namespace tinyv1

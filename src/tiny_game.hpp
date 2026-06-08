#pragma once

#include "bot_controller.hpp"
#include "game_simulation/game_simulation.hpp"
#include "local_player_state.hpp"
#include "rendering/game_renderer.hpp"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

namespace tinyv1 {

class TinyGame : public godot::Node2D {
	GDCLASS(TinyGame, godot::Node2D)

	GameSimulation sim;
	BotController bot;
	LocalPlayerState local;
	GameRenderer renderer;
	float animation_time = 0.0f;

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
	godot::String get_selected_portrait_path() const;
	godot::String get_action_button_text(int32_t p_index) const;
	godot::String get_action_button_icon_path(int32_t p_index) const;
	bool is_action_button_enabled(int32_t p_index) const;
	void perform_action_button(int32_t p_index);

private:
	void reset_match();
};

} // namespace tinyv1

#include "tiny_game.hpp"

#include "game_constants.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>

using namespace godot;

namespace tinyv1 {

void TinyGame::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_version_number"), &TinyGame::get_version_number);
	ClassDB::bind_method(D_METHOD("get_status_text"), &TinyGame::get_status_text);
	ClassDB::bind_method(D_METHOD("get_resource_text"), &TinyGame::get_resource_text);
	ClassDB::bind_method(D_METHOD("get_selected_name"), &TinyGame::get_selected_name);
	ClassDB::bind_method(D_METHOD("get_selected_actions_text"), &TinyGame::get_selected_actions_text);
	ClassDB::bind_method(D_METHOD("get_selected_details_text"), &TinyGame::get_selected_details_text);
	ClassDB::bind_method(D_METHOD("get_selected_portrait_color"), &TinyGame::get_selected_portrait_color);
	ClassDB::bind_method(D_METHOD("get_selected_portrait_path"), &TinyGame::get_selected_portrait_path);
	ClassDB::bind_method(D_METHOD("get_action_button_text", "index"), &TinyGame::get_action_button_text);
	ClassDB::bind_method(D_METHOD("get_action_button_icon_path", "index"), &TinyGame::get_action_button_icon_path);
	ClassDB::bind_method(D_METHOD("is_action_button_enabled", "index"), &TinyGame::is_action_button_enabled);
	ClassDB::bind_method(D_METHOD("perform_action_button", "index"), &TinyGame::perform_action_button);
}

void TinyGame::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	set_process(true);
	set_process_unhandled_input(true);
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	load_textures();
	reset_match();
	UtilityFunctions::print("tinyV1 native prototype ready");
}

void TinyGame::_process(double p_delta) {
	if (!sim.get_winner_text().is_empty()) {
		queue_redraw();
		return;
	}

	sim.update_units(p_delta);
	local.command_marker_timer = std::max(0.0f, local.command_marker_timer - static_cast<float>(p_delta));

	sim.update_training(p_delta);
	for (const GameCommand &command : bot.update(sim, p_delta)) {
		sim.apply_command(command);
	}
	sim.remove_dead_units();
	sim.remove_destroyed_barracks();
	sim.check_win_condition();
	queue_redraw();
}

void TinyGame::_draw() {
	const Rect2 map_rect = sim.get_map_rect();
	draw_rect(map_rect, Color(0.18f, 0.45f, 0.18f), true);
	draw_rect(map_rect, Color(0.08f, 0.28f, 0.09f), false, 3.0f);

	for (const ResourceSummary &resource : sim.get_render_resources()) {
		if (resource.amount <= 0) {
			continue;
		}

		if (goldmine_texture.is_valid()) {
			draw_texture_rect(goldmine_texture, Rect2(resource.position - Vector2(30, 30), Vector2(60, 60)), false);
		} else {
			draw_circle(resource.position, RESOURCE_RADIUS, Color(0.15f, 0.70f, 0.85f));
			draw_circle(resource.position, RESOURCE_RADIUS * 0.55f, Color(0.75f, 0.95f, 1.0f));
		}
	}

	for (const BaseSummary &base : sim.get_render_bases()) {
		const Color color = owner_color(base.owner);
		if (base_texture.is_valid()) {
			draw_texture_rect(base_texture, Rect2(base.position - Vector2(44, 38), Vector2(88, 76)), false);
		} else {
			draw_circle(base.position, BASE_RADIUS, color.darkened(0.25f));
			draw_rect(Rect2(base.position - Vector2(32, 24), Vector2(64, 48)), color, true);
			draw_rect(Rect2(base.position - Vector2(34, 26), Vector2(68, 52)), Color(0.04f, 0.04f, 0.04f), false, 2.0f);
		}
		if (base.owner == local.selected_base_owner || base.hp < base.max_hp) {
			const float hp_ratio = std::max(0.0f, base.hp / base.max_hp);
			draw_rect(Rect2(base.position + Vector2(-36, -50), Vector2(72, 7)), Color(0.04f, 0.04f, 0.04f), true);
			draw_rect(Rect2(base.position + Vector2(-36, -50), Vector2(72 * hp_ratio, 7)), Color(0.2f, 0.9f, 0.3f), true);
		}
		if (base.training_worker && base.train_duration > 0.0f) {
			const float progress = 1.0f - std::max(0.0f, base.train_timer / base.train_duration);
			draw_rect(Rect2(base.position + Vector2(-36, 42), Vector2(72, 7)), Color(0.04f, 0.04f, 0.04f), true);
			draw_rect(Rect2(base.position + Vector2(-36, 42), Vector2(72 * progress, 7)), Color(0.35f, 0.65f, 1.0f), true);
		}
		if (base.owner == local.selected_base_owner && base.has_rally_point) {
			draw_rect(Rect2(base.rally_point - Vector2(6, 6), Vector2(12, 12)), Color(1.0f, 0.1f, 0.1f), true);
			draw_line(base.position, base.rally_point, Color(1.0f, 0.1f, 0.1f, 0.55f), 2.0f);
		}
	}

	for (const BuildingSummary &building : sim.get_render_buildings()) {
		const Color color = owner_color(building.owner).darkened(0.1f);
		const Color sprite_modulate = building.completed ? Color(1, 1, 1, 1) : Color(1, 1, 1, 0.45f);
		if (barracks_texture.is_valid()) {
			draw_texture_rect(barracks_texture, Rect2(building.position - Vector2(38, 34), Vector2(76, 68)), false, sprite_modulate);
		} else {
			draw_rect(Rect2(building.position - Vector2(34, 28), Vector2(68, 56)), building.completed ? color : color.lightened(0.35f), true);
			draw_line(building.position + Vector2(-24, 0), building.position + Vector2(24, 0), Color(0.05f, 0.05f, 0.05f), 4.0f);
		}
		if (!building.completed) {
			draw_rect(Rect2(building.position - Vector2(36, 30), Vector2(72, 60)), Color(0.04f, 0.04f, 0.04f), false, 2.0f);
		}
		if (building.id == local.selected_building_id || building.hp < building.max_hp) {
			const float hp_ratio = std::max(0.0f, building.hp / building.max_hp);
			draw_rect(Rect2(building.position + Vector2(-36, -44), Vector2(72, 7)), Color(0.04f, 0.04f, 0.04f), true);
			draw_rect(Rect2(building.position + Vector2(-36, -44), Vector2(72 * hp_ratio, 7)), Color(0.2f, 0.9f, 0.3f), true);
		}
		if (!building.completed) {
			draw_rect(Rect2(building.position + Vector2(-36, 38), Vector2(72, 7)), Color(0.04f, 0.04f, 0.04f), true);
			draw_rect(Rect2(building.position + Vector2(-36, 38), Vector2(72.0f * building.build_progress, 7)), Color(0.95f, 0.75f, 0.25f), true);
		} else if (building.training_fighter && building.train_duration > 0.0f) {
			const float progress = 1.0f - std::max(0.0f, building.train_timer / building.train_duration);
			draw_rect(Rect2(building.position + Vector2(-36, 38), Vector2(72, 7)), Color(0.04f, 0.04f, 0.04f), true);
			draw_rect(Rect2(building.position + Vector2(-36, 38), Vector2(72.0f * progress, 7)), Color(0.35f, 0.65f, 1.0f), true);
		}
		if (building.id == local.selected_building_id) {
			draw_rect(Rect2(building.position - Vector2(42, 36), Vector2(84, 72)), Color(1.0f, 0.95f, 0.25f), false, 2.0f);
		}
		if (building.id == local.selected_building_id && building.has_rally_point) {
			draw_rect(Rect2(building.rally_point - Vector2(6, 6), Vector2(12, 12)), Color(1.0f, 0.1f, 0.1f), true);
			draw_line(building.position, building.rally_point, Color(1.0f, 0.1f, 0.1f, 0.55f), 2.0f);
		}
	}

	if (local.command_marker_timer > 0.0f) {
		const float alpha = std::min(1.0f, local.command_marker_timer / 0.45f);
		draw_rect(Rect2(local.command_marker_position - Vector2(6, 6), Vector2(12, 12)), Color(1.0f, 0.1f, 0.1f, alpha), true);
		draw_rect(Rect2(local.command_marker_position - Vector2(8, 8), Vector2(16, 16)), Color(0.25f, 0.0f, 0.0f, alpha), false, 2.0f);
	}

	for (const UnitSummary &unit : sim.get_render_units()) {
		const float radius = unit_radius(unit);
		Color color = owner_color(unit.owner);
		if (unit.type == UnitType::WORKER) {
			color = color.lightened(0.25f);
		}

		if (local.is_unit_selected(unit.id)) {
			draw_arc(unit.position + Vector2(0, 8), radius + 8.0f, 0.0f, Math_TAU, 48, Color(1.0f, 0.95f, 0.25f, 0.95f), 3.0f);
		}

		const Ref<Texture2D> unit_texture = unit.type == UnitType::WORKER ? worker_texture : fighter_texture;
		if (unit_texture.is_valid()) {
			draw_texture_rect(unit_texture, Rect2(unit_sprite_top_left(unit), unit_sprite_size(unit)), false);
		} else {
			draw_circle(unit.position, radius, color);
		}
		if (unit.type == UnitType::FIGHTER && !unit_texture.is_valid()) {
			draw_line(unit.position + Vector2(-radius, -radius), unit.position + Vector2(radius, radius), Color(0.05f, 0.05f, 0.05f), 3.0f);
		}
		if (local.is_unit_selected(unit.id) || unit.hp < unit.max_hp) {
			const float hp_ratio = std::max(0.0f, unit.hp / unit.max_hp);
			const Vector2 hp_position = unit_texture.is_valid() ? unit_sprite_top_left(unit) + Vector2(0, -8) : unit.position + Vector2(-16, -30);
			draw_rect(Rect2(hp_position, Vector2(32, 4)), Color(0.04f, 0.04f, 0.04f), true);
			draw_rect(Rect2(hp_position, Vector2(32 * hp_ratio, 4)), Color(0.2f, 0.9f, 0.3f), true);
		}
	}

	if (local.is_drag_selecting) {
		const Vector2 top_left(std::min(local.drag_start.x, local.drag_current.x), std::min(local.drag_start.y, local.drag_current.y));
		const Vector2 size(std::abs(local.drag_current.x - local.drag_start.x), std::abs(local.drag_current.y - local.drag_start.y));
		const Rect2 selection_rect(top_left, size);
		draw_rect(selection_rect, Color(0.25f, 0.55f, 1.0f, 0.16f), true);
		draw_rect(selection_rect, Color(0.65f, 0.85f, 1.0f), false, 2.0f);
	}

	if (local.is_placing_barracks) {
		const Vector2 mouse_position = get_global_mouse_position();
		draw_rect(Rect2(mouse_position - Vector2(34, 28), Vector2(68, 56)), Color(0.35f, 0.75f, 1.0f, 0.35f), true);
		draw_rect(Rect2(mouse_position - Vector2(36, 30), Vector2(72, 60)), Color(0.75f, 0.95f, 1.0f), false, 2.0f);
	}

	if (!sim.get_winner_text().is_empty()) {
		draw_rect(Rect2(Vector2(390, 300), Vector2(500, 120)), Color(0.02f, 0.02f, 0.02f, 0.85f), true);
	}
}

void TinyGame::_unhandled_input(const Ref<InputEvent> &p_event) {
	if (!sim.get_winner_text().is_empty()) {
		Ref<InputEventKey> key_event = p_event;
		if (key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo()) {
			reset_match();
		}
		return;
	}

	Ref<InputEventMouseButton> mouse_event = p_event;
	if (mouse_event.is_valid()) {
		const Vector2 mouse_position = mouse_event->get_position();
		if (mouse_event->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
			if (mouse_event->is_pressed()) {
				if (local.is_placing_barracks) {
					GameCommand command;
					command.type = GameCommandType::PLACE_BARRACKS;
					command.owner = PLAYER;
					command.position = mouse_position;
					UnitSummary builder;
					command.builder_unit_id = first_selected_unit_summary(builder) ? builder.id : -1;
					const GameCommandResult result = sim.apply_command(command);
					const int32_t building_id = result.placed_building_id;
					if (sim.get_essence(PLAYER) < BARRACKS_COST || building_id != -1) {
						local.is_placing_barracks = false;
						if (building_id != -1) {
							local.clear_selection();
							local.selected_building_id = building_id;
						}
					}
					queue_redraw();
					return;
				}

				const UnitId unit_id_at_mouse = sim.find_player_unit_id_at(mouse_position);
				UnitSummary unit_at_mouse;
				const bool has_unit_at_mouse = sim.get_unit_summary(unit_id_at_mouse, unit_at_mouse);
				if (mouse_event->is_double_click() && has_unit_at_mouse) {
					local.is_drag_selecting = false;
					local.drag_has_unit_type_filter = false;
					local.set_selection(sim.select_all_units_of_type(PLAYER, unit_at_mouse.type));
					queue_redraw();
					return;
				}

				local.is_drag_selecting = true;
				local.drag_has_unit_type_filter = has_unit_at_mouse;
				if (has_unit_at_mouse) {
					local.drag_unit_type_filter = unit_at_mouse.type;
				}
				local.drag_start = mouse_position;
				local.drag_current = mouse_position;
			} else if (local.is_drag_selecting) {
				local.is_drag_selecting = false;
				const Vector2 top_left(std::min(local.drag_start.x, local.drag_current.x), std::min(local.drag_start.y, local.drag_current.y));
				const Vector2 size(std::abs(local.drag_current.x - local.drag_start.x), std::abs(local.drag_current.y - local.drag_start.y));
				if (size.length() >= 8.0f) {
					local.set_selection(sim.select_units_in_rect(PLAYER, Rect2(top_left, size), local.drag_has_unit_type_filter, local.drag_unit_type_filter));
				} else {
					local.set_selection(sim.select_at(PLAYER, mouse_position));
				}
				local.drag_has_unit_type_filter = false;
			}
		} else if (mouse_event->get_button_index() == MouseButton::MOUSE_BUTTON_RIGHT && mouse_event->is_pressed()) {
			local.is_placing_barracks = false;
			GameCommand command;
			command.type = GameCommandType::COMMAND_SELECTED_TO;
			command.owner = PLAYER;
			command.position = mouse_position;
			command.selected_unit_ids = local.selected_unit_ids;
			command.selected_base_owner = local.selected_base_owner;
			command.selected_building_id = local.selected_building_id;
			const GameCommandResult result = sim.apply_command(command);
			if (result.feedback.has_marker) {
				local.command_marker_position = result.feedback.marker_position;
				local.command_marker_timer = 0.45f;
			}
		}
		queue_redraw();
		return;
	}

	Ref<InputEventMouseMotion> motion_event = p_event;
	if (motion_event.is_valid() && local.is_drag_selecting) {
		local.drag_current = motion_event->get_position();
		queue_redraw();
		return;
	}
	if (motion_event.is_valid() && local.is_placing_barracks) {
		queue_redraw();
		return;
	}

	Ref<InputEventKey> key_event = p_event;
	if (!key_event.is_valid() || !key_event->is_pressed() || key_event->is_echo()) {
		return;
	}

	if (key_event->get_keycode() == Key::KEY_DELETE) {
		if (local.is_placing_barracks) {
			local.is_placing_barracks = false;
		} else {
			GameCommand command;
			command.type = GameCommandType::DELETE_BARRACKS;
			command.owner = PLAYER;
			command.selected_building_id = local.selected_building_id;
			sim.apply_command(command);
			local.clear_selection();
		}
	} else if (key_event->get_keycode() == Key::KEY_F && local.selected_building_id != -1) {
		GameCommand command;
		command.type = GameCommandType::TRAIN_UNIT;
		command.owner = PLAYER;
		command.unit_type = UnitType::FIGHTER;
		command.selected_building_id = local.selected_building_id;
		sim.apply_command(command);
	} else if (key_event->get_keycode() == Key::KEY_W && local.selected_base_owner == PLAYER) {
		GameCommand command;
		command.type = GameCommandType::TRAIN_UNIT;
		command.owner = PLAYER;
		command.unit_type = UnitType::WORKER;
		sim.apply_command(command);
	} else if (key_event->get_keycode() == Key::KEY_B && !local.selected_unit_ids.empty() && sim.can_start_barracks_placement(PLAYER, local.selected_unit_ids.front())) {
		local.is_placing_barracks = true;
	}
}

int32_t TinyGame::get_version_number() const {
	return 1;
}

String TinyGame::get_status_text() const {
	String selected_text = static_cast<int32_t>(local.selected_unit_ids.size()) > 0 ? " selected units: " + String::num_int64(static_cast<int32_t>(local.selected_unit_ids.size())) : "";
	if (local.selected_base_owner == PLAYER) {
		selected_text += " | base selected: W worker";
	} else if (local.selected_building_id != -1) {
		selected_text += " | barracks selected: F fighter";
	} else if (local.is_placing_barracks) {
		selected_text += " | placing barracks: left click place, right click cancel";
	}
	if (!sim.get_winner_text().is_empty()) {
		return sim.get_winner_text() + String(" | press any key to restart");
	}
	return "Gold " + String::num_int64(sim.get_essence(PLAYER)) + " | left click select | right click command" + selected_text;
}

String TinyGame::get_resource_text() const {
	return "Food: 0\nWood: 0\nGold: " + String::num_int64(sim.get_essence(PLAYER));
}

String TinyGame::get_selected_name() const {
	const int32_t count = static_cast<int32_t>(local.selected_unit_ids.size());
	if (count > 1) {
		return "Unit Group (" + String::num_int64(count) + ")";
	}

	UnitSummary unit;
	if (first_selected_unit_summary(unit)) {
		return unit.type == UnitType::WORKER ? "Worker" : "Fighter";
	}

	if (local.selected_base_owner == PLAYER) {
		return "Main Base";
	}

	BuildingSummary selected_building;
	if (selected_building_summary(selected_building)) {
		if (!selected_building.completed) {
			return "Barracks Site";
		}
		return "Barracks";
	}

	return "No Selection";
}

String TinyGame::get_selected_actions_text() const {
	UnitSummary unit;
	if (static_cast<int32_t>(local.selected_unit_ids.size()) > 0) {
		if (first_selected_unit_summary(unit) && unit.type == UnitType::WORKER) {
			return "Right Click: Move / Gather\nBuild: Barracks";
		}
		return "Right Click: Attack-move\nRight Click Base: Attack";
	}

	if (local.selected_base_owner == PLAYER) {
		BaseSummary base;
		if (sim.get_base_summary(PLAYER, base) && base.training_worker) {
			return "Training Worker: " + String::num_int64(static_cast<int64_t>((1.0f - base.train_timer / base.train_duration) * 100.0f)) + "%\nQueue: " + String::num_int64(base.worker_queue) + "/3";
		}
		return "W: Train Worker (20 Gold)\nRight Click: Set rally point\nQueue: " + String::num_int64(base.worker_queue) + "/3";
	}

	BuildingSummary selected_building;
	if (selected_building_summary(selected_building)) {
		if (!selected_building.completed) {
			return "Under construction\nDel: Cancel refund";
		}
		if (selected_building.training_fighter) {
			return "Training Fighter: " + String::num_int64(static_cast<int64_t>((1.0f - selected_building.train_timer / selected_building.train_duration) * 100.0f)) + "%\nQueue: " + String::num_int64(selected_building.fighter_queue) + "/3";
		}
		return "F: Train Fighter (25 Gold)\nRight Click: Set rally point\nQueue: " + String::num_int64(selected_building.fighter_queue) + "/3";
	}

	return "Select a unit or building.";
}

String TinyGame::get_selected_details_text() const {
	const int32_t count = static_cast<int32_t>(local.selected_unit_ids.size());
	if (count > 1) {
		int32_t workers = 0;
		int32_t fighters = 0;
		for (UnitId unit_id : local.selected_unit_ids) {
			UnitSummary unit;
			if (!sim.get_unit_summary(unit_id, unit)) {
				continue;
			}
			if (unit.type == UnitType::WORKER) {
				workers++;
			} else {
				fighters++;
			}
		}
		return "Selected group\nWorkers: " + String::num_int64(workers) + "\nFighters: " + String::num_int64(fighters);
	}

	UnitSummary unit;
	if (first_selected_unit_summary(unit)) {
		String role = unit.type == UnitType::WORKER ? "Gathers Gold and builds Barracks." : "Basic melee combat unit.";
		return "HP: " + String::num_int64(static_cast<int64_t>(unit.hp)) + "\n" + role;
	}

	if (local.selected_base_owner == PLAYER) {
		BaseSummary base;
		const int64_t hp = sim.get_base_summary(PLAYER, base) ? static_cast<int64_t>(base.hp) : 0;
		if (base.training_worker) {
			return "HP: " + String::num_int64(hp) + "\nTraining Worker\nQueue: " + String::num_int64(base.worker_queue) + "/3";
		}
		return "HP: " + String::num_int64(hp) + "\nTrains workers.";
	}

	BuildingSummary selected_building;
	if (selected_building_summary(selected_building)) {
		if (!selected_building.completed) {
			return "Progress: " + String::num_int64(static_cast<int64_t>(selected_building.build_progress * 100.0f)) + "%\nNeeds worker construction.";
		}
		if (selected_building.training_fighter) {
			return "HP: " + String::num_int64(static_cast<int64_t>(selected_building.hp)) + "\nTraining Fighter\nQueue: " + String::num_int64(selected_building.fighter_queue) + "/3";
		}
		return "HP: " + String::num_int64(static_cast<int64_t>(selected_building.hp)) + "\nTrains fighters.";
	}

	return "No unit selected.\nSelect workers, fighters, or your base.";
}

Color TinyGame::get_selected_portrait_color() const {
	UnitSummary unit;
	if (static_cast<int32_t>(local.selected_unit_ids.size()) > 1) {
		return Color(0.35f, 0.55f, 0.95f);
	}
	if (first_selected_unit_summary(unit)) {
		return unit.type == UnitType::WORKER ? Color(0.55f, 0.78f, 1.0f) : Color(0.25f, 0.55f, 1.0f);
	}
	if (local.selected_base_owner == PLAYER) {
		return Color(0.15f, 0.35f, 0.85f);
	}
	if (local.selected_building_id != -1) {
		return Color(0.20f, 0.45f, 0.95f);
	}
	return Color(0.18f, 0.18f, 0.20f);
}

String TinyGame::get_selected_portrait_path() const {
	UnitSummary unit;
	if (first_selected_unit_summary(unit)) {
		return unit.type == UnitType::WORKER ? "res://assets/units/greece/worker/worker_face.png" : "res://assets/units/greece/hoplite/hoplite_face.png";
	}

	if (local.selected_base_owner == PLAYER) {
		return "res://assets/units/greece/towncenter/towncenter_face.png";
	}

	if (local.selected_building_id != -1) {
		return "res://assets/units/greece/barracks/barracks_face.png";
	}

	return "";
}

String TinyGame::get_action_button_text(int32_t p_index) const {
	if (local.selected_base_owner == PLAYER) {
		if (p_index == 0) {
			BaseSummary base;
			if (sim.get_base_summary(PLAYER, base) && base.training_worker) {
				return "Queue " + String::num_int64(base.worker_queue) + "/3";
			}
			return "20 Gold";
		}
	}

	BuildingSummary selected_building;
	if (selected_building_summary(selected_building) && p_index == 0) {
		if (!selected_building.completed) {
			return "Building...";
		}
		if (selected_building.training_fighter) {
			return "Queue " + String::num_int64(selected_building.fighter_queue) + "/3";
		}
		return "25 Gold";
	}

	UnitSummary unit;
	if (first_selected_unit_summary(unit)) {
		if (unit.type == UnitType::WORKER) {
			return p_index == 0 ? "80 Gold" : "";
		}
		return p_index == 0 ? "Attack" : "Hold";
	}

	return p_index == 0 ? "No Action" : "";
}

String TinyGame::get_action_button_icon_path(int32_t p_index) const {
	if (local.selected_base_owner == PLAYER && p_index == 0) {
		return "res://assets/units/greece/worker/worker_face.png";
	}

	if (local.selected_building_id != -1 && p_index == 0) {
		return "res://assets/units/greece/hoplite/hoplite_face.png";
	}

	UnitSummary unit;
	if (first_selected_unit_summary(unit) && unit.type == UnitType::WORKER && p_index == 0) {
		return "res://assets/units/greece/barracks/barracks_face.png";
	}

	return "";
}

bool TinyGame::is_action_button_enabled(int32_t p_index) const {
	if (local.selected_base_owner == PLAYER) {
		if (p_index == 0) {
			return sim.can_train_worker(PLAYER);
		}
	}

	BuildingSummary selected_building;
	if (selected_building_summary(selected_building) && p_index == 0) {
		return sim.can_train_fighter(PLAYER, selected_building.id);
	}

	UnitSummary unit;
	if (first_selected_unit_summary(unit) && unit.type == UnitType::WORKER && p_index == 0) {
		return sim.can_start_barracks_placement(PLAYER, unit.id);
	}

	return false;
}

void TinyGame::perform_action_button(int32_t p_index) {
	if (local.selected_base_owner == PLAYER && p_index == 0) {
		GameCommand command;
		command.type = GameCommandType::TRAIN_UNIT;
		command.owner = PLAYER;
		command.unit_type = UnitType::WORKER;
		sim.apply_command(command);
	} else if (local.selected_building_id != -1 && p_index == 0) {
		GameCommand command;
		command.type = GameCommandType::TRAIN_UNIT;
		command.owner = PLAYER;
		command.unit_type = UnitType::FIGHTER;
		command.selected_building_id = local.selected_building_id;
		sim.apply_command(command);
	} else {
		UnitSummary unit;
		if (first_selected_unit_summary(unit) && unit.type == UnitType::WORKER && p_index == 0 && sim.can_start_barracks_placement(PLAYER, unit.id)) {
			local.is_placing_barracks = true;
		}
	}
}

void TinyGame::reset_match() {
	sim.reset_match();
	bot.reset();
	local.reset();
}

void TinyGame::load_textures() {
	auto load_image_texture = [](const String &p_path) -> Ref<Texture2D> {
		Ref<Image> image;
		image.instantiate();
		if (image->load(p_path) != OK) {
			UtilityFunctions::push_warning(String("Could not load texture: ") + p_path);
			return Ref<Texture2D>();
		}

		return ImageTexture::create_from_image(image);
	};

	worker_texture = load_image_texture("res://assets/units/greece/worker/worker.png");
	fighter_texture = load_image_texture("res://assets/units/greece/hoplite/hoplite.png");
	base_texture = load_image_texture("res://assets/units/greece/towncenter/towncenter.png");
	barracks_texture = load_image_texture("res://assets/units/greece/barracks/barracks.png");
	goldmine_texture = load_image_texture("res://assets/world/gold/gold.png");
}

bool TinyGame::first_selected_unit_summary(UnitSummary &r_summary) const {
	for (int32_t unit_id : local.selected_unit_ids) {
		if (sim.get_unit_summary(unit_id, r_summary)) {
			return true;
		}
	}
	return false;
}

bool TinyGame::selected_building_summary(BuildingSummary &r_summary) const {
	return sim.get_building_summary(local.selected_building_id, r_summary);
}

float TinyGame::unit_radius(const UnitSummary &p_unit) const {
	return p_unit.type == UnitType::WORKER ? 12.0f : 16.0f;
}

Vector2 TinyGame::unit_sprite_size(const UnitSummary &p_unit) const {
	return p_unit.type == UnitType::WORKER ? Vector2(32, 32) : Vector2(40, 40);
}

Vector2 TinyGame::unit_sprite_top_left(const UnitSummary &p_unit) const {
	const Vector2 size = unit_sprite_size(p_unit);
	return p_unit.position - Vector2(size.x * 0.5f, size.y - 6.0f);
}

Color TinyGame::owner_color(int32_t p_owner) const {
	return p_owner == PLAYER ? Color(0.25f, 0.55f, 1.0f) : Color(0.95f, 0.25f, 0.22f);
}

} // namespace tinyv1

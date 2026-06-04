#include "tiny_game.hpp"

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
#include <cmath>

using namespace godot;

namespace tinyv1 {

namespace {
constexpr int32_t PLAYER = 0;
constexpr int32_t BOT = 1;
constexpr float BASE_RADIUS = 42.0f;
constexpr float BARRACKS_RADIUS = 34.0f;
constexpr float RESOURCE_RADIUS = 28.0f;
constexpr float WORKER_COST = 20.0f;
constexpr float FIGHTER_COST = 25.0f;
constexpr float BARRACKS_COST = 80.0f;
constexpr float GATHER_DISTANCE = 34.0f;
constexpr float RETURN_DISTANCE = 52.0f;
constexpr float GATHER_TIME = 1.0f;
constexpr int32_t WORKER_CARRY_LIMIT = 10;
constexpr float BOT_ATTACK_INTERVAL = 2.5f;

float distance_to(const Vector2 &a, const Vector2 &b) {
	return a.distance_to(b);
}

Vector2 move_toward(const Vector2 &from, const Vector2 &to, float max_distance) {
	const Vector2 offset = to - from;
	const float length = offset.length();
	if (length <= max_distance || length <= 0.001f) {
		return to;
	}
	return from + offset / length * max_distance;
}
} // namespace

void TinyGame::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_version_number"), &TinyGame::get_version_number);
	ClassDB::bind_method(D_METHOD("get_status_text"), &TinyGame::get_status_text);
	ClassDB::bind_method(D_METHOD("get_resource_text"), &TinyGame::get_resource_text);
	ClassDB::bind_method(D_METHOD("get_selected_name"), &TinyGame::get_selected_name);
	ClassDB::bind_method(D_METHOD("get_selected_actions_text"), &TinyGame::get_selected_actions_text);
	ClassDB::bind_method(D_METHOD("get_selected_details_text"), &TinyGame::get_selected_details_text);
	ClassDB::bind_method(D_METHOD("get_selected_portrait_color"), &TinyGame::get_selected_portrait_color);
	ClassDB::bind_method(D_METHOD("get_action_button_text", "index"), &TinyGame::get_action_button_text);
	ClassDB::bind_method(D_METHOD("is_action_button_enabled", "index"), &TinyGame::is_action_button_enabled);
	ClassDB::bind_method(D_METHOD("perform_action_button", "index"), &TinyGame::perform_action_button);
}

void TinyGame::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	set_process(true);
	set_process_unhandled_input(true);
	load_textures();
	reset_match();
	UtilityFunctions::print("tinyV1 native prototype ready");
}

void TinyGame::_process(double p_delta) {
	if (!winner_text.is_empty()) {
		queue_redraw();
		return;
	}

	for (Unit &unit : units) {
		update_unit(unit, p_delta);
	}

	update_bot(p_delta);
	remove_dead_units();
	check_win_condition();
	queue_redraw();
}

void TinyGame::_draw() {
	draw_rect(map_rect, Color(0.10f, 0.13f, 0.11f), true);
	draw_rect(map_rect, Color(0.32f, 0.36f, 0.30f), false, 3.0f);

	for (const ResourceNode &resource : resources) {
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

	for (const Base &base : bases) {
		const Color color = owner_color(base.owner);
		if (base_texture.is_valid()) {
			draw_texture_rect(base_texture, Rect2(base.position - Vector2(44, 38), Vector2(88, 76)), false);
		} else {
			draw_circle(base.position, BASE_RADIUS, color.darkened(0.25f));
			draw_rect(Rect2(base.position - Vector2(32, 24), Vector2(64, 48)), color, true);
		}
		draw_rect(Rect2(base.position - Vector2(34, 26), Vector2(68, 52)), Color(0.04f, 0.04f, 0.04f), false, 2.0f);
		const float hp_ratio = std::max(0.0f, base.hp / 400.0f);
		draw_rect(Rect2(base.position + Vector2(-36, -48), Vector2(72 * hp_ratio, 6)), Color(0.2f, 0.9f, 0.3f), true);
	}

	for (int32_t i = 0; i < static_cast<int32_t>(barracks.size()); ++i) {
		const Barracks &building = barracks[i];
		const Color color = owner_color(building.owner).darkened(0.1f);
		if (barracks_texture.is_valid()) {
			draw_texture_rect(barracks_texture, Rect2(building.position - Vector2(38, 34), Vector2(76, 68)), false);
		} else {
			draw_rect(Rect2(building.position - Vector2(34, 28), Vector2(68, 56)), color, true);
			draw_line(building.position + Vector2(-24, 0), building.position + Vector2(24, 0), Color(0.05f, 0.05f, 0.05f), 4.0f);
		}
		draw_rect(Rect2(building.position - Vector2(36, 30), Vector2(72, 60)), Color(0.04f, 0.04f, 0.04f), false, 2.0f);
		if (i == selected_barracks_index) {
			draw_rect(Rect2(building.position - Vector2(42, 36), Vector2(84, 72)), Color(1.0f, 0.95f, 0.25f), false, 2.0f);
		}
	}

	for (const Unit &unit : units) {
		const float radius = unit_radius(unit);
		Color color = owner_color(unit.owner);
		if (unit.type == UnitType::WORKER) {
			color = color.lightened(0.25f);
		}

		if (unit.selected) {
			draw_arc(unit.position + Vector2(0, 8), radius + 8.0f, 0.0f, Math_TAU, 48, Color(1.0f, 0.95f, 0.25f, 0.95f), 3.0f);
		}

		const Ref<Texture2D> unit_texture = unit.type == UnitType::WORKER ? worker_texture : fighter_texture;
		if (unit_texture.is_valid()) {
			draw_texture_rect(unit_texture, Rect2(unit.position - Vector2(18, 22), Vector2(36, 44)), false);
		} else {
			draw_circle(unit.position, radius, color);
		}
		if (unit.type == UnitType::FIGHTER && !unit_texture.is_valid()) {
			draw_line(unit.position + Vector2(-radius, -radius), unit.position + Vector2(radius, radius), Color(0.05f, 0.05f, 0.05f), 3.0f);
		}
	}

	if (is_drag_selecting) {
		const Vector2 top_left(std::min(drag_start.x, drag_current.x), std::min(drag_start.y, drag_current.y));
		const Vector2 size(std::abs(drag_current.x - drag_start.x), std::abs(drag_current.y - drag_start.y));
		const Rect2 selection_rect(top_left, size);
		draw_rect(selection_rect, Color(0.25f, 0.55f, 1.0f, 0.16f), true);
		draw_rect(selection_rect, Color(0.65f, 0.85f, 1.0f), false, 2.0f);
	}

	if (is_placing_barracks) {
		const Vector2 mouse_position = get_global_mouse_position();
		draw_rect(Rect2(mouse_position - Vector2(34, 28), Vector2(68, 56)), Color(0.35f, 0.75f, 1.0f, 0.35f), true);
		draw_rect(Rect2(mouse_position - Vector2(36, 30), Vector2(72, 60)), Color(0.75f, 0.95f, 1.0f), false, 2.0f);
	}

	if (!winner_text.is_empty()) {
		draw_rect(Rect2(Vector2(390, 300), Vector2(500, 120)), Color(0.02f, 0.02f, 0.02f, 0.85f), true);
	}
}

void TinyGame::_unhandled_input(const Ref<InputEvent> &p_event) {
	if (!winner_text.is_empty()) {
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
				if (is_placing_barracks) {
					place_barracks(PLAYER, mouse_position);
					queue_redraw();
					return;
				}

				Unit *unit_at_mouse = find_player_unit_at(mouse_position);
				if (mouse_event->is_double_click() && unit_at_mouse != nullptr) {
					is_drag_selecting = false;
					drag_has_unit_type_filter = false;
					select_all_units_of_type(unit_at_mouse->type);
					queue_redraw();
					return;
				}

				is_drag_selecting = true;
				drag_has_unit_type_filter = unit_at_mouse != nullptr;
				if (unit_at_mouse != nullptr) {
					drag_unit_type_filter = unit_at_mouse->type;
				}
				drag_start = mouse_position;
				drag_current = mouse_position;
			} else if (is_drag_selecting) {
				is_drag_selecting = false;
				const Vector2 top_left(std::min(drag_start.x, drag_current.x), std::min(drag_start.y, drag_current.y));
				const Vector2 size(std::abs(drag_current.x - drag_start.x), std::abs(drag_current.y - drag_start.y));
				if (size.length() >= 8.0f) {
					select_units_in_rect(Rect2(top_left, size));
				} else {
					select_at(mouse_position);
				}
			}
		} else if (mouse_event->get_button_index() == MouseButton::MOUSE_BUTTON_RIGHT && mouse_event->is_pressed()) {
			is_placing_barracks = false;
			command_selected_to(mouse_position);
		}
		queue_redraw();
		return;
	}

	Ref<InputEventMouseMotion> motion_event = p_event;
	if (motion_event.is_valid() && is_drag_selecting) {
		drag_current = motion_event->get_position();
		queue_redraw();
		return;
	}

	Ref<InputEventKey> key_event = p_event;
	if (!key_event.is_valid() || !key_event->is_pressed() || key_event->is_echo()) {
		return;
	}

	if (key_event->get_keycode() == Key::KEY_F && selected_barracks_index != -1) {
		train_unit(PLAYER, UnitType::FIGHTER);
	} else if (key_event->get_keycode() == Key::KEY_W && selected_base_owner == PLAYER) {
		train_unit(PLAYER, UnitType::WORKER);
	} else if (key_event->get_keycode() == Key::KEY_B && first_selected_unit() != nullptr && first_selected_unit()->type == UnitType::WORKER && player_essence >= BARRACKS_COST) {
		is_placing_barracks = true;
	}
}

int32_t TinyGame::get_version_number() const {
	return 1;
}

String TinyGame::get_status_text() const {
	String selected_text = selected_unit_count() > 0 ? " selected units: " + String::num_int64(selected_unit_count()) : "";
	if (selected_base_owner == PLAYER) {
		selected_text += " | base selected: W worker";
	} else if (selected_barracks_index != -1) {
		selected_text += " | barracks selected: F fighter";
	} else if (is_placing_barracks) {
		selected_text += " | placing barracks: left click place, right click cancel";
	}
	if (!winner_text.is_empty()) {
		return winner_text + String(" | press any key to restart");
	}
	return "Gold " + String::num_int64(player_essence) + " | left click select | right click command" + selected_text;
}

String TinyGame::get_resource_text() const {
	return "Food: 0\nWood: 0\nGold: " + String::num_int64(player_essence);
}

String TinyGame::get_selected_name() const {
	const int32_t count = selected_unit_count();
	if (count > 1) {
		return "Unit Group (" + String::num_int64(count) + ")";
	}

	const Unit *unit = first_selected_unit();
	if (unit != nullptr) {
		return unit->type == UnitType::WORKER ? "Worker" : "Fighter";
	}

	if (selected_base_owner == PLAYER) {
		return "Main Base";
	}

	if (selected_barracks_index != -1) {
		return "Barracks";
	}

	return "No Selection";
}

String TinyGame::get_selected_actions_text() const {
	const Unit *unit = first_selected_unit();
	if (selected_unit_count() > 0) {
		if (unit != nullptr && unit->type == UnitType::WORKER) {
			return "Right Click: Move / Gather\nBox Select: Group workers";
		}
		return "Right Click: Attack-move\nRight Click Base: Attack";
	}

	if (selected_base_owner == PLAYER) {
		return "W: Train Worker (20 Gold)";
	}

	if (selected_barracks_index != -1) {
		return "F: Train Fighter (25 Gold)";
	}

	return "Select a unit or building.";
}

String TinyGame::get_selected_details_text() const {
	const int32_t count = selected_unit_count();
	if (count > 1) {
		int32_t workers = 0;
		int32_t fighters = 0;
		for (const Unit &unit : units) {
			if (!unit.selected) {
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

	const Unit *unit = first_selected_unit();
	if (unit != nullptr) {
		String role = unit->type == UnitType::WORKER ? "Gathers Gold and builds Barracks." : "Basic melee combat unit.";
		return "HP: " + String::num_int64(static_cast<int64_t>(unit->hp)) + "\n" + role;
	}

	if (selected_base_owner == PLAYER) {
		Base *base = const_cast<TinyGame *>(this)->find_base(PLAYER);
		const int64_t hp = base == nullptr ? 0 : static_cast<int64_t>(base->hp);
		return "HP: " + String::num_int64(hp) + "\nTrains workers.";
	}

	if (selected_barracks_index != -1 && selected_barracks_index < static_cast<int32_t>(barracks.size())) {
		return "HP: " + String::num_int64(static_cast<int64_t>(barracks[selected_barracks_index].hp)) + "\nTrains fighters.";
	}

	return "No unit selected.\nSelect workers, fighters, or your base.";
}

Color TinyGame::get_selected_portrait_color() const {
	const Unit *unit = first_selected_unit();
	if (selected_unit_count() > 1) {
		return Color(0.35f, 0.55f, 0.95f);
	}
	if (unit != nullptr) {
		return unit->type == UnitType::WORKER ? Color(0.55f, 0.78f, 1.0f) : Color(0.25f, 0.55f, 1.0f);
	}
	if (selected_base_owner == PLAYER) {
		return Color(0.15f, 0.35f, 0.85f);
	}
	if (selected_barracks_index != -1) {
		return Color(0.20f, 0.45f, 0.95f);
	}
	return Color(0.18f, 0.18f, 0.20f);
}

String TinyGame::get_action_button_text(int32_t p_index) const {
	if (selected_base_owner == PLAYER) {
		if (p_index == 0) {
			return "W\nWorker\n20 Gold";
		}
	}

	if (selected_barracks_index != -1 && p_index == 0) {
		return "F\nFighter\n25 Gold";
	}

	const Unit *unit = first_selected_unit();
	if (unit != nullptr) {
		if (unit->type == UnitType::WORKER) {
			return p_index == 0 ? "Gather\nRight Click" : "B\nBarracks\n80 Gold";
		}
		return p_index == 0 ? "Attack Move\nRight Click" : "Hold\nSoon";
	}

	return p_index == 0 ? "No Action" : "";
}

bool TinyGame::is_action_button_enabled(int32_t p_index) const {
	if (selected_base_owner == PLAYER) {
		if (p_index == 0) {
			return player_essence >= WORKER_COST;
		}
	}

	if (selected_barracks_index != -1 && p_index == 0) {
		return player_essence >= FIGHTER_COST;
	}

	const Unit *unit = first_selected_unit();
	if (unit != nullptr && unit->type == UnitType::WORKER && p_index == 1) {
		return player_essence >= BARRACKS_COST;
	}

	return false;
}

void TinyGame::perform_action_button(int32_t p_index) {
	if (selected_base_owner == PLAYER && p_index == 0) {
		train_unit(PLAYER, UnitType::WORKER);
	} else if (selected_barracks_index != -1 && p_index == 0) {
		train_unit(PLAYER, UnitType::FIGHTER);
	} else if (first_selected_unit() != nullptr && first_selected_unit()->type == UnitType::WORKER && p_index == 1 && player_essence >= BARRACKS_COST) {
		is_placing_barracks = true;
	}
}

void TinyGame::reset_match() {
	map_rect = Rect2(Vector2(64, 86), Vector2(1152, 460));
	resources.clear();
	bases.clear();
	barracks.clear();
	units.clear();
	player_essence = 50;
	bot_essence = 50;
	next_unit_id = 1;
	selected_base_owner = -1;
	selected_barracks_index = -1;
	bot_decision_timer = 0.0f;
	winner_text = String();
	is_drag_selecting = false;
	drag_has_unit_type_filter = false;
	is_placing_barracks = false;

	bases.push_back({ PLAYER, Vector2(160, 316), 400.0f, 0.0f });
	bases.push_back({ BOT, Vector2(1120, 316), 400.0f, 0.0f });

	resources.push_back({ 1, Vector2(300, 210), 700 });
	resources.push_back({ 2, Vector2(300, 430), 700 });
	resources.push_back({ 3, Vector2(980, 210), 700 });
	resources.push_back({ 4, Vector2(980, 430), 700 });
	resources.push_back({ 5, Vector2(640, 316), 1200 });

	for (int32_t i = 0; i < 3; ++i) {
		units.push_back({ next_unit_id++, PLAYER, UnitType::WORKER, UnitOrder::IDLE, Vector2(210, 270 + i * 34.0f), Vector2(), 45.0f });
		units.push_back({ next_unit_id++, BOT, UnitType::WORKER, UnitOrder::GATHER, Vector2(1070, 270 + i * 34.0f), Vector2(980, 210), 45.0f, 0.0f, 0.0f, 0, 3 });
	}
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

	worker_texture = load_image_texture("res://assets/units/greece/worker.png");
	fighter_texture = load_image_texture("res://assets/units/greece/hoplite.png");
	base_texture = load_image_texture("res://assets/units/greece/town center.png");
	barracks_texture = load_image_texture("res://assets/units/greece/barracks.png");
	goldmine_texture = load_image_texture("res://assets/units/greece/goldmine.png");
}

void TinyGame::update_unit(Unit &p_unit, double p_delta) {
	p_unit.attack_timer = std::max(0.0f, p_unit.attack_timer - static_cast<float>(p_delta));
	p_unit.gather_timer = std::max(0.0f, p_unit.gather_timer - static_cast<float>(p_delta));

	if (p_unit.type == UnitType::FIGHTER && p_unit.order != UnitOrder::ATTACK) {
		const int32_t enemy_id = find_enemy_unit_id(p_unit.owner, p_unit.position, 140.0f);
		if (enemy_id != -1) {
			p_unit.order = UnitOrder::ATTACK;
			p_unit.target_base_owner = -1;
			p_unit.target_resource = enemy_id;
		}
	}

	if (p_unit.order == UnitOrder::MOVE) {
		p_unit.position = move_toward(p_unit.position, p_unit.target_position, unit_speed(p_unit) * static_cast<float>(p_delta));
		if (distance_to(p_unit.position, p_unit.target_position) <= 2.0f) {
			p_unit.order = UnitOrder::IDLE;
		}
		return;
	}

	if (p_unit.order == UnitOrder::GATHER && p_unit.type == UnitType::WORKER) {
		ResourceNode *resource = find_resource(p_unit.target_resource);
		if (resource == nullptr || resource->amount <= 0) {
			p_unit.order = UnitOrder::IDLE;
			return;
		}

		if (distance_to(p_unit.position, resource->position) > GATHER_DISTANCE) {
			p_unit.position = move_toward(p_unit.position, resource->position, unit_speed(p_unit) * static_cast<float>(p_delta));
			return;
		}

		if (p_unit.gather_timer <= 0.0f) {
			const int32_t gathered = std::min(WORKER_CARRY_LIMIT, resource->amount);
			resource->amount -= gathered;
			p_unit.carrying = gathered;
			p_unit.order = UnitOrder::RETURN;
			p_unit.gather_timer = GATHER_TIME;
		}
		return;
	}

	if (p_unit.order == UnitOrder::RETURN && p_unit.type == UnitType::WORKER) {
		Base *base = find_base(p_unit.owner);
		if (base == nullptr) {
			p_unit.order = UnitOrder::IDLE;
			return;
		}

		if (distance_to(p_unit.position, base->position) > RETURN_DISTANCE) {
			p_unit.position = move_toward(p_unit.position, base->position, unit_speed(p_unit) * static_cast<float>(p_delta));
			return;
		}

		if (p_unit.owner == PLAYER) {
			player_essence += p_unit.carrying;
		} else {
			bot_essence += p_unit.carrying;
		}
		p_unit.carrying = 0;
		p_unit.order = UnitOrder::GATHER;
		return;
	}

	if (p_unit.order == UnitOrder::ATTACK) {
		const int32_t enemy_id = find_enemy_unit_id(p_unit.owner, p_unit.position, unit_attack_range(p_unit));
		Unit *enemy_unit = find_unit(enemy_id);
		if (enemy_unit != nullptr) {
			if (p_unit.attack_timer <= 0.0f) {
				enemy_unit->hp -= unit_attack_damage(p_unit);
				p_unit.attack_timer = 0.8f;
			}
			return;
		}

		if (p_unit.target_base_owner == -1) {
			p_unit.position = move_toward(p_unit.position, p_unit.target_position, unit_speed(p_unit) * static_cast<float>(p_delta));
			if (distance_to(p_unit.position, p_unit.target_position) <= 2.0f) {
				p_unit.order = UnitOrder::IDLE;
			}
			return;
		}

		Base *enemy_base = find_base(p_unit.target_base_owner);
		if (enemy_base == nullptr) {
			p_unit.order = UnitOrder::IDLE;
			return;
		}

		if (distance_to(p_unit.position, enemy_base->position) > unit_attack_range(p_unit) + BASE_RADIUS) {
			p_unit.position = move_toward(p_unit.position, p_unit.target_position, unit_speed(p_unit) * static_cast<float>(p_delta));
			return;
		}

		if (p_unit.attack_timer <= 0.0f) {
			enemy_base->hp -= unit_attack_damage(p_unit);
			p_unit.attack_timer = 0.8f;
		}
	}
}

void TinyGame::update_bot(double p_delta) {
	bot_decision_timer -= static_cast<float>(p_delta);
	if (bot_decision_timer > 0.0f) {
		return;
	}
	bot_decision_timer = BOT_ATTACK_INTERVAL;

	for (Unit &unit : units) {
		if (unit.owner == BOT && unit.type == UnitType::WORKER && unit.order == UnitOrder::IDLE) {
			unit.target_resource = find_nearest_resource(unit.position);
			unit.order = UnitOrder::GATHER;
		}
	}

	int32_t bot_workers = 0;
	int32_t bot_fighters = 0;
	for (const Unit &unit : units) {
		if (unit.owner != BOT) {
			continue;
		}
		if (unit.type == UnitType::WORKER) {
			bot_workers++;
		} else {
			bot_fighters++;
		}
	}

	if (bot_workers < 5 && bot_essence >= WORKER_COST) {
		train_unit(BOT, UnitType::WORKER);
	} else if (find_barracks(BOT) == nullptr && bot_essence >= BARRACKS_COST) {
		Base *bot_base = find_base(BOT);
		if (bot_base != nullptr) {
			place_barracks(BOT, bot_base->position + Vector2(-110, 78));
		}
	} else if (find_barracks(BOT) != nullptr && bot_essence >= FIGHTER_COST) {
		train_unit(BOT, UnitType::FIGHTER);
	}

	if (bot_fighters >= 4) {
		Base *player_base = find_base(PLAYER);
		if (player_base == nullptr) {
			return;
		}
		for (Unit &unit : units) {
			if (unit.owner == BOT && unit.type == UnitType::FIGHTER) {
				unit.order = UnitOrder::ATTACK;
				unit.target_base_owner = PLAYER;
				unit.target_position = player_base->position + formation_offset(unit.id % bot_fighters, bot_fighters);
			}
		}
	}
}

void TinyGame::train_unit(int32_t p_owner, UnitType p_type) {
	int32_t &essence = p_owner == PLAYER ? player_essence : bot_essence;
	const int32_t cost = p_type == UnitType::WORKER ? static_cast<int32_t>(WORKER_COST) : static_cast<int32_t>(FIGHTER_COST);
	if (essence < cost) {
		return;
	}

	Base *base = find_base(p_owner);
	Barracks *source_barracks = find_barracks(p_owner);
	if (p_type == UnitType::WORKER && base == nullptr) {
		return;
	}
	if (p_type == UnitType::FIGHTER && source_barracks == nullptr) {
		return;
	}

	essence -= cost;
	const float side = p_owner == PLAYER ? 1.0f : -1.0f;
	const Vector2 source_position = p_type == UnitType::WORKER ? base->position : source_barracks->position;
	Unit unit;
	unit.id = next_unit_id++;
	unit.owner = p_owner;
	unit.type = p_type;
	unit.position = source_position + Vector2(70.0f * side, 24.0f * ((unit.id % 3) - 1));
	unit.target_position = unit.position;
	unit.hp = p_type == UnitType::WORKER ? 45.0f : 80.0f;
	if (p_owner == BOT && p_type == UnitType::WORKER) {
		unit.target_resource = find_nearest_resource(unit.position);
		unit.order = UnitOrder::GATHER;
	}
	units.push_back(unit);
}

void TinyGame::place_barracks(int32_t p_owner, const Vector2 &p_position) {
	int32_t &essence = p_owner == PLAYER ? player_essence : bot_essence;
	if (essence < BARRACKS_COST) {
		is_placing_barracks = false;
		return;
	}

	if (!map_rect.has_point(p_position)) {
		return;
	}

	essence -= static_cast<int32_t>(BARRACKS_COST);
	barracks.push_back({ p_owner, p_position, 250.0f });
	is_placing_barracks = false;
	if (p_owner == PLAYER) {
		selected_barracks_index = static_cast<int32_t>(barracks.size()) - 1;
		selected_base_owner = -1;
		for (Unit &unit : units) {
			unit.selected = false;
		}
	}
}

void TinyGame::command_selected_to(const Vector2 &p_position) {
	int32_t resource_id = -1;
	for (const ResourceNode &resource : resources) {
		if (resource.amount > 0 && distance_to(p_position, resource.position) <= RESOURCE_RADIUS + 14.0f) {
			resource_id = resource.id;
			break;
		}
	}

	bool attack_enemy_base = false;
	Base *bot_base = find_base(BOT);
	if (bot_base != nullptr && distance_to(p_position, bot_base->position) <= BASE_RADIUS + 24.0f) {
		attack_enemy_base = true;
	}

	const int32_t selected_count = selected_unit_count();
	int32_t selected_index = 0;
	for (Unit &unit : units) {
		if (!unit.selected || unit.owner != PLAYER) {
			continue;
		}

		const Vector2 offset = formation_offset(selected_index, selected_count);
		selected_index++;

		if (resource_id != -1 && unit.type == UnitType::WORKER) {
			unit.target_resource = resource_id;
			unit.order = UnitOrder::GATHER;
		} else if (attack_enemy_base || unit.type == UnitType::FIGHTER) {
			unit.target_base_owner = attack_enemy_base ? BOT : -1;
			unit.target_position = attack_enemy_base && bot_base != nullptr ? bot_base->position + offset : p_position + offset;
			unit.order = UnitOrder::ATTACK;
		} else {
			unit.target_base_owner = -1;
			unit.target_position = p_position + offset;
			unit.order = UnitOrder::MOVE;
		}
	}
}

void TinyGame::select_at(const Vector2 &p_position) {
	selected_base_owner = -1;
	selected_barracks_index = -1;
	drag_has_unit_type_filter = false;
	for (Unit &unit : units) {
		unit.selected = false;
	}

	float best_distance = 999999.0f;
	Unit *best_unit = nullptr;
	for (Unit &unit : units) {
		if (unit.owner != PLAYER) {
			continue;
		}
		const float distance = distance_to(p_position, unit.position);
		if (distance <= unit_radius(unit) + 10.0f && distance < best_distance) {
			best_distance = distance;
			best_unit = &unit;
		}
	}

	if (best_unit != nullptr) {
		best_unit->selected = true;
		return;
	}

	Base *player_base = find_base(PLAYER);
	if (player_base != nullptr && distance_to(p_position, player_base->position) <= BASE_RADIUS + 18.0f) {
		selected_base_owner = PLAYER;
		return;
	}

	for (int32_t i = 0; i < static_cast<int32_t>(barracks.size()); ++i) {
		if (barracks[i].owner == PLAYER && distance_to(p_position, barracks[i].position) <= BARRACKS_RADIUS + 18.0f) {
			selected_barracks_index = i;
			return;
		}
	}
}

void TinyGame::select_units_in_rect(const Rect2 &p_rect) {
	selected_base_owner = -1;
	selected_barracks_index = -1;
	for (Unit &unit : units) {
		unit.selected = unit.owner == PLAYER && p_rect.has_point(unit.position) && (!drag_has_unit_type_filter || unit.type == drag_unit_type_filter);
	}
	drag_has_unit_type_filter = false;
}

void TinyGame::select_all_units_of_type(UnitType p_type) {
	selected_base_owner = -1;
	selected_barracks_index = -1;
	for (Unit &unit : units) {
		unit.selected = unit.owner == PLAYER && unit.type == p_type;
	}
}

void TinyGame::remove_dead_units() {
	units.erase(std::remove_if(units.begin(), units.end(), [](const Unit &unit) { return unit.hp <= 0.0f; }), units.end());
}

void TinyGame::check_win_condition() {
	Base *player_base = find_base(PLAYER);
	Base *bot_base = find_base(BOT);
	if (player_base != nullptr && player_base->hp <= 0.0f) {
		winner_text = "Bot wins";
	} else if (bot_base != nullptr && bot_base->hp <= 0.0f) {
		winner_text = "Player wins";
	}
}

TinyGame::Unit *TinyGame::find_unit(int32_t p_id) {
	if (p_id == -1) {
		return nullptr;
	}
	for (Unit &unit : units) {
		if (unit.id == p_id) {
			return &unit;
		}
	}
	return nullptr;
}

TinyGame::Unit *TinyGame::find_player_unit_at(const Vector2 &p_position) {
	float best_distance = 999999.0f;
	Unit *best_unit = nullptr;
	for (Unit &unit : units) {
		if (unit.owner != PLAYER) {
			continue;
		}

		const float distance = distance_to(p_position, unit.position);
		if (distance <= unit_radius(unit) + 10.0f && distance < best_distance) {
			best_distance = distance;
			best_unit = &unit;
		}
	}
	return best_unit;
}

TinyGame::ResourceNode *TinyGame::find_resource(int32_t p_id) {
	for (ResourceNode &resource : resources) {
		if (resource.id == p_id) {
			return &resource;
		}
	}
	return nullptr;
}

TinyGame::Base *TinyGame::find_base(int32_t p_owner) {
	for (Base &base : bases) {
		if (base.owner == p_owner) {
			return &base;
		}
	}
	return nullptr;
}

TinyGame::Barracks *TinyGame::find_barracks(int32_t p_owner) {
	for (Barracks &building : barracks) {
		if (building.owner == p_owner) {
			return &building;
		}
	}
	return nullptr;
}

int32_t TinyGame::find_nearest_resource(const Vector2 &p_position) const {
	float best_distance = 999999.0f;
	int32_t best_id = -1;
	for (const ResourceNode &resource : resources) {
		if (resource.amount <= 0) {
			continue;
		}
		const float distance = distance_to(p_position, resource.position);
		if (distance < best_distance) {
			best_distance = distance;
			best_id = resource.id;
		}
	}
	return best_id;
}

int32_t TinyGame::find_enemy_unit_id(int32_t p_owner, const Vector2 &p_position, float p_range) const {
	float best_distance = 999999.0f;
	int32_t best_id = -1;
	for (const Unit &unit : units) {
		if (unit.owner == p_owner || unit.hp <= 0.0f) {
			continue;
		}
		const float distance = distance_to(p_position, unit.position);
		if (distance <= p_range && distance < best_distance) {
			best_distance = distance;
			best_id = unit.id;
		}
	}
	return best_id;
}

int32_t TinyGame::selected_unit_count() const {
	int32_t count = 0;
	for (const Unit &unit : units) {
		if (unit.selected) {
			count++;
		}
	}
	return count;
}

const TinyGame::Unit *TinyGame::first_selected_unit() const {
	for (const Unit &unit : units) {
		if (unit.selected) {
			return &unit;
		}
	}
	return nullptr;
}

Vector2 TinyGame::formation_offset(int32_t p_index, int32_t p_count) const {
	if (p_count <= 1) {
		return Vector2();
	}

	const int32_t columns = static_cast<int32_t>(std::ceil(std::sqrt(static_cast<float>(p_count))));
	const int32_t row = p_index / columns;
	const int32_t column = p_index % columns;
	const float spacing = 34.0f;
	const float width = static_cast<float>(columns - 1) * spacing;
	const int32_t rows = static_cast<int32_t>(std::ceil(static_cast<float>(p_count) / static_cast<float>(columns)));
	const float height = static_cast<float>(rows - 1) * spacing;

	return Vector2(static_cast<float>(column) * spacing - width * 0.5f, static_cast<float>(row) * spacing - height * 0.5f);
}

float TinyGame::unit_speed(const Unit &p_unit) const {
	return p_unit.type == UnitType::WORKER ? 120.0f : 95.0f;
}

float TinyGame::unit_radius(const Unit &p_unit) const {
	return p_unit.type == UnitType::WORKER ? 12.0f : 16.0f;
}

float TinyGame::unit_attack_range(const Unit &p_unit) const {
	return p_unit.type == UnitType::WORKER ? 24.0f : 34.0f;
}

float TinyGame::unit_attack_damage(const Unit &p_unit) const {
	return p_unit.type == UnitType::WORKER ? 4.0f : 14.0f;
}

Color TinyGame::owner_color(int32_t p_owner) const {
	return p_owner == PLAYER ? Color(0.25f, 0.55f, 1.0f) : Color(0.95f, 0.25f, 0.22f);
}

} // namespace tinyv1

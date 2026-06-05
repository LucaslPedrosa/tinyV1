#pragma once

#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <vector>

namespace tinyv1 {

using PlayerId = int32_t;
using UnitId = int32_t;
using BuildingId = int32_t;
using ResourceId = int32_t;

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
	BUILD,
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
	float train_duration = 0.0f;
	bool training_worker = false;
	int32_t worker_queue = 0;
	bool has_rally_point = false;
	godot::Vector2 rally_point;
};

struct Barracks {
	BuildingId id = -1;
	int32_t owner = 0;
	godot::Vector2 position;
	float hp = 250.0f;
	float build_progress = 0.0f;
	bool completed = false;
	float train_timer = 0.0f;
	float train_duration = 0.0f;
	bool training_fighter = false;
	int32_t fighter_queue = 0;
	bool has_rally_point = false;
	godot::Vector2 rally_point;
};

struct Unit {
	UnitId id = 0;
	int32_t owner = 0;
	UnitType type = UnitType::WORKER;
	UnitOrder order = UnitOrder::IDLE;
	godot::Vector2 position;
	godot::Vector2 target_position;
	float hp = 40.0f;
	float attack_timer = 0.0f;
	float gather_timer = 0.0f;
	bool gathering_resource = false;
	int32_t carrying = 0;
	int32_t target_resource = -1;
	int32_t target_unit_id = -1;
	int32_t target_base_owner = -1;
	BuildingId target_building_id = -1;
};

struct CommandFeedback {
	bool has_marker = false;
	godot::Vector2 marker_position;
};

struct SelectionResult {
	std::vector<int32_t> unit_ids;
	int32_t base_owner = -1;
	BuildingId building_id = -1;
};

} // namespace tinyv1

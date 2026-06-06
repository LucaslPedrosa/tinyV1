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

struct OwnerComponent {
	PlayerId owner = -1;
};

struct TransformComponent {
	godot::Vector2 position;
};

struct HealthComponent {
	float hp = 0.0f;
	float max_hp = 0.0f;
};

struct RallyPointComponent {
	bool has_rally_point = false;
	godot::Vector2 rally_point;
};

struct ProductionQueueComponent {
	int32_t queue = 0;
	bool training = false;
	float train_timer = 0.0f;
	float train_duration = 0.0f;
};

struct ConstructionComponent {
	float build_progress = 0.0f;
	bool completed = false;
};

struct MovementComponent {
	godot::Vector2 target_position;
};

struct CombatComponent {
	float attack_timer = 0.0f;
	int32_t target_unit_id = -1;
	int32_t target_base_owner = -1;
	BuildingId target_building_id = -1;
};

struct GatherComponent {
	float gather_timer = 0.0f;
	bool gathering_resource = false;
	int32_t carrying = 0;
	ResourceId target_resource = -1;
};

struct BuildComponent {
	BuildingId target_building_id = -1;
};

struct Base {
	OwnerComponent owner_component;
	TransformComponent transform_component;
	HealthComponent health_component;
	ProductionQueueComponent production_component;
	RallyPointComponent rally_component;
};

struct Barracks {
	BuildingId id = -1;
	OwnerComponent owner_component;
	TransformComponent transform_component;
	HealthComponent health_component;
	ConstructionComponent construction_component;
	ProductionQueueComponent production_component;
	RallyPointComponent rally_component;
};

struct Unit {
	UnitId id = 0;
	OwnerComponent owner_component;
	TransformComponent transform_component;
	HealthComponent health_component;
	UnitType type = UnitType::WORKER;
	UnitOrder order = UnitOrder::IDLE;
	MovementComponent movement_component;
	CombatComponent combat_component;
	GatherComponent gather_component;
	BuildComponent build_component;
};

struct UnitSummary {
	UnitId id = -1;
	PlayerId owner = -1;
	UnitType type = UnitType::WORKER;
	godot::Vector2 position;
	float hp = 0.0f;
	float max_hp = 0.0f;
};

struct ResourceSummary {
	ResourceId id = -1;
	godot::Vector2 position;
	int32_t amount = 0;
};

struct BaseSummary {
	PlayerId owner = -1;
	godot::Vector2 position;
	float hp = 0.0f;
	float max_hp = 400.0f;
	float train_timer = 0.0f;
	float train_duration = 0.0f;
	bool training_worker = false;
	int32_t worker_queue = 0;
	bool has_rally_point = false;
	godot::Vector2 rally_point;
};

struct BuildingSummary {
	BuildingId id = -1;
	PlayerId owner = -1;
	godot::Vector2 position;
	float hp = 0.0f;
	float max_hp = 250.0f;
	float build_progress = 0.0f;
	bool completed = false;
	float train_timer = 0.0f;
	float train_duration = 0.0f;
	bool training_fighter = false;
	int32_t fighter_queue = 0;
	bool has_rally_point = false;
	godot::Vector2 rally_point;
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

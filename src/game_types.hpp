#pragma once

#include <godot_cpp/variant/vector2.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace tinyv1
{

using PlayerId = int32_t;
using UnitId = int32_t;
using BuildingId = int32_t;
using ResourceId = int32_t;
using ObjectId = int32_t;

inline constexpr int32_t PRODUCTION_QUEUE_CAPACITY = 3;

enum class ResourceType
{
  FOOD,
  GOLD,
  WOOD,
  FAVOR
};

struct ResourceCost
{
  int32_t food = 0;
  int32_t wood = 0;
  int32_t gold = 0;
  int32_t favor = 0;
};

struct ResourceWallet
{
  int32_t food = 0;
  int32_t wood = 0;
  int32_t gold = 0;
  int32_t favor = 0;

  bool can_afford(const ResourceCost &p_cost) const
  {
    return food >= p_cost.food && wood >= p_cost.wood && gold >= p_cost.gold &&
           favor >= p_cost.favor;
  }

  void spend(const ResourceCost &p_cost)
  {
    food -= p_cost.food;
    wood -= p_cost.wood;
    gold -= p_cost.gold;
    favor -= p_cost.favor;
  }

  void add(const ResourceCost &p_amount)
  {
    food += p_amount.food;
    wood += p_amount.wood;
    gold += p_amount.gold;
    favor += p_amount.favor;
  }
};

enum class FootprintShape
{
  RECT,
  CIRCLE,
};

struct Footprint
{
  FootprintShape shape = FootprintShape::CIRCLE;
  float size = 0.0f;
};

enum class UnitType
{
  WORKER,
  FIGHTER,
  CAVALRY,
};

enum class BuildingType
{
  TOWN_CENTER,
  BARRACKS,
  STABLE,
};

enum class UnitOrder
{
  IDLE,
  MOVE,
  GATHER,
  RETURN,
  ATTACK,
  BUILD,
};

enum class AvailableActionType
{
  BUILD_BUILDING,
  TRAIN_UNIT,
  DELETE_OBJECT,
};

struct AvailableAction
{
  AvailableActionType type = AvailableActionType::BUILD_BUILDING;
  bool enabled = false;
  ResourceCost cost;
  UnitType unit_type = UnitType::WORKER;
  BuildingType building_type = BuildingType::BARRACKS;
  BuildingId source_building_id = -1;
};

struct ResourceNode
{
  int32_t id = 0;
  ResourceType type = ResourceType::GOLD;
  godot::Vector2 position;
  int32_t amount = 0;
};

struct OwnerComponent
{
  PlayerId owner = -1;
};

struct TransformComponent
{
  godot::Vector2 position;
};

struct HealthComponent
{
  float hp = 0.0f;
  float max_hp = 0.0f;
};

struct GameObject
{
  ObjectId id = -1;
  OwnerComponent owner_component;
  TransformComponent transform_component;
  HealthComponent health_component;
};

enum class RallyActionType
{
  NONE,
  MOVE,
  GATHER,
  ATTACK_MOVE,
  ATTACK_UNIT,
  ATTACK_BUILDING,
};

struct RallyActionComponent
{
  bool has_rally_action = false;
  RallyActionType type = RallyActionType::NONE;
  godot::Vector2 position;
  ResourceId target_resource = -1;
  UnitId target_unit_id = -1;
  BuildingId target_building_id = -1;
};

struct ProductionQueueComponent
{
  std::array<UnitType, PRODUCTION_QUEUE_CAPACITY> queued_units{};
  int32_t queue_start = 0;
  int32_t queue_count = 0;
  bool training = false;
  UnitType active_unit = UnitType::WORKER;
  float train_timer = 0.0f;
  float train_duration = 0.0f;

  bool is_full() const
  {
    return queue_count >= PRODUCTION_QUEUE_CAPACITY;
  }

  bool is_empty() const
  {
    return queue_count <= 0;
  }

  bool push(UnitType p_type)
  {
    if (is_full())
    {
      return false;
    }
    const int32_t index = (queue_start + queue_count) % PRODUCTION_QUEUE_CAPACITY;
    queued_units[index] = p_type;
    ++queue_count;
    return true;
  }

  bool pop(UnitType &r_type)
  {
    if (is_empty())
    {
      return false;
    }
    r_type = queued_units[queue_start];
    queue_start = (queue_start + 1) % PRODUCTION_QUEUE_CAPACITY;
    --queue_count;
    return true;
  }
};

struct ConstructionComponent
{
  float build_progress = 0.0f;
  bool completed = false;
};

struct MovementComponent
{
  godot::Vector2 target_position;
};

struct CombatComponent
{
  float attack_timer = 0.0f;
  int32_t target_unit_id = -1;
  BuildingId target_building_id = -1;
};

struct GatherRule
{
  ResourceType resource_type = ResourceType::GOLD;
  float gather_time = 0.0f;
  int32_t amount_per_trip = 0;
};

struct GatherComponent
{
  std::vector<GatherRule> rules;
  float gather_timer = 0.0f;
  bool gathering_resource = false;
  int32_t carrying = 0;
  ResourceType carrying_resource_type = ResourceType::GOLD;
  ResourceId target_resource = -1;
};

struct BuildComponent
{
  BuildingId target_building_id = -1;
};

struct Building
{
  BuildingId id = -1;
  BuildingType type = BuildingType::BARRACKS;
  OwnerComponent owner_component;
  TransformComponent transform_component;
  HealthComponent health_component;
  ConstructionComponent construction_component;
  ProductionQueueComponent production_component;
  RallyActionComponent rally_component;
};

struct Unit
{
  GameObject object;
  UnitType type = UnitType::WORKER;
  UnitOrder order = UnitOrder::IDLE;
  MovementComponent movement_component;
  CombatComponent combat_component;
  GatherComponent gather_component;
  BuildComponent build_component;
};

struct UnitSummary
{
  UnitId id = -1;
  PlayerId owner = -1;
  UnitType type = UnitType::WORKER;
  UnitOrder order = UnitOrder::IDLE;
  godot::Vector2 position;
  float hp = 0.0f;
  float max_hp = 0.0f;
  bool moving = false;
};

struct ResourceSummary
{
  ResourceId id = -1;
  godot::Vector2 position;
  int32_t amount = 0;
};

struct BuildingSummary
{
  BuildingId id = -1;
  BuildingType type = BuildingType::BARRACKS;
  PlayerId owner = -1;
  godot::Vector2 position;
  float hp = 0.0f;
  float max_hp = 0.0f;
  float build_progress = 0.0f;
  bool completed = false;
  float train_timer = 0.0f;
  float train_duration = 0.0f;
  bool training = false;
  UnitType active_unit = UnitType::WORKER;
  int32_t queue_count = 0;
  bool has_rally_point = false;
  godot::Vector2 rally_point;
};

struct CommandFeedback
{
  bool has_marker = false;
  godot::Vector2 marker_position;
};

struct SelectionResult
{
  std::vector<int32_t> unit_ids;
  BuildingId building_id = -1;
};

struct RenderSnapshot
{
  std::vector<ResourceSummary> resources;
  std::vector<BuildingSummary> buildings;
  std::vector<UnitSummary> units;
};

} // namespace tinyv1

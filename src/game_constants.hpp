#pragma once

#include "definitions/unit_definitions.hpp"
#include "game_types.hpp"

#include <godot_cpp/variant/vector2.hpp>

#include <cmath>
#include <cstdint>

namespace tinyv1
{

inline constexpr int32_t PLAYER = 0;
inline constexpr int32_t BOT = 1;
inline constexpr float RESOURCE_RADIUS = 40.0f;
inline constexpr float BASE_RADIUS = 150.0f;
inline constexpr float BARRACKS_RADIUS = 200.0f;

inline constexpr float GIANT_BUILDING_SIZE = 280.0f;
inline constexpr float BIG_BUILDING_SIZE = 240.0f;
inline constexpr float MEDIUM_BUILDING_SIZE = 180.0f;
inline constexpr float SMALL_BUILDING_SIZE = 128.0f;

inline constexpr float BIG_UNIT_RADIUS = 24.0f;
inline constexpr float MEDIUM_UNIT_RADIUS = 16.0f;
inline constexpr float SMALL_UNIT_RADIUS = 12.0f;

inline constexpr float BIG_UNIT_COLLISION_RADIUS = 9.0f;
inline constexpr float MEDIUM_UNIT_COLLISION_RADIUS = 7.0f;
inline constexpr float SMALL_UNIT_COLLISION_RADIUS = 5.0f;
inline constexpr int32_t MAX_TRAIN_QUEUE = PRODUCTION_QUEUE_CAPACITY;
inline constexpr float BOT_ATTACK_INTERVAL = 2.5f;
inline constexpr float BUILD_DISTANCE = 120.0f;
inline constexpr float GATHER_DISTANCE = 34.0f;
inline constexpr float RETURN_DISTANCE = 160.0f;
inline constexpr float GATHER_TIME = 1.0f;
inline constexpr int32_t WORKER_CARRY_LIMIT = 10;
inline constexpr float AUTO_AGGRO_RANGE = 96.0f;

inline bool rects_overlap(const godot::Vector2 &p_center_a, float p_half_size_a,
                          const godot::Vector2 &p_center_b, float p_half_size_b)
{
  return std::abs(p_center_a.x - p_center_b.x) < p_half_size_a + p_half_size_b &&
         std::abs(p_center_a.y - p_center_b.y) < p_half_size_a + p_half_size_b;
}

inline bool rect_circle_overlap(const godot::Vector2 &p_rect_center, float p_rect_half_size,
                                const godot::Vector2 &p_circle_center, float p_circle_radius)
{
  const float dx = std::abs(p_circle_center.x - p_rect_center.x) - p_rect_half_size;
  const float dy = std::abs(p_circle_center.y - p_rect_center.y) - p_rect_half_size;
  if (dx > 0.0f && dy > 0.0f)
  {
    return dx * dx + dy * dy < p_circle_radius * p_circle_radius;
  }
  if (dx > 0.0f)
  {
    return dx < p_circle_radius;
  }
  if (dy > 0.0f)
  {
    return dy < p_circle_radius;
  }
  return true;
}

inline float distance_to(const godot::Vector2 &a, const godot::Vector2 &b)
{
  return a.distance_to(b);
}

inline godot::Vector2 move_toward(const godot::Vector2 &from, const godot::Vector2 &to,
                                  float max_distance)
{
  const godot::Vector2 offset = to - from;
  const float length = offset.length();
  if (length <= max_distance || length <= 0.001f)
  {
    return to;
  }
  return from + offset / length * max_distance;
}

inline float unit_speed(const Unit &unit)
{
  return get_unit_definition(unit.type).movement_speed;
}

inline float unit_radius(const Unit &unit)
{
  return get_unit_definition(unit.type).selection_radius;
}

inline float unit_attack_range(const Unit &unit)
{
  return get_unit_definition(unit.type).attack_range;
}

inline float unit_attack_damage(const Unit &unit)
{
  return get_unit_definition(unit.type).attack_damage;
}

inline Footprint base_footprint()
{
  Footprint fp;
  fp.shape = FootprintShape::RECT;
  fp.size = BIG_BUILDING_SIZE * 0.5f;
  return fp;
}

inline Footprint medium_building_footprint()
{
  Footprint fp;
  fp.shape = FootprintShape::RECT;
  fp.size = MEDIUM_BUILDING_SIZE * 0.5f;
  return fp;
}

inline Footprint resource_footprint()
{
  Footprint fp;
  fp.shape = FootprintShape::CIRCLE;
  fp.size = RESOURCE_RADIUS;
  return fp;
}

inline Footprint unit_footprint(const Unit &p_unit)
{
  Footprint fp;
  fp.shape = FootprintShape::CIRCLE;
  fp.size = unit_radius(p_unit);
  return fp;
}

inline bool footprints_overlap(const godot::Vector2 &p_pos_a, const Footprint &p_fp_a,
                               const godot::Vector2 &p_pos_b, const Footprint &p_fp_b)
{
  if (p_fp_a.shape == FootprintShape::RECT && p_fp_b.shape == FootprintShape::RECT)
  {
    return rects_overlap(p_pos_a, p_fp_a.size, p_pos_b, p_fp_b.size);
  }
  if (p_fp_a.shape == FootprintShape::RECT && p_fp_b.shape == FootprintShape::CIRCLE)
  {
    return rect_circle_overlap(p_pos_a, p_fp_a.size, p_pos_b, p_fp_b.size);
  }
  if (p_fp_a.shape == FootprintShape::CIRCLE && p_fp_b.shape == FootprintShape::RECT)
  {
    return rect_circle_overlap(p_pos_b, p_fp_b.size, p_pos_a, p_fp_a.size);
  }
  const float dist_sq = p_pos_a.distance_squared_to(p_pos_b);
  const float combined = p_fp_a.size + p_fp_b.size;
  return dist_sq < combined * combined;
}

inline godot::Vector2 formation_offset(int32_t p_index, int32_t p_count)
{
  if (p_count <= 1)
  {
    return godot::Vector2();
  }

  const int32_t columns = static_cast<int32_t>(std::ceil(std::sqrt(static_cast<float>(p_count))));
  const int32_t row = p_index / columns;
  const int32_t column = p_index % columns;
  const float spacing = 34.0f;
  const float width = static_cast<float>(columns - 1) * spacing;
  const int32_t rows =
      static_cast<int32_t>(std::ceil(static_cast<float>(p_count) / static_cast<float>(columns)));
  const float height = static_cast<float>(rows - 1) * spacing;

  return godot::Vector2(static_cast<float>(column) * spacing - width * 0.5f,
                        static_cast<float>(row) * spacing - height * 0.5f);
}

} // namespace tinyv1

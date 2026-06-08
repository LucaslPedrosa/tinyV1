#pragma once

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
inline constexpr float WORKER_COST = 20.0f;
inline constexpr float FIGHTER_COST = 25.0f;
inline constexpr float BARRACKS_COST = 80.0f;
inline constexpr float WORKER_TRAIN_TIME = 4.0f;
inline constexpr float FIGHTER_TRAIN_TIME = 5.0f;
inline constexpr int32_t MAX_TRAIN_QUEUE = 3;
inline constexpr float BOT_ATTACK_INTERVAL = 2.5f;
inline constexpr float BARRACKS_BUILD_TIME = 8.0f;
inline constexpr float BUILD_DISTANCE = 120.0f;
inline constexpr float GATHER_DISTANCE = 34.0f;
inline constexpr float RETURN_DISTANCE = 160.0f;
inline constexpr float GATHER_TIME = 1.0f;
inline constexpr int32_t WORKER_CARRY_LIMIT = 10;

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
  return unit.type == UnitType::WORKER ? 120.0f : 95.0f;
}

inline float unit_radius(const Unit &unit)
{
  return unit.type == UnitType::WORKER ? 12.0f : 16.0f;
}

inline float unit_attack_range(const Unit &unit)
{
  return unit.type == UnitType::WORKER ? 24.0f : 34.0f;
}

inline float unit_attack_damage(const Unit &unit)
{
  return unit.type == UnitType::WORKER ? 4.0f : 14.0f;
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

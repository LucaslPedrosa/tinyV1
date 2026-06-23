#pragma once

#include "game_types.hpp"

#include <span>

namespace tinyv1
{

struct UnitDefinition
{
  UnitType type = UnitType::WORKER;
  const char *id = "worker";
  const char *display_name = "Worker";
  const char *description = "";
  const char *humanoid_asset_folder = "worker";
  ResourceCost cost;
  float max_hp = 0.0f;
  float movement_speed = 0.0f;
  float selection_radius = 0.0f;
  float collision_radius = 0.0f;
  float attack_range = 0.0f;
  float attack_damage = 0.0f;
  float attack_cooldown = 0.8f;
  float train_time = 0.0f;
  bool can_gather = false;
  bool can_attack_move = false;
  std::span<const BuildingType> buildable_buildings;
};

const UnitDefinition &get_unit_definition(UnitType p_type);

} // namespace tinyv1

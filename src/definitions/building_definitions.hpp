#pragma once

#include "game_types.hpp"

#include <span>

namespace tinyv1
{

struct BuildingDefinition
{
  BuildingType type = BuildingType::BARRACKS;
  const char *id = "barracks";
  const char *display_name = "Barracks";
  const char *description = "";
  const char *asset_name = "barracks";
  ResourceCost cost;
  float max_hp = 0.0f;
  float build_time = 0.0f;
  Footprint footprint;
  std::span<const UnitType> trainable_units;
};

const BuildingDefinition &get_building_definition(BuildingType p_type);

} // namespace tinyv1

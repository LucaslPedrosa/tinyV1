#pragma once

#include "game_types.hpp"

#include <span>

namespace tinyv1
{

enum class CivilizationType
{
  GREECE,
};

struct CivilizationDefinition
{
  CivilizationType type = CivilizationType::GREECE;
  const char *id = "greece";
  const char *display_name = "Greece";
  std::span<const UnitType> units;
  std::span<const BuildingType> buildings;
};

const CivilizationDefinition &get_civilization_definition(CivilizationType p_type);

} // namespace tinyv1

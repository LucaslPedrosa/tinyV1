#include "definitions/building_definitions.hpp"

#include "game_constants.hpp"

#include <array>

namespace tinyv1
{
namespace
{

const std::array<UnitType, 1> TOWN_CENTER_TRAINABLE_UNITS{UnitType::WORKER};
const std::array<UnitType, 1> BARRACKS_TRAINABLE_UNITS{UnitType::FIGHTER};
const std::array<UnitType, 1> STABLE_TRAINABLE_UNITS{UnitType::CAVALRY};

const BuildingDefinition TOWN_CENTER_DEFINITION{
    BuildingType::TOWN_CENTER,
    "towncenter",
    "Town Center",
    "Main base and worker production building.",
    "towncenter",
    {0, 0, 0, 0},
    400.0f,
    0.0f,
    base_footprint(),
    TOWN_CENTER_TRAINABLE_UNITS,
};

const BuildingDefinition BARRACKS_DEFINITION{
    BuildingType::BARRACKS,
    "barracks",
    "Barracks",
    "Trains infantry units.",
    "barracks",
    {0, 0, 80, 0},
    250.0f,
    8.0f,
    medium_building_footprint(),
    BARRACKS_TRAINABLE_UNITS,
};

const BuildingDefinition STABLE_DEFINITION{
    BuildingType::STABLE,
    "stable",
    "Stable",
    "Trains mounted units.",
    "stable",
    {0, 0, 120, 0},
    260.0f,
    10.0f,
    medium_building_footprint(),
    STABLE_TRAINABLE_UNITS,
};

} // namespace

const BuildingDefinition &get_building_definition(BuildingType p_type)
{
  switch (p_type)
  {
  case BuildingType::TOWN_CENTER:
    return TOWN_CENTER_DEFINITION;
  case BuildingType::BARRACKS:
    return BARRACKS_DEFINITION;
  case BuildingType::STABLE:
    return STABLE_DEFINITION;
  }
  return BARRACKS_DEFINITION;
}

} // namespace tinyv1

#include "definitions/unit_definitions.hpp"

#include "game_constants.hpp"

#include <array>

namespace tinyv1
{
namespace
{

const std::array<BuildingType, 2> WORKER_BUILDABLE_BUILDINGS{BuildingType::BARRACKS,
                                                             BuildingType::STABLE};
const std::array<BuildingType, 0> NO_BUILDABLE_BUILDINGS{};

const UnitDefinition WORKER_DEFINITION{
    UnitType::WORKER,
    "worker",
    "Worker",
    "Gathers resources and constructs buildings.",
    "worker",
    {0, 0, 20, 0},
    45.0f,
    120.0f,
    SMALL_UNIT_RADIUS,
    SMALL_UNIT_COLLISION_RADIUS,
    24.0f,
    4.0f,
    0.8f,
    4.0f,
    true,
    false,
    WORKER_BUILDABLE_BUILDINGS,
};

const UnitDefinition FIGHTER_DEFINITION{
    UnitType::FIGHTER,
    "hoplite",
    "Fighter",
    "Basic melee combat unit.",
    "hoplite",
    {0, 0, 25, 0},
    80.0f,
    95.0f,
    MEDIUM_UNIT_RADIUS,
    MEDIUM_UNIT_COLLISION_RADIUS,
    34.0f,
    14.0f,
    0.8f,
    5.0f,
    false,
    true,
    NO_BUILDABLE_BUILDINGS,
};

const UnitDefinition CAVALRY_DEFINITION{
    UnitType::CAVALRY,
    "cavalry",
    "Cavalry",
    "Fast mounted combat unit.",
    "cavalry",
    {0, 0, 60, 0},
    120.0f,
    135.0f,
    MEDIUM_UNIT_RADIUS,
    MEDIUM_UNIT_COLLISION_RADIUS,
    38.0f,
    18.0f,
    0.8f,
    8.0f,
    false,
    true,
    NO_BUILDABLE_BUILDINGS,
};

} // namespace

const UnitDefinition &get_unit_definition(UnitType p_type)
{
  switch (p_type)
  {
  case UnitType::WORKER:
    return WORKER_DEFINITION;
  case UnitType::FIGHTER:
    return FIGHTER_DEFINITION;
  case UnitType::CAVALRY:
    return CAVALRY_DEFINITION;
  }
  return WORKER_DEFINITION;
}

} // namespace tinyv1

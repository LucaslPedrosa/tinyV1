#include "definitions/civilization_definitions.hpp"

#include <array>

namespace tinyv1
{
namespace
{

const std::array<UnitType, 3> GREECE_UNITS{UnitType::WORKER, UnitType::FIGHTER,
                                           UnitType::CAVALRY};
const std::array<BuildingType, 3> GREECE_BUILDINGS{BuildingType::TOWN_CENTER,
                                                   BuildingType::BARRACKS,
                                                   BuildingType::STABLE};

const CivilizationDefinition GREECE_DEFINITION{
    CivilizationType::GREECE,
    "greece",
    "Greece",
    GREECE_UNITS,
    GREECE_BUILDINGS,
};

} // namespace

const CivilizationDefinition &get_civilization_definition(CivilizationType p_type)
{
  switch (p_type)
  {
  case CivilizationType::GREECE:
    return GREECE_DEFINITION;
  }
  return GREECE_DEFINITION;
}

} // namespace tinyv1

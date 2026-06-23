#pragma once

#include "game_types.hpp"

#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <vector>

namespace tinyv1
{

enum class GameCommandType
{
  COMMAND_SELECTED_TO,
  TRAIN_UNIT,
  PLACE_BUILDING,
  DELETE_OBJECT
};

struct GameCommand
{
  GameCommandType type = GameCommandType::COMMAND_SELECTED_TO;
  PlayerId owner = -1;
  godot::Vector2 position;
  std::vector<UnitId> selected_unit_ids;
  BuildingId selected_building_id = -1;
  BuildingType building_type = BuildingType::BARRACKS;
  UnitType unit_type = UnitType::WORKER;
  UnitId builder_unit_id = -1;
};

struct GameCommandResult
{
  CommandFeedback feedback;
  BuildingId placed_building_id = -1;
};

} // namespace tinyv1

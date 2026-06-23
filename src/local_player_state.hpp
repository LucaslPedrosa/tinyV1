#pragma once

#include "game_types.hpp"

#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace tinyv1
{

struct LocalPlayerState
{
  std::vector<int32_t> selected_unit_ids;
  BuildingId selected_building_id = -1;
  bool is_placing_building = false;
  BuildingType placing_building_type = BuildingType::BARRACKS;
  bool is_drag_selecting = false;
  bool drag_has_unit_type_filter = false;
  UnitType drag_unit_type_filter = UnitType::WORKER;
  godot::Vector2 drag_start;
  godot::Vector2 drag_current;
  godot::Vector2 command_marker_position;
  float command_marker_timer = 0.0f;

  bool is_unit_selected(int32_t p_unit_id) const
  {
    return std::find(selected_unit_ids.begin(), selected_unit_ids.end(), p_unit_id) !=
           selected_unit_ids.end();
  }

  void set_selection(const SelectionResult &p_selection)
  {
    selected_unit_ids = p_selection.unit_ids;
    selected_building_id = p_selection.building_id;
  }

  void clear_selection()
  {
    selected_unit_ids.clear();
    selected_building_id = -1;
  }

  void reset()
  {
    clear_selection();
    is_placing_building = false;
    placing_building_type = BuildingType::BARRACKS;
    is_drag_selecting = false;
    drag_has_unit_type_filter = false;
    drag_unit_type_filter = UnitType::WORKER;
    drag_start = godot::Vector2();
    drag_current = godot::Vector2();
    command_marker_position = godot::Vector2();
    command_marker_timer = 0.0f;
  }
};

} // namespace tinyv1

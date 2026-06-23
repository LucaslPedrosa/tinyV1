#pragma once

#include <godot_cpp/variant/color.hpp>

namespace tinyv1::render_constants
{

constexpr float WORKER_RADIUS = 64.0f;
constexpr float HOPLITE_RADIUS = 64.0f;
constexpr float WORKER_SPRITE_SIZE = 64.0f;
constexpr float HOPLITE_SPRITE_SIZE = 64.0f;
constexpr float UNIT_BASELINE_OFFSET = 6.0f;

constexpr float TOWN_CENTER_SPRITE_SIZE = 256.0f;
constexpr float BARRACKS_SPRITE_SIZE = 256.0f;
constexpr float GOLD_MINE_SPRITE_SIZE = 128.0f;

constexpr float UNIT_HP_BAR_WIDTH = 64.0f;
constexpr float UNIT_HP_BAR_HEIGHT = 8.0f;
constexpr float UNIT_HP_BAR_Y_OFFSET = -8.0f;

constexpr float SELECTED_UNIT_RING_Y_OFFSET = 8.0f;
constexpr float SELECTED_UNIT_RING_PADDING = 8.0f;
constexpr float SELECTED_BUILDING_OUTLINE_PADDING = 8.0f;
constexpr float SELECTED_OUTLINE_WIDTH = 3.0f;

inline const godot::Color SELECTED_HIGHLIGHT_COLOR = godot::Color(1.0f, 0.95f, 0.25f, 0.95f);

constexpr float PLACEMENT_BARRACKS_OUTLINE_PADDING = 4.0f;

} // namespace tinyv1::render_constants

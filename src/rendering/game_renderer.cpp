#include "rendering/game_renderer.hpp"

#include "definitions/building_definitions.hpp"
#include "game_constants.hpp"
#include "game_simulation/game_simulation.hpp"
#include "local_player_state.hpp"
#include "rendering/render_constants.hpp"
#include "rendering/texture_loader.hpp"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace tinyv1
{

namespace
{

Vector2 pixel_snap(const Vector2 &p_position)
{
  return Vector2(std::round(p_position.x), std::round(p_position.y));
}

} // namespace

GameRenderer::GameRenderer()
{
  render_snapshot.resources.reserve(8);
  render_snapshot.buildings.reserve(4);
  render_snapshot.units.reserve(32);
}

void GameRenderer::load_textures()
{
  greece_renderer.load_textures();
  goldmine_texture = load_texture(asset_contracts::world_resource_sprite("gold"));
}

void GameRenderer::draw(Node2D &p_canvas, const GameSimulation &p_simulation,
                        const LocalPlayerState &p_local, float p_animation_time) const
{
  p_simulation.build_render_snapshot(render_snapshot);

  const Rect2 map_rect = p_simulation.get_map_rect();
  p_canvas.draw_rect(map_rect, Color(0.18f, 0.45f, 0.18f), true);
  p_canvas.draw_rect(map_rect, Color(0.08f, 0.28f, 0.09f), false, 3.0f);

  for (const ResourceSummary &resource : render_snapshot.resources)
  {
    if (resource.amount <= 0)
    {
      continue;
    }

    if (goldmine_texture.is_valid())
    {
      p_canvas.draw_texture_rect(
          goldmine_texture,
          Rect2(pixel_snap(resource.position -
                           Vector2(render_constants::GOLD_MINE_SPRITE_SIZE * 0.5f,
                                   render_constants::GOLD_MINE_SPRITE_SIZE * 0.5f)),
                Vector2(render_constants::GOLD_MINE_SPRITE_SIZE,
                        render_constants::GOLD_MINE_SPRITE_SIZE)),
          false);
    }
    else
    {
      p_canvas.draw_circle(resource.position, RESOURCE_RADIUS, Color(0.15f, 0.70f, 0.85f));
      p_canvas.draw_circle(resource.position, RESOURCE_RADIUS * 0.55f, Color(0.75f, 0.95f, 1.0f));
    }
  }

  for (const BuildingSummary &building : render_snapshot.buildings)
  {
    greece_renderer.draw_building(p_canvas, building);
    if (!building.completed)
    {
      p_canvas.draw_rect(Rect2(building.position - Vector2(104, 104), Vector2(208, 208)),
                         Color(0.04f, 0.04f, 0.04f), false, 2.0f);
    }
    if (building.id == p_local.selected_building_id || building.hp < building.max_hp)
    {
      const float hp_ratio = std::max(0.0f, building.hp / building.max_hp);
      p_canvas.draw_rect(Rect2(building.position + Vector2(-70, -118), Vector2(140, 8)),
                         Color(0.04f, 0.04f, 0.04f), true);
      p_canvas.draw_rect(Rect2(building.position + Vector2(-70, -118), Vector2(140 * hp_ratio, 8)),
                         Color(0.2f, 0.9f, 0.3f), true);
    }
    if (!building.completed)
    {
      p_canvas.draw_rect(Rect2(building.position + Vector2(-70, 108), Vector2(140, 8)),
                         Color(0.04f, 0.04f, 0.04f), true);
      p_canvas.draw_rect(Rect2(building.position + Vector2(-70, 108),
                               Vector2(140.0f * building.build_progress, 8)),
                         Color(0.95f, 0.75f, 0.25f), true);
    }
    else if (building.training && building.train_duration > 0.0f)
    {
      const float progress = 1.0f - std::max(0.0f, building.train_timer / building.train_duration);
      p_canvas.draw_rect(Rect2(building.position + Vector2(-70, 108), Vector2(140, 8)),
                         Color(0.04f, 0.04f, 0.04f), true);
      p_canvas.draw_rect(
          Rect2(building.position + Vector2(-70, 108), Vector2(140.0f * progress, 8)),
          Color(0.35f, 0.65f, 1.0f), true);
    }
    if (building.id == p_local.selected_building_id)
    {
      const float side =
          MEDIUM_BUILDING_SIZE + render_constants::SELECTED_BUILDING_OUTLINE_PADDING * 2.0f;
      p_canvas.draw_rect(
          Rect2(building.position - Vector2(side * 0.5f, side * 0.5f), Vector2(side, side)),
          render_constants::SELECTED_HIGHLIGHT_COLOR, false,
          render_constants::SELECTED_OUTLINE_WIDTH);
    }
    if (building.id == p_local.selected_building_id && building.has_rally_point)
    {
      p_canvas.draw_rect(Rect2(building.rally_point - Vector2(6, 6), Vector2(12, 12)),
                         Color(1.0f, 0.1f, 0.1f), true);
      p_canvas.draw_line(building.position, building.rally_point, Color(1.0f, 0.1f, 0.1f, 0.55f),
                         2.0f);
    }
  }

  if (p_local.command_marker_timer > 0.0f)
  {
    const float alpha = std::min(1.0f, p_local.command_marker_timer / 0.45f);
    p_canvas.draw_rect(Rect2(p_local.command_marker_position - Vector2(6, 6), Vector2(12, 12)),
                       Color(1.0f, 0.1f, 0.1f, alpha), true);
    p_canvas.draw_rect(Rect2(p_local.command_marker_position - Vector2(8, 8), Vector2(16, 16)),
                       Color(0.25f, 0.0f, 0.0f, alpha), false, 2.0f);
  }

  for (const UnitSummary &unit : render_snapshot.units)
  {
    const float radius = greece_renderer.unit_radius(unit);
    const bool selected = p_local.is_unit_selected(unit.id);

    if (selected)
    {
      p_canvas.draw_arc(unit.position + Vector2(0, render_constants::SELECTED_UNIT_RING_Y_OFFSET),
                        radius + render_constants::SELECTED_UNIT_RING_PADDING, 0.0f, Math_TAU, 48,
                        render_constants::SELECTED_HIGHLIGHT_COLOR,
                        render_constants::SELECTED_OUTLINE_WIDTH);
    }

    greece_renderer.draw_unit(p_canvas, unit, p_animation_time);
    if (selected || unit.hp < unit.max_hp)
    {
      const float hp_ratio = std::max(0.0f, unit.hp / unit.max_hp);
      const Vector2 hp_position =
          greece_renderer.unit_top_left(unit) + Vector2(0, render_constants::UNIT_HP_BAR_Y_OFFSET);
      p_canvas.draw_rect(Rect2(hp_position, Vector2(render_constants::UNIT_HP_BAR_WIDTH,
                                                    render_constants::UNIT_HP_BAR_HEIGHT)),
                         Color(0.04f, 0.04f, 0.04f), true);
      p_canvas.draw_rect(Rect2(hp_position, Vector2(render_constants::UNIT_HP_BAR_WIDTH * hp_ratio,
                                                    render_constants::UNIT_HP_BAR_HEIGHT)),
                         Color(0.2f, 0.9f, 0.3f), true);
    }
  }

  if (p_local.is_drag_selecting)
  {
    const Vector2 top_left(std::min(p_local.drag_start.x, p_local.drag_current.x),
                           std::min(p_local.drag_start.y, p_local.drag_current.y));
    const Vector2 size(std::abs(p_local.drag_current.x - p_local.drag_start.x),
                       std::abs(p_local.drag_current.y - p_local.drag_start.y));
    const Rect2 selection_rect(top_left, size);
    p_canvas.draw_rect(selection_rect, Color(0.25f, 0.55f, 1.0f, 0.16f), true);
    p_canvas.draw_rect(selection_rect, Color(0.65f, 0.85f, 1.0f), false, 2.0f);
  }

  if (p_local.is_placing_building)
  {
    const Vector2 mouse_position = p_canvas.get_global_mouse_position();
    const BuildingDefinition &definition = get_building_definition(p_local.placing_building_type);
    const bool valid = !p_simulation.does_overlap(mouse_position, definition.footprint);
    const Vector2 collision_size(MEDIUM_BUILDING_SIZE, MEDIUM_BUILDING_SIZE);
    const Vector2 outline_size =
        collision_size + Vector2(render_constants::PLACEMENT_BARRACKS_OUTLINE_PADDING * 2.0f,
                                 render_constants::PLACEMENT_BARRACKS_OUTLINE_PADDING * 2.0f);
    const Color fill_color =
        valid ? Color(0.35f, 0.75f, 1.0f, 0.35f) : Color(0.9f, 0.2f, 0.2f, 0.35f);
    const Color outline_color = valid ? Color(0.75f, 0.95f, 1.0f) : Color(0.95f, 0.3f, 0.3f);
    p_canvas.draw_rect(Rect2(mouse_position - collision_size * 0.5f, collision_size), fill_color,
                       true);
    p_canvas.draw_rect(Rect2(mouse_position - outline_size * 0.5f, outline_size), outline_color,
                       false, 2.0f);
  }

  if (!p_simulation.get_winner_text().is_empty())
  {
    p_canvas.draw_rect(Rect2(Vector2(390, 300), Vector2(500, 120)),
                       Color(0.02f, 0.02f, 0.02f, 0.85f), true);
  }
}

} // namespace tinyv1

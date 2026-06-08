#include "rendering/greece_renderer.hpp"

#include "game_constants.hpp"
#include "rendering/render_constants.hpp"
#include "rendering/texture_loader.hpp"

#include <algorithm>
#include <cmath>

using namespace godot;

namespace tinyv1
{

namespace
{

enum HumanoidPart : size_t {
  HEAD,
  TORSO,
  LEFT_SHOULDER,
  LEFT_ARM,
  LEFT_LEG,
  RIGHT_SHOULDER,
  RIGHT_ARM,
  RIGHT_LEG,
};

Color owner_color(PlayerId p_owner)
{
  return p_owner == PLAYER ? Color(0.25f, 0.55f, 1.0f) : Color(0.95f, 0.25f, 0.22f);
}

Vector2 pixel_snap(const Vector2 &p_position)
{
  return Vector2(std::round(p_position.x), std::round(p_position.y));
}

} // namespace

bool GreeceRenderer::HumanoidTextures::has_body() const
{
  return std::all_of(body_parts.begin(), body_parts.end(), [](const Ref<Texture2D> &p_texture) {
    return p_texture.is_valid();
  });
}

void GreeceRenderer::load_textures()
{
  load_humanoid_textures("worker", worker_textures);
  worker_textures.right_hand_item = load_texture(asset_contracts::humanoid_item("tools", "pickaxe"));

  load_humanoid_textures("hoplite", hoplite_textures);
  hoplite_textures.right_hand_item = load_texture(asset_contracts::humanoid_item("weapons", "spear"));
  hoplite_textures.left_hand_item = load_texture(asset_contracts::humanoid_item("weapons", "shield"));

  town_center_texture = load_texture(asset_contracts::building_sprite("towncenter"));
  barracks_texture = load_texture(asset_contracts::building_sprite("barracks"));
}

void GreeceRenderer::load_humanoid_textures(const char *p_unit_folder,
                                            HumanoidTextures &r_textures) const
{
  for (size_t i = 0; i < asset_contracts::HUMANOID_PART_FILES.size(); ++i)
  {
    r_textures.body_parts[i] = load_texture(asset_contracts::humanoid_part(p_unit_folder, i));
  }
}

void GreeceRenderer::draw_base(Node2D &p_canvas, const BaseSummary &p_base) const
{
  const Color color = owner_color(p_base.owner);
  if (town_center_texture.is_valid())
  {
    p_canvas.draw_texture_rect(town_center_texture,
                               Rect2(pixel_snap(p_base.position -
                                          Vector2(render_constants::TOWN_CENTER_SPRITE_SIZE * 0.5f,
                                                  render_constants::TOWN_CENTER_SPRITE_SIZE * 0.5f)),
                                      Vector2(render_constants::TOWN_CENTER_SPRITE_SIZE,
                                              render_constants::TOWN_CENTER_SPRITE_SIZE)),
                               false);
    return;
  }

  p_canvas.draw_circle(p_base.position, BASE_RADIUS, color.darkened(0.25f));
  p_canvas.draw_rect(Rect2(p_base.position - Vector2(120, 110), Vector2(240, 220)), color, true);
  p_canvas.draw_rect(Rect2(p_base.position - Vector2(122, 112), Vector2(244, 224)),
                     Color(0.04f, 0.04f, 0.04f), false, 2.0f);
}

void GreeceRenderer::draw_barracks(Node2D &p_canvas, const BuildingSummary &p_building) const
{
  const Color color = owner_color(p_building.owner).darkened(0.1f);
  const Color sprite_modulate =
      p_building.completed ? Color(1, 1, 1, 1) : Color(1, 1, 1, 0.45f);

  if (barracks_texture.is_valid())
  {
    p_canvas.draw_texture_rect(barracks_texture,
                               Rect2(pixel_snap(p_building.position -
                                          Vector2(render_constants::BARRACKS_SPRITE_SIZE * 0.5f,
                                                  render_constants::BARRACKS_SPRITE_SIZE * 0.5f)),
                                      Vector2(render_constants::BARRACKS_SPRITE_SIZE,
                                              render_constants::BARRACKS_SPRITE_SIZE)),
                               false, sprite_modulate);
    return;
  }

  p_canvas.draw_rect(Rect2(p_building.position - Vector2(88, 78), Vector2(176, 156)),
                     p_building.completed ? color : color.lightened(0.35f), true);
  p_canvas.draw_line(p_building.position + Vector2(-64, 0),
                     p_building.position + Vector2(64, 0), Color(0.05f, 0.05f, 0.05f), 4.0f);
}

float GreeceRenderer::unit_radius(const UnitSummary &p_unit) const
{
  return p_unit.type == UnitType::WORKER ? render_constants::WORKER_RADIUS
                                         : render_constants::HOPLITE_RADIUS;
}

Vector2 GreeceRenderer::unit_size(const UnitSummary &p_unit) const
{
  const float size = p_unit.type == UnitType::WORKER ? render_constants::WORKER_SPRITE_SIZE
                                                     : render_constants::HOPLITE_SPRITE_SIZE;
  return Vector2(size, size);
}

Vector2 GreeceRenderer::unit_top_left(const UnitSummary &p_unit) const
{
  const Vector2 size = unit_size(p_unit);
  return p_unit.position - Vector2(size.x * 0.5f, size.y - render_constants::UNIT_BASELINE_OFFSET);
}

HumanoidPose GreeceRenderer::humanoid_pose_for(const UnitSummary &p_unit,
                                               float p_animation_time) const
{
  HumanoidPose pose;
  if (!p_unit.moving && p_unit.order == UnitOrder::GATHER)
  {
    const float bob =
        std::round(std::sin(p_animation_time * 8.0f + static_cast<float>(p_unit.id) * 0.2f) * 1.5f);
    pose.right_arm.offset = Vector2(2.0f, bob);
  }
  else if (!p_unit.moving && p_unit.order == UnitOrder::ATTACK)
  {
    pose.right_arm.offset = Vector2(4.0f, -1.0f);
  }
  return pose;
}

Vector2 GreeceRenderer::bone_position(const HumanoidPose &p_pose, HumanoidBone p_bone,
                                      const Vector2 &p_top_left) const
{
  switch (p_bone)
  {
  case HumanoidBone::TORSO:
    return p_top_left + Vector2(0.0f, 0.0f) + p_pose.torso.offset;
  case HumanoidBone::HEAD:
    return p_top_left + Vector2(0.0f, -8.0f) + p_pose.head.offset;
  case HumanoidBone::LEFT_SHOULDER:
    return p_top_left + Vector2(6.0f, 8.0f) + p_pose.left_shoulder.offset;
  case HumanoidBone::RIGHT_SHOULDER:
    return p_top_left + Vector2(18.0f, 8.0f) + p_pose.right_shoulder.offset;
  case HumanoidBone::LEFT_ARM:
    return p_top_left + Vector2(4.0f, 12.0f) + p_pose.left_arm.offset;
  case HumanoidBone::RIGHT_ARM:
    return p_top_left + Vector2(16.0f, 12.0f) + p_pose.right_arm.offset;
  case HumanoidBone::LEFT_LEG:
    return p_top_left + Vector2(5.0f, 20.0f) + p_pose.left_leg.offset;
  case HumanoidBone::RIGHT_LEG:
    return p_top_left + Vector2(15.0f, 20.0f) + p_pose.right_leg.offset;
  case HumanoidBone::ROOT:
  default:
    return p_top_left + p_pose.root.offset;
  }
}

Vector2 GreeceRenderer::socket_position(const HumanoidPose &p_pose, HumanoidSocket p_socket,
                                        const Vector2 &p_top_left) const
{
  switch (p_socket)
  {
  case HumanoidSocket::TORSO:
    return bone_position(p_pose, HumanoidBone::TORSO, p_top_left) + Vector2(8.0f, 12.0f);
  case HumanoidSocket::HEAD:
    return bone_position(p_pose, HumanoidBone::HEAD, p_top_left) + Vector2(8.0f, 8.0f);
  case HumanoidSocket::LEFT_HAND:
    return bone_position(p_pose, HumanoidBone::LEFT_ARM, p_top_left) + Vector2(6.0f, 16.0f);
  case HumanoidSocket::RIGHT_HAND:
    return bone_position(p_pose, HumanoidBone::RIGHT_ARM, p_top_left) + Vector2(6.0f, 16.0f);
  case HumanoidSocket::LEFT_SHOULDER:
    return bone_position(p_pose, HumanoidBone::LEFT_SHOULDER, p_top_left);
  case HumanoidSocket::RIGHT_SHOULDER:
    return bone_position(p_pose, HumanoidBone::RIGHT_SHOULDER, p_top_left);
  default:
    return p_top_left;
  }
}

void GreeceRenderer::draw_humanoid(Node2D &p_canvas, const UnitSummary &p_unit,
                                   const HumanoidTextures &p_textures,
                                   float p_animation_time) const
{
  const Vector2 top_left = pixel_snap(unit_top_left(p_unit));
  const Vector2 size = unit_size(p_unit);
  const HumanoidPose pose = humanoid_pose_for(p_unit, p_animation_time);

  p_canvas.draw_texture_rect(p_textures.body_parts[LEFT_LEG],
                             Rect2(pixel_snap(top_left + pose.left_leg.offset), size), false);
  p_canvas.draw_texture_rect(p_textures.body_parts[RIGHT_LEG],
                             Rect2(pixel_snap(top_left + pose.right_leg.offset), size), false);
  p_canvas.draw_texture_rect(p_textures.body_parts[TORSO], Rect2(pixel_snap(top_left + pose.torso.offset), size),
                             false);
  p_canvas.draw_texture_rect(p_textures.body_parts[LEFT_SHOULDER],
                             Rect2(pixel_snap(top_left + pose.left_shoulder.offset), size), false);
  p_canvas.draw_texture_rect(p_textures.body_parts[RIGHT_SHOULDER],
                             Rect2(pixel_snap(top_left + pose.right_shoulder.offset), size), false);
  p_canvas.draw_texture_rect(
      p_textures.body_parts[LEFT_ARM],
      Rect2(pixel_snap(top_left + pose.left_shoulder.offset + pose.left_arm.offset), size), false);
  p_canvas.draw_texture_rect(
      p_textures.body_parts[RIGHT_ARM],
      Rect2(pixel_snap(top_left + pose.right_shoulder.offset + pose.right_arm.offset), size), false);
  p_canvas.draw_texture_rect(p_textures.body_parts[HEAD], Rect2(pixel_snap(top_left + pose.head.offset), size),
                             false);

  if (p_textures.right_hand_item.is_valid())
  {
    const Vector2 item_size = p_textures.right_hand_item->get_size();
    const Vector2 item_position =
        pixel_snap(socket_position(pose, HumanoidSocket::RIGHT_HAND, top_left) - item_size * 0.5f);
    p_canvas.draw_texture_rect(p_textures.right_hand_item, Rect2(item_position, item_size), false);
  }
  if (p_textures.left_hand_item.is_valid())
  {
    const Vector2 item_size = p_textures.left_hand_item->get_size();
    const Vector2 item_position =
        pixel_snap(socket_position(pose, HumanoidSocket::LEFT_HAND, top_left) - item_size * 0.5f);
    p_canvas.draw_texture_rect(p_textures.left_hand_item, Rect2(item_position, item_size), false);
  }
}

void GreeceRenderer::draw_worker_unit(Node2D &p_canvas, const UnitSummary &p_unit,
                                      float p_animation_time) const
{
  if (worker_textures.has_body())
  {
    draw_humanoid(p_canvas, p_unit, worker_textures, p_animation_time);
    return;
  }

  p_canvas.draw_circle(p_unit.position, unit_radius(p_unit), owner_color(p_unit.owner));
}

void GreeceRenderer::draw_fighter_unit(Node2D &p_canvas, const UnitSummary &p_unit,
                                       float p_animation_time) const
{
  if (hoplite_textures.has_body())
  {
    draw_humanoid(p_canvas, p_unit, hoplite_textures, p_animation_time);
    return;
  }

  p_canvas.draw_circle(p_unit.position, unit_radius(p_unit), owner_color(p_unit.owner));
  p_canvas.draw_line(p_unit.position + Vector2(-unit_radius(p_unit), -unit_radius(p_unit)),
                     p_unit.position + Vector2(unit_radius(p_unit), unit_radius(p_unit)),
                     Color(0.05f, 0.05f, 0.05f), 3.0f);
}

void GreeceRenderer::draw_unit(Node2D &p_canvas, const UnitSummary &p_unit,
                               float p_animation_time) const
{
  if (p_unit.type == UnitType::WORKER)
  {
    draw_worker_unit(p_canvas, p_unit, p_animation_time);
    return;
  }

  draw_fighter_unit(p_canvas, p_unit, p_animation_time);
}

} // namespace tinyv1

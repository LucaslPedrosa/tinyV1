#include "rendering/greece_renderer.hpp"

#include "game_constants.hpp"
#include "rendering/render_constants.hpp"
#include "rendering/render_helpers.hpp"
#include "rendering/texture_loader.hpp"

#include <algorithm>
#include <cmath>

using namespace godot;

namespace tinyv1
{

namespace
{

enum HumanoidPart : size_t
{
  LEFT_ARM,
  RIGHT_ARM,
  LEFT_SHOULDER,
  RIGHT_SHOULDER,
  LEFT_LEG,
  RIGHT_LEG,
  HEAD,
  TORSO,
};

Color owner_color(PlayerId p_owner)
{
  return p_owner == PLAYER ? Color(0.25f, 0.55f, 1.0f) : Color(0.95f, 0.25f, 0.22f);
}

} // namespace

bool GreeceRenderer::HumanoidTextures::has_body() const
{
  return body_ready;
}

void GreeceRenderer::load_textures()
{
  load_humanoid_textures("worker", worker_textures);
  add_humanoid_attachment(worker_textures, asset_contracts::humanoid_item("tools", "pickaxe"),
                          HumanoidBone::RIGHT_ARM);

  load_humanoid_textures("hoplite", hoplite_textures);
  add_humanoid_attachment(hoplite_textures,
                          asset_contracts::humanoid_item("weapons", "spear_1wood"),
                          HumanoidBone::LEFT_ARM);
  add_humanoid_attachment(hoplite_textures,
                          asset_contracts::humanoid_item("weapons", "shield_3gold"),
                          HumanoidBone::RIGHT_ARM);

  town_center_texture = load_texture(asset_contracts::building_sprite("towncenter"));
  barracks_texture = load_texture(asset_contracts::building_sprite("barracks"));
}

void GreeceRenderer::load_humanoid_textures(const char *p_unit_folder,
                                            HumanoidTextures &r_textures) const
{
  r_textures.body_ready = true;
  r_textures.attachment_count = 0;
  for (size_t i = 0; i < asset_contracts::HUMANOID_PART_FILES.size(); ++i)
  {
    r_textures.body_parts[i] = load_texture(asset_contracts::humanoid_part(p_unit_folder, i));
    if (!r_textures.body_parts[i].is_valid())
    {
      r_textures.body_ready = false;
    }
  }
}

void GreeceRenderer::add_humanoid_attachment(HumanoidTextures &r_textures, const String &p_path,
                                             HumanoidBone p_follow_bone) const
{
  if (r_textures.attachment_count >= r_textures.attachments.size())
  {
    return;
  }

  Ref<Texture2D> texture = load_texture(p_path);
  if (!texture.is_valid())
  {
    return;
  }

  r_textures.attachments[r_textures.attachment_count] = {texture, p_follow_bone};
  ++r_textures.attachment_count;
}

void GreeceRenderer::draw_building(Node2D &p_canvas, const BuildingSummary &p_building) const
{
  const Color color = owner_color(p_building.owner).darkened(0.1f);
  const Color sprite_modulate = p_building.completed ? Color(1, 1, 1, 1) : Color(1, 1, 1, 0.45f);

  if (p_building.type == BuildingType::TOWN_CENTER)
  {
    if (town_center_texture.is_valid())
    {
      p_canvas.draw_texture_rect(
          town_center_texture,
          Rect2(pixel_snap(p_building.position -
                           Vector2(render_constants::TOWN_CENTER_SPRITE_SIZE * 0.5f,
                                   render_constants::TOWN_CENTER_SPRITE_SIZE * 0.5f)),
                Vector2(render_constants::TOWN_CENTER_SPRITE_SIZE,
                        render_constants::TOWN_CENTER_SPRITE_SIZE)),
          false);
      return;
    }

    p_canvas.draw_circle(p_building.position, BASE_RADIUS, color.darkened(0.25f));
    p_canvas.draw_rect(Rect2(p_building.position - Vector2(120, 110), Vector2(240, 220)), color,
                       true);
    p_canvas.draw_rect(Rect2(p_building.position - Vector2(122, 112), Vector2(244, 224)),
                       Color(0.04f, 0.04f, 0.04f), false, 2.0f);
    return;
  }

  if (barracks_texture.is_valid())
  {
    p_canvas.draw_texture_rect(
        barracks_texture,
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
  p_canvas.draw_line(p_building.position + Vector2(-64, 0), p_building.position + Vector2(64, 0),
                     Color(0.05f, 0.05f, 0.05f), 4.0f);
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

HumanoidPose GreeceRenderer::setup_pose_for(UnitType p_type) const
{
  HumanoidPose pose;
  if (p_type == UnitType::FIGHTER)
  {
    pose.left_arm.rotation = -90;
    pose.left_arm.offset += Vector2(-15.0f, -7.0f);
  }
  return pose;
}

HumanoidPose GreeceRenderer::humanoid_pose_for(const UnitSummary &p_unit,
                                               float p_animation_time) const
{
  HumanoidPose pose = setup_pose_for(p_unit.type);
  if (!p_unit.moving && p_unit.order == UnitOrder::GATHER)
  {
    const float bob =
        std::round(std::sin(p_animation_time * 8.0f + static_cast<float>(p_unit.id) * 0.2f) * 1.5f);
    pose.right_arm.offset += Vector2(2.0f, bob);
  }
  else if (!p_unit.moving && p_unit.order == UnitOrder::ATTACK)
  {
    pose.right_arm.offset += Vector2(4.0f, -1.0f);
  }
  return pose;
}

void GreeceRenderer::draw_humanoid(Node2D &p_canvas, const UnitSummary &p_unit,
                                   const HumanoidTextures &p_textures, float p_animation_time) const
{
  const Vector2 top_left = pixel_snap(unit_top_left(p_unit));
  const Vector2 size = unit_size(p_unit);
  const HumanoidPose pose = humanoid_pose_for(p_unit, p_animation_time);

  draw_bone_texture(p_canvas, p_textures.body_parts[LEFT_LEG], top_left + pose.left_leg.offset,
                    size, pose.left_leg.rotation);
  draw_bone_texture(p_canvas, p_textures.body_parts[RIGHT_LEG], top_left + pose.right_leg.offset,
                    size, pose.right_leg.rotation);
  draw_bone_texture(p_canvas, p_textures.body_parts[LEFT_SHOULDER],
                    top_left + pose.left_shoulder.offset, size, pose.left_shoulder.rotation);
  draw_bone_texture(p_canvas, p_textures.body_parts[RIGHT_SHOULDER],
                    top_left + pose.right_shoulder.offset, size, pose.right_shoulder.rotation);
  draw_bone_texture(p_canvas, p_textures.body_parts[TORSO], top_left + pose.torso.offset, size,
                    pose.torso.rotation);
  draw_bone_texture(p_canvas, p_textures.body_parts[HEAD], top_left + pose.head.offset, size,
                    pose.head.rotation);

  for (size_t i = 0; i < p_textures.attachment_count; ++i)
  {
    const HumanoidTextures::Attachment &attachment = p_textures.attachments[i];
    if (attachment.follow_bone == HumanoidBone::RIGHT_ARM)
    {
      continue;
    }
    const Vector2 item_size = attachment.texture->get_size();
    const Vector2 item_position = top_left + attachment_offset(pose, attachment.follow_bone);
    const float item_rotation = attachment_rotation(pose, attachment.follow_bone);
    draw_bone_texture(p_canvas, attachment.texture, item_position, item_size, item_rotation);
  }

  const float left_arm_rotation = pose.left_shoulder.rotation + pose.left_arm.rotation;
  draw_bone_texture(p_canvas, p_textures.body_parts[LEFT_ARM],
                    top_left + pose.left_shoulder.offset + pose.left_arm.offset, size,
                    left_arm_rotation);
  const float right_arm_rotation = pose.right_shoulder.rotation + pose.right_arm.rotation;
  draw_bone_texture(p_canvas, p_textures.body_parts[RIGHT_ARM],
                    top_left + pose.right_shoulder.offset + pose.right_arm.offset, size,
                    right_arm_rotation);

  for (size_t i = 0; i < p_textures.attachment_count; ++i)
  {
    const HumanoidTextures::Attachment &attachment = p_textures.attachments[i];
    if (attachment.follow_bone != HumanoidBone::RIGHT_ARM)
    {
      continue;
    }
    const Vector2 item_size = attachment.texture->get_size();
    const Vector2 item_position = top_left + attachment_offset(pose, attachment.follow_bone);
    const float item_rotation = attachment_rotation(pose, attachment.follow_bone);
    draw_bone_texture(p_canvas, attachment.texture, item_position, item_size, item_rotation);
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

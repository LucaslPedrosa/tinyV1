#pragma once

#include "rendering/humanoid_pose.hpp"

#include "game_types.hpp"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <array>
#include <cstddef>

namespace tinyv1 {

class GreeceRenderer {
  struct HumanoidTextures {
    std::array<godot::Ref<godot::Texture2D>, 8> body_parts;
    struct Attachment {
      godot::Ref<godot::Texture2D> texture;
      HumanoidBone follow_bone = HumanoidBone::ROOT;
    };
    std::array<Attachment, 4> attachments;
    size_t attachment_count = 0;
    bool body_ready = false;

    bool has_body() const;
  };

  HumanoidTextures worker_textures;
  HumanoidTextures hoplite_textures;
  godot::Ref<godot::Texture2D> town_center_texture;
  godot::Ref<godot::Texture2D> barracks_texture;

public:
  void load_textures();
  void draw_building(godot::Node2D &p_canvas, const BuildingSummary &p_building) const;
  void draw_unit(godot::Node2D &p_canvas, const UnitSummary &p_unit, float p_animation_time) const;
  float unit_radius(const UnitSummary &p_unit) const;
  godot::Vector2 unit_size(const UnitSummary &p_unit) const;
  godot::Vector2 unit_top_left(const UnitSummary &p_unit) const;

private:
  void load_humanoid_textures(const char *p_unit_folder, HumanoidTextures &r_textures) const;
  void add_humanoid_attachment(HumanoidTextures &r_textures, const godot::String &p_path, HumanoidBone p_follow_bone) const;
  HumanoidPose setup_pose_for(UnitType p_type) const;
  HumanoidPose humanoid_pose_for(const UnitSummary &p_unit, float p_animation_time) const;
  void draw_humanoid(godot::Node2D &p_canvas, const UnitSummary &p_unit, const HumanoidTextures &p_textures, float p_animation_time) const;
  void draw_worker_unit(godot::Node2D &p_canvas, const UnitSummary &p_unit, float p_animation_time) const;
  void draw_fighter_unit(godot::Node2D &p_canvas, const UnitSummary &p_unit, float p_animation_time) const;
};

} // namespace tinyv1

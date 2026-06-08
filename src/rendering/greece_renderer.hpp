#pragma once

#include "game_types.hpp"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <array>

namespace tinyv1 {

enum class HumanoidBone {
  ROOT,
  TORSO,
  HEAD,
  LEFT_SHOULDER,
  RIGHT_SHOULDER,
  LEFT_ARM,
  RIGHT_ARM,
  LEFT_LEG,
  RIGHT_LEG,
};

enum class HumanoidSocket {
  TORSO,
  HEAD,
  LEFT_HAND,
  RIGHT_HAND,
  LEFT_SHOULDER,
  RIGHT_SHOULDER,
};

struct BonePose {
  godot::Vector2 offset;
  float rotation = 0.0f;
};

struct HumanoidPose {
  BonePose root;
  BonePose torso;
  BonePose head;
  BonePose left_shoulder;
  BonePose right_shoulder;
  BonePose left_arm;
  BonePose right_arm;
  BonePose left_leg;
  BonePose right_leg;
};

class GreeceRenderer {
  struct HumanoidTextures {
    std::array<godot::Ref<godot::Texture2D>, 8> body_parts;
    godot::Ref<godot::Texture2D> right_hand_item;
    godot::Ref<godot::Texture2D> left_hand_item;

    bool has_body() const;
  };

  HumanoidTextures worker_textures;
  HumanoidTextures hoplite_textures;
  godot::Ref<godot::Texture2D> town_center_texture;
  godot::Ref<godot::Texture2D> barracks_texture;

public:
  void load_textures();
  void draw_base(godot::Node2D &p_canvas, const BaseSummary &p_base) const;
  void draw_barracks(godot::Node2D &p_canvas, const BuildingSummary &p_building) const;
  void draw_unit(godot::Node2D &p_canvas, const UnitSummary &p_unit, float p_animation_time) const;
  float unit_radius(const UnitSummary &p_unit) const;
  godot::Vector2 unit_size(const UnitSummary &p_unit) const;
  godot::Vector2 unit_top_left(const UnitSummary &p_unit) const;

private:
  void load_humanoid_textures(const char *p_unit_folder, HumanoidTextures &r_textures) const;
  HumanoidPose humanoid_pose_for(const UnitSummary &p_unit, float p_animation_time) const;
  godot::Vector2 bone_position(const HumanoidPose &p_pose, HumanoidBone p_bone, const godot::Vector2 &p_top_left) const;
  godot::Vector2 socket_position(const HumanoidPose &p_pose, HumanoidSocket p_socket, const godot::Vector2 &p_top_left) const;
  void draw_humanoid(godot::Node2D &p_canvas, const UnitSummary &p_unit, const HumanoidTextures &p_textures, float p_animation_time) const;
  void draw_worker_unit(godot::Node2D &p_canvas, const UnitSummary &p_unit, float p_animation_time) const;
  void draw_fighter_unit(godot::Node2D &p_canvas, const UnitSummary &p_unit, float p_animation_time) const;
};

} // namespace tinyv1

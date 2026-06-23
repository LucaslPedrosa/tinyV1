#pragma once

#include "rendering/humanoid_pose.hpp"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {

class Node2D;
class Texture2D;

} // namespace godot

namespace tinyv1 {

godot::Vector2 pixel_snap(const godot::Vector2 &p_position);
void draw_bone_texture(godot::Node2D &p_canvas, const godot::Ref<godot::Texture2D> &p_texture, const godot::Vector2 &p_position, const godot::Vector2 &p_size, float p_rotation_degrees);
godot::Vector2 attachment_offset(const HumanoidPose &p_pose, HumanoidBone p_follow_bone);
float attachment_rotation(const HumanoidPose &p_pose, HumanoidBone p_follow_bone);

} // namespace tinyv1
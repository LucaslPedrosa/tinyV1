#include "rendering/render_helpers.hpp"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/texture2d.hpp>

#include <cmath>

using namespace godot;

namespace tinyv1 {

Vector2 pixel_snap(const Vector2 &p_position) {
	return Vector2(std::round(p_position.x), std::round(p_position.y));
}

void draw_bone_texture(Node2D &p_canvas, const Ref<Texture2D> &p_texture,
                       const Vector2 &p_position, const Vector2 &p_size,
                       float p_rotation_degrees) {
	if (!p_texture.is_valid()) {
		return;
	}

	if (p_rotation_degrees == 0.0f) {
		p_canvas.draw_texture_rect(p_texture, Rect2(pixel_snap(p_position), p_size), false);
		return;
	}

	const Vector2 center = pixel_snap(p_position) + p_size * 0.5f;
	const float rotation_rad = p_rotation_degrees * (3.14159265f / 180.0f);
	p_canvas.draw_set_transform(center, rotation_rad, Vector2(1.0f, 1.0f));
	p_canvas.draw_texture_rect(p_texture, Rect2(-p_size * 0.5f, p_size), false);
	p_canvas.draw_set_transform(Vector2(), 0.0f, Vector2(1.0f, 1.0f));
}

Vector2 attachment_offset(const HumanoidPose &p_pose, HumanoidBone p_follow_bone) {
	switch (p_follow_bone) {
	case HumanoidBone::TORSO:
		return p_pose.torso.offset;
	case HumanoidBone::HEAD:
		return p_pose.head.offset;
	case HumanoidBone::LEFT_SHOULDER:
		return p_pose.left_shoulder.offset;
	case HumanoidBone::RIGHT_SHOULDER:
		return p_pose.right_shoulder.offset;
	case HumanoidBone::LEFT_ARM:
		return p_pose.left_shoulder.offset + p_pose.left_arm.offset;
	case HumanoidBone::RIGHT_ARM:
		return p_pose.right_shoulder.offset + p_pose.right_arm.offset;
	case HumanoidBone::LEFT_LEG:
		return p_pose.left_leg.offset;
	case HumanoidBone::RIGHT_LEG:
		return p_pose.right_leg.offset;
	case HumanoidBone::ROOT:
	default:
		return p_pose.root.offset;
	}
}

float attachment_rotation(const HumanoidPose &p_pose, HumanoidBone p_follow_bone) {
	switch (p_follow_bone) {
	case HumanoidBone::TORSO:
		return p_pose.torso.rotation;
	case HumanoidBone::HEAD:
		return p_pose.head.rotation;
	case HumanoidBone::LEFT_SHOULDER:
		return p_pose.left_shoulder.rotation;
	case HumanoidBone::RIGHT_SHOULDER:
		return p_pose.right_shoulder.rotation;
	case HumanoidBone::LEFT_ARM:
		return p_pose.left_shoulder.rotation + p_pose.left_arm.rotation;
	case HumanoidBone::RIGHT_ARM:
		return p_pose.right_shoulder.rotation + p_pose.right_arm.rotation;
	case HumanoidBone::LEFT_LEG:
		return p_pose.left_leg.rotation;
	case HumanoidBone::RIGHT_LEG:
		return p_pose.right_leg.rotation;
	case HumanoidBone::ROOT:
	default:
		return p_pose.root.rotation;
	}
}

} // namespace tinyv1
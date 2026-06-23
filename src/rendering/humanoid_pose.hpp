#pragma once

#include <godot_cpp/variant/vector2.hpp>

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

} // namespace tinyv1
#pragma once

#include <godot_cpp/variant/string.hpp>

#include <array>
#include <cstddef>

namespace tinyv1::asset_contracts {

inline constexpr const char *GREECE = "greece";

inline constexpr std::array<const char *, 8> HUMANOID_PART_FILES = {{
    "1head.png",
    "2torso.png",
    "3left_shoulder.png",
    "4left_arm.png",
    "5left_leg.png",
    "6right_shoulder.png",
    "7right_arm.png",
    "8right_leg.png",
}};

inline godot::String civilization_root(const char *p_civilization = GREECE)
{
  return godot::String("res://assets/civilizations/") + p_civilization + "/";
}

inline godot::String humanoid_root(const char *p_civilization = GREECE)
{
  return civilization_root(p_civilization) + "units/humanoid/";
}

inline godot::String humanoid_part(const char *p_unit_folder, size_t p_part_index,
                                  const char *p_civilization = GREECE)
{
  if (p_part_index >= HUMANOID_PART_FILES.size())
  {
    return "";
  }

  return humanoid_root(p_civilization) + p_unit_folder + "/" +
         HUMANOID_PART_FILES[p_part_index];
}

inline godot::String humanoid_face(const char *p_unit_folder,
                                  const char *p_civilization = GREECE)
{
  return humanoid_root(p_civilization) + p_unit_folder + "/_" + p_unit_folder + "_face.png";
}

inline godot::String humanoid_item(const char *p_category_folder, const char *p_item_name,
                                  const char *p_civilization = GREECE)
{
  return humanoid_root(p_civilization) + "_tools/" + p_category_folder + "/" + p_item_name +
         ".png";
}

inline godot::String building_sprite(const char *p_building_name,
                                    const char *p_civilization = GREECE)
{
  return civilization_root(p_civilization) + "b_" + p_building_name + "/" + p_building_name +
         ".png";
}

inline godot::String building_face(const char *p_building_name,
                                  const char *p_civilization = GREECE)
{
  return civilization_root(p_civilization) + "b_" + p_building_name + "/" + p_building_name +
         "_face.png";
}

inline godot::String world_resource_sprite(const char *p_resource_name)
{
  return godot::String("res://assets/world/") + p_resource_name + "/" + p_resource_name +
         ".png";
}

inline godot::String world_resource_face(const char *p_resource_name)
{
  return godot::String("res://assets/world/") + p_resource_name + "/" + p_resource_name +
         "_face.png";
}

} // namespace tinyv1::asset_contracts

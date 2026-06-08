#pragma once

#include "rendering/asset_contracts.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace tinyv1 {

inline godot::Ref<godot::Texture2D> load_texture(const godot::String &p_path)
{
  godot::ResourceLoader *loader = godot::ResourceLoader::get_singleton();
  if (loader->exists(p_path, "Texture2D"))
  {
    godot::Ref<godot::Resource> resource = loader->load(p_path, "Texture2D");
    godot::Ref<godot::Texture2D> texture = resource;
    if (texture.is_valid())
    {
      return texture;
    }
  }

  godot::Ref<godot::Image> image;
  image.instantiate();
  if (image->load(p_path) != godot::OK)
  {
    godot::UtilityFunctions::push_warning(godot::String("Could not load texture: ") + p_path);
    return godot::Ref<godot::Texture2D>();
  }

  return godot::ImageTexture::create_from_image(image);
}

} // namespace tinyv1

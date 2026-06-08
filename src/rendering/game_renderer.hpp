#pragma once

#include "game_types.hpp"
#include "rendering/greece_renderer.hpp"

#include <cstddef>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace tinyv1 {

class GameSimulation;
struct LocalPlayerState;

class GameRenderer {
  GreeceRenderer greece_renderer;
  godot::Ref<godot::Texture2D> goldmine_texture;
  mutable RenderSnapshot render_snapshot;

public:
  GameRenderer();
  void load_textures();
  void draw(godot::Node2D &p_canvas, const GameSimulation &p_simulation, const LocalPlayerState &p_local, float p_animation_time) const;
};

} // namespace tinyv1

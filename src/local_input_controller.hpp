#pragma once

#include "game_simulation/game_simulation.hpp"
#include "local_player_state.hpp"

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace tinyv1 {

class LocalInputController {
public:
  static bool handle_event(const godot::Ref<godot::InputEvent> &event,
                           const godot::Vector2 &mouse_position,
                           GameSimulation &sim,
                           LocalPlayerState &local);
  static bool perform_action_button(int32_t index,
                                    GameSimulation &sim,
                                    LocalPlayerState &local);
};

} // namespace tinyv1

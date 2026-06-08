#pragma once

#include "game_simulation/game_simulation.hpp"
#include "local_player_state.hpp"

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>

namespace tinyv1 {

class HudPresenter {
public:
  static godot::String get_status_text(const GameSimulation &sim,
                                       const LocalPlayerState &local);
  static godot::String get_resource_text(const GameSimulation &sim);
  static godot::String get_selected_name(const GameSimulation &sim,
                                         const LocalPlayerState &local);
  static godot::String get_selected_actions_text(const GameSimulation &sim,
                                                 const LocalPlayerState &local);
  static godot::String get_selected_details_text(const GameSimulation &sim,
                                                 const LocalPlayerState &local);
  static godot::Color get_selected_portrait_color(const GameSimulation &sim,
                                                  const LocalPlayerState &local);
  static godot::String get_selected_portrait_path(const GameSimulation &sim,
                                                  const LocalPlayerState &local);
  static godot::String get_action_button_text(const GameSimulation &sim,
                                              const LocalPlayerState &local,
                                              int32_t index);
  static godot::String get_action_button_icon_path(const GameSimulation &sim,
                                                   const LocalPlayerState &local,
                                                   int32_t index);
  static bool is_action_button_enabled(const GameSimulation &sim,
                                       const LocalPlayerState &local,
                                       int32_t index);
};

} // namespace tinyv1

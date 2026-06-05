#pragma once

#include "game_command.hpp"

#include <vector>

namespace tinyv1 {

class GameSimulation;

class BotController {
public:
	void reset();
	std::vector<GameCommand> update(const GameSimulation &p_simulation, double p_delta);

private:
	float decision_timer = 0.0f;
};

} // namespace tinyv1

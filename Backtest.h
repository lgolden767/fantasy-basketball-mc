#pragma once

#include "PlayerGameStats.h"
#include "FantasyScoring.h"

#include <string>
#include <vector>

void backtestPlayer(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    const ScoringRules& rules
);


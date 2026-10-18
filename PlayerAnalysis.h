#pragma once

#include "PlayerGameStats.h"
#include <string>
#include <vector>

struct PlayerSummary {
    std::string playerId;
    std::string playerName;

    int gamesPlayed = 0;

    double avgPoints = 0.0;
    double avgRebounds = 0.0;
    double avgAssists = 0.0;
};

PlayerSummary calculatePlayerAverages(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId
);

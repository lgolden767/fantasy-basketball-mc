#pragma once

#include "PlayerGameStats.h"
#include <cstddef>
#include <string>
#include <vector>
#include <optional>

struct PlayerSummary {
    std::string playerId;
    std::string playerName;

    int gamesPlayed = 0;

    double avgPoints = 0.0;
    double avgRebounds = 0.0;
    double avgAssists = 0.0;

    std::optional<double> pointsStdDev;
};

PlayerSummary calculatePlayerAverages(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId
);

void printPlayerHistory(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId
);

std::vector<PlayerGameStats> getRecentPlayerGames(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    const std::string& beforeDate,
    std::size_t limit = 60
);
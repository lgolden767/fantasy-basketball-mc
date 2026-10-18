#pragma once

#include <string>

struct PlayerGameStats {
    std::string playerId;
    std::string playerName;
    std::string gameId;

    bool didNotPlay = false;
    std::string minutes;

    int points = 0;
    int rebounds = 0;
    int assists = 0;
    int steals = 0;
    int blocks = 0;
    int turnovers = 0;

    int fieldGoalsMade = 0;
    int fieldGoalsAttempted = 0;

    int freeThrowsMade = 0;
    int freeThrowsAttempted = 0;

    int threePointersMade = 0;
    int threePointersAttempted = 0;
};
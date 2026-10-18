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
};
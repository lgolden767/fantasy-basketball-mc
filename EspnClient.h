#pragma once

#include "HttpClient.h"
#include "Game.h"
#include "PlayerGameStats.h"
#include "PlayerGameRef.h"

#include <string>
#include <vector>

class EspnClient{
    private:
        HttpClient http;

    public:
        std::vector<std::string> getGameNames();
        std::vector<Game> getGames(const std::string& date);
        std::vector<PlayerGameStats> getPlayerStats(const std::string& gameId);
        std::vector<PlayerGameRef> getPlayerGameLog(
            const std::string& playerId,
            int season
        );
};
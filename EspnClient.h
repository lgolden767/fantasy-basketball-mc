#pragma once

#include "HttpClient.h"
#include "Game.h"
#include "PlayerGameStats.h"

#include <string>
#include <vector>

class EspnClient{
    private:
        HttpClient http;

    public:
        std::vector<std::string> getGameNames();
        std::vector<Game> getGames();
        std::vector<PlayerGameStats> getPlayerStats(const std::string& gameId);
};
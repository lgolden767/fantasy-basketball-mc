#pragma once

#include "HttpClient.h"
#include "Game.h"
#include <string>
#include <vector>

class EspnClient{
    private:
        HttpClient http;

    public:
        std::vector<std::string> getGameNames();
        std::vector<Game> getGames();
};
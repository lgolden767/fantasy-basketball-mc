#pragma once
#include <string>

struct Game {
    std::string id;
    std::string homeTeam;
    std::string awayTeam;
    std::string status;
    bool completed = false;
};
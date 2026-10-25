#pragma once

#include <string> 
#include <vector>

struct RosterPlayer {
    std::string playerId;
    std::string nbaTeamId;
};

struct FantasyTeam {
    std::string name;
    std::vector<RosterPlayer> players;
};
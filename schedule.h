#pragma once

#include "EspnClient.h"
#include "Game.h"

#include <string>
#include <vector>

std::vector<Game> getTeamWeek(
    EspnClient& espn,
    const std::string& teamId,
    const std::string& startDate
);
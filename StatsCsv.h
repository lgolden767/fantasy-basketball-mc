#pragma once

#include "PlayerGameStats.h"
#include <string>
#include <vector>

void savePlayerStats(
    const std::vector<PlayerGameStats>& players,
    const std::string& filename
);
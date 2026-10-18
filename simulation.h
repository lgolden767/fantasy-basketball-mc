#include "PlayerGameStats.h"

#include <cstddef>
#include <string>
#include <vector>

struct SimulationResult {
    std::size_t historicalGames = 0;
    int simulations = 0;
    int successes = 0;
    double probability = 0.0;
};

SimulationResult simulatePoints(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    int threshold,
    int simulation,
    unsigned int seed = 42
);


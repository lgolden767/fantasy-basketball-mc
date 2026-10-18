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

struct WeeklySimulationResult {
    std::size_t historicalGames = 0;
    int simulations = 0;
    int successes = 0;
    double probability = 0.0;

    double meanPoints = 0.0;
    long long p10 = 0.0;
    long long median = 0.0;
    long long p90 = 0.0;
};

SimulationResult simulatePoints(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    int threshold,
    int simulation,
    unsigned int seed = 42
);

WeeklySimulationResult simulateWeeklyPoints(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    int gamesPerWeek,
    int threshold,
    int simluations,
    unsigned int seed = 42
);

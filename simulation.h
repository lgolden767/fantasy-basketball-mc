#pragma once

#include "PlayerGameStats.h"
#include "FantasyScoring.h"

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

struct WeeklyFantasyResult {
    std::size_t historicalGames = 0;
    int simulations = 0;
    int successess = 0;

    double probability = 0.0;
    double meanPoints = 0.0;
    double p10 = 0.0;
    double median = 0.0;
    double p90 = 0.0;

    std::vector<double> trialTotals;
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

WeeklyFantasyResult simulateWeeklyFantasyPoints(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    int gamesPerWeek,
    double threshold,
    int simulations,
    const ScoringRules& rules,
    unsigned int seed = 42
);
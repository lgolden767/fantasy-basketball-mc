#include "simulation.h"

#include <random>
#include <stdexcept>
#include <unordered_set>
#include <iostream>

SimulationResult simulatePoints(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    int threshold,
    int simulations,
    unsigned int seed
) {
    if (simulations <= 0) {
        throw std::runtime_error("Simulation count must be positive.");
    }

    if (threshold < 0) {
        throw std::runtime_error("Threshold must be greater than or equal to zero.");
    }

    std::vector<PlayerGameStats> history;
    std::unordered_set<std::string> countedGames;

    for (const auto& record : records) {
        if (record.playerId != playerId || record.didNotPlay) {
            continue;
        }

        if (!countedGames.insert(record.gameId).second) {
            continue;
        }

        history.push_back(record);
    }

    if (history.size() < 2) {
        throw std::runtime_error("Load at least two games for this player.");
    }

    std::mt19937 generator(seed);

    std::uniform_int_distribution<std::size_t> chooseGame(0, history.size()-1);

    SimulationResult result;
    result.historicalGames = history.size();
    result.simulations = simulations;

    std::cerr << "Historical games: " << history.size()
          << " | Simulations: " << simulations << '\n';

    for (int i = 0; i < simulations; ++i) {
        const auto& performance = history.at(
            chooseGame(generator)
        );

        if (performance.points > threshold) {
            ++result.successes;
        }
    }

    std::cerr << "Simulation loop finished.\n";

    result.probability = 
        static_cast<double>(result.successes) / simulations;

    return result;

}
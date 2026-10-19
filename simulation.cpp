#include "simulation.h"
#include "FantasyScoring.h"

#include <random>
#include <stdexcept>
#include <unordered_set>
#include <iostream>
#include <cmath>
#include <algorithm>

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

    int historicalSuccesses = 0;

    for (const auto& game : history) {
        if (game.points >= threshold) {
            ++historicalSuccesses;
        }
    }

    std::cerr << "Simulation loop finished.\n";

    double historicalProbability = 
        static_cast<double>(historicalSuccesses) / history.size();

    result.probability = 
        static_cast<double>(result.successes) / simulations;

    std::cout << "Historical percentage: "
          << historicalProbability * 100.0 << "%\n";

    return result;

}

WeeklySimulationResult simulateWeeklyPoints(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    int gamesPerWeek,
    int threshold,
    int simulations,
    unsigned int seed
) {
    if (gamesPerWeek <= 0 || simulations <= 0) {
        throw std::invalid_argument(
            "Games per week and simulation count must be positive."
        );
    }

    if (threshold < 0) {
        throw std::invalid_argument(
            "Threshold must be greater than zero."
        );
    }

    std::vector<PlayerGameStats> history;
    std::unordered_set<std::string> countedGames;

    for (const auto& record : records) {
        if (record.playerId != playerId) {
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

    std::uniform_int_distribution<std::size_t> chooseGame(
        0, history.size() - 1
    );

    WeeklySimulationResult result;
    result.historicalGames = history.size();
    result.simulations = simulations;

    std::vector<long long> weeklyTotals;
    weeklyTotals.reserve(simulations);

    double sumOfTotals = 0.0;

    for (int i = 0; i < simulations; ++i) {
        long long totalPoints = 0;

        for (int game = 0; game < gamesPerWeek; ++game) {
            const auto& performance =
                history.at(chooseGame(generator));

            totalPoints += performance.points;
        }

        weeklyTotals.push_back(totalPoints);
        sumOfTotals += static_cast<double>(totalPoints);

        if (totalPoints >= threshold) {
            ++result.successes;
        }
    }

    result.probability =
        static_cast<double>(result.successes) / simulations;

    result.meanPoints = sumOfTotals / simulations;

    std::sort(weeklyTotals.begin(), weeklyTotals.end());

    auto percentile = [&](double fraction) {
        std::size_t rank = static_cast<std::size_t>(
            std::ceil(fraction * weeklyTotals.size())
        );

        return weeklyTotals.at(rank - 1);
    };

    result.p10      = percentile(0.10);
    result.median   = percentile(0.50);
    result.p90      = percentile(0.90);

    return result;
}

WeeklyFantasyResult simulateWeeklyFantasyPoints(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    int gamesPerWeek,
    double threshold,
    int simulations,
    const ScoringRules& rules,
    unsigned int seed
) {
    if (gamesPerWeek <= 0 || simulations <= 0) {
        throw std::invalid_argument("Games per week and simulations must not be negative.");
    }

    if (threshold < 0) {
        throw std::invalid_argument("Threshold must be nonnegative.");
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
        throw std::runtime_error("Load at least two player games for this player");
    }

    std::vector<double> historicalScores;

    for (const auto& performance : history) {
        double score = calculateFantasyPoints(performance, rules);

        if (!std::isfinite(score)) {
            throw std::runtime_error("Invalid fantasy score.");
        }

        historicalScores.push_back(score);
    }

    std::mt19937 generator(seed);

    std::uniform_int_distribution<std::size_t> chooseGame(
        0, historicalScores.size() - 1
    );

    WeeklyFantasyResult result;
    result.historicalGames = history.size();
    result.simulations = simulations;

    std::vector<double> weeklyTotals;
    weeklyTotals.reserve(simulations);

    double sumOfTotals = 0.0;

    for (int trial = 0; trial < simulations; ++trial) {
        double totalFantasyPoints = 0.0;

        for (int game = 0; game < gamesPerWeek; ++game) {
            totalFantasyPoints +=
                historicalScores.at(chooseGame(generator));
        }

        weeklyTotals.push_back(totalFantasyPoints);
        sumOfTotals += totalFantasyPoints;

        if (totalFantasyPoints >= threshold) {
            ++result.successess;
        }
    }

    result.probability =
        static_cast<double>(result.successess) / simulations;

    result.meanPoints = sumOfTotals / simulations;

    std::sort(weeklyTotals.begin(), weeklyTotals.end());

    auto percentile = [&](double fraction) {
        std::size_t rank = static_cast<std::size_t>(
            std::ceil(fraction * weeklyTotals.size())
        );

        return weeklyTotals.at(rank - 1);
    };

    result.p10      = percentile(0.10);
    result.median   = percentile(0.50);
    result.p90      = percentile(0.90);

    return result;
}
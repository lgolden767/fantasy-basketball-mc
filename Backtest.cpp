#include "Backtest.h"
#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <unordered_set>

void backtestPlayer(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    const ScoringRules& rules
) {
    std::vector<PlayerGameStats> training;
    std::vector<PlayerGameStats> testing;
    std::unordered_set<std::string> countedGames;

    for (const auto& record : records) {
        if (record.playerId != playerId 
            || record.didNotPlay 
            || record.seasonType != 2) {
            continue;
        }

        if (!countedGames.insert(record.gameId).second) {
            continue;
        }

        if (record.gameDate.empty()) {
            throw std::runtime_error(
                "Missing date for game " + record.gameId
            );
        }

        if (record.seasonYear >= 2022
            && record.seasonYear <= 2025) {
                training.push_back(record);
        } else if (record.seasonYear == 2026) {
                testing.push_back(record);
        }
    }
    if (training.size() < 2) {
        throw std::runtime_error(
            "Need at least two training games."
        );
    }

    if (testing.size() < 3) {
        throw std::runtime_error(
            "Need at least three test games."
        );
    }

    auto byDate = [](const PlayerGameStats& a, const PlayerGameStats& b) {
        if (a.gameDate == b.gameDate) {
            return a.gameId < b.gameId;
        }

        return a.gameDate < b.gameDate;
    };

    std::sort(training.begin(), training.end(), byDate);
    std::sort(testing.begin(), testing.end(), byDate);

    if (training.back().gameDate >= testing.front().gameDate) {
        throw std::runtime_error(
            "Training and test dates overlap."
        );
    }

    constexpr int gamesPerGroup = 3;

    auto prediction = simulateWeeklyFantasyPoints(
        training,
        playerId,
        gamesPerGroup,
        0.0,
        10000,
        rules,
        42
    );

    std::cout << std::fixed << std::setprecision(2);

    std::cout << "\nTraining games: " << training.size()
              << "\nTest games : " << testing.size()
              << "\nPredicted 10th-90th percentile range: "
              << prediction.p10 << " to " << prediction.p90
              << "\n\n";
    
    std::cout << std::left
              << std::setw(8) << "Group"
              << std::setw(15) << "Predicted"
              << std::setw(15) << "Actual"
              << "Inside range\n";
    
    double absoluteErrorTotal = 0.0;
    double signedErrorTotal = 0.0;
    int groups = 0;
    int insideCount = 0;

    for (std::size_t start = 0;
         start + gamesPerGroup <= testing.size();
         start += gamesPerGroup) {
        double actualTotal = 0.0;

        for (int offset = 0; offset < gamesPerGroup; ++offset) {
            actualTotal += calculateFantasyPoints(
                testing.at(start + offset),
                rules
            );
        }

        bool inside = 
            actualTotal >= prediction.p10 &&
            actualTotal <= prediction.p90;
        
        double error = prediction.meanPoints - actualTotal;

        absoluteErrorTotal += std::abs(error);
        signedErrorTotal += error;

        if (inside) {
            ++insideCount;
        }

        ++groups;

        if (groups == 2  || groups == 13) {
            std::cout << "\nDetails for group " << groups << ":\n";

            for (int offset = 0; offset < gamesPerGroup; ++offset) {
                const auto& game = testing.at(start + offset);

                std::cout
                    << "Game: " << game.gameId
                    << " | Date: " << game.gameDate
                    << " | MIN: " << game.minutes << '\n'

                    << "PTS: " << game.points
                    << " | REB: " << game.rebounds
                    << " | AST: " << game.assists
                    << " | STL: " << game.steals
                    << " | BLK: " << game.blocks
                    << " | TO: " << game.turnovers << '\n'

                    << "FG: " << game.fieldGoalsMade
                    << '-' << game.fieldGoalsAttempted
                    << " | FT: " << game.freeThrowsMade
                    << '-' << game.freeThrowsAttempted
                    << " | 3PT: " << game.threePointersMade
                    << '-' << game.threePointersAttempted << '\n'

                    << "Fantasy Points: "
                    << calculateFantasyPoints(game, rules)
                    << "\n\n";
            }
        }

        std::cout << std::left
                  << std::setw(8) << groups
                  << std::setw(15) << prediction.meanPoints
                  << std::setw(15) << actualTotal
                  << (inside ? "Yes" : "No") << '\n';
    }

    std::cout << "\nGroups tested: " << groups
              << "\nUnused leftover games: "
              << testing.size() % gamesPerGroup
              << "\nMean absolute error: "
              << absoluteErrorTotal / groups
              << "\nMean signed error (prediction - actual): "
              << signedErrorTotal / groups
              << "\n80% interval coverage: "
              << 100.0 * insideCount / groups << "%\n";
}
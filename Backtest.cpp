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
        if (record.playerId != playerId ||
            record.didNotPlay ||
            record.seasonType != 2) {
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

        if (record.seasonYear >= 2022 &&
            record.seasonYear <= 2025) {
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

    auto byDate = [](
        const PlayerGameStats& a,
        const PlayerGameStats& b
    ) {
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
              << "\nTest games: " << testing.size()
              << "\nBaseline 10th–90th percentile range: "
              << prediction.p10 << " to "
              << prediction.p90 << "\n\n";

    double absoluteErrorTotal = 0.0;
    double signedErrorTotal = 0.0;

    int groups = 0;
    int insideCount = 0;

    std::vector<PlayerGameStats> availableHistory = training;

    constexpr std::size_t historyWindow = 30;

    double rollingAbsoluteError = 0.0;
    double rollingSignedError = 0.0;

    int rollingInsideCount = 0;

    std::cout << std::left
              << std::setw(8) << "Group"
              << std::setw(15) << "Baseline"
              << std::setw(15) << "Rolling"
              << std::setw(15) << "Actual"
              << "Rolling range\n";

    for (
        std::size_t start = 0;
        start + gamesPerGroup <= testing.size();
        start += gamesPerGroup
    ) {
        std::size_t windowSize = std::min(
            historyWindow,
            availableHistory.size()
        );

        auto windowStart =
            availableHistory.end() -
            static_cast<
                std::vector<PlayerGameStats>::difference_type
            >(windowSize);

        std::vector<PlayerGameStats> recentHistory(
            windowStart,
            availableHistory.end()
        );

        if (recentHistory.back().gameDate >=
            testing.at(start).gameDate) {
            throw std::runtime_error(
                "Rolling history overlaps the test group."
            );
        }

        auto rollingPrediction = simulateWeeklyFantasyPoints(
            recentHistory,
            playerId,
            gamesPerGroup,
            0.0,
            10000,
            rules,
            42
        );

        double actualTotal = 0.0;

        for (int offset = 0; offset < gamesPerGroup; ++offset) {
            actualTotal += calculateFantasyPoints(
                testing.at(start + offset),
                rules
            );
        }

        double rollingError =
            rollingPrediction.meanPoints - actualTotal;

        rollingAbsoluteError += std::abs(rollingError);
        rollingSignedError += rollingError;

        bool rollingInside =
            actualTotal >= rollingPrediction.p10 &&
            actualTotal <= rollingPrediction.p90;

        if (rollingInside) {
            ++rollingInsideCount;
        }

        bool inside =
            actualTotal >= prediction.p10 &&
            actualTotal <= prediction.p90;

        if (inside) {
            ++insideCount;
        }

        double error = prediction.meanPoints - actualTotal;

        absoluteErrorTotal += std::abs(error);
        signedErrorTotal += error;

        ++groups;

        std::cout << std::left
                  << std::setw(8) << groups
                  << std::setw(15) << prediction.meanPoints
                  << std::setw(15) << rollingPrediction.meanPoints
                  << std::setw(15) << actualTotal
                  << rollingPrediction.p10 << " to "
                  << rollingPrediction.p90 << '\n';

        for (int offset = 0; offset < gamesPerGroup; ++offset) {
            availableHistory.push_back(
                testing.at(start + offset)
            );
        }

    }

    std::cout << "\nGroups tested: " << groups
              << "\nUnused leftover games: "
              << testing.size() % gamesPerGroup
              << '\n';

    std::cout << "\nBaseline model:\n"
              << "Mean absolute error: "
              << absoluteErrorTotal / groups << '\n'
              << "Mean signed error (prediction - actual): "
              << signedErrorTotal / groups << '\n'
              << "80% interval coverage: "
              << 100.0 * insideCount / groups << "%\n";

    std::cout << "\nRolling model (last "
              << historyWindow << " played games):\n"
              << "Mean absolute error: "
              << rollingAbsoluteError / groups << '\n'
              << "Mean signed error (prediction - actual): "
              << rollingSignedError / groups << '\n'
              << "80% interval coverage: "
              << 100.0 * rollingInsideCount / groups << "%\n";
}
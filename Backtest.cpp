#include "Backtest.h"
#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <filesystem>
#include <fstream>

BacktestResult backtestPlayer(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    const ScoringRules& rules,
    std::size_t historyWindow
) {
    if (historyWindow < 2) {
        throw std::invalid_argument(
            "History window must be at least two games"
        );
    }

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

    const int testSeason = testing.front().seasonYear;

    std::filesystem::create_directories("Data/processed");

    const std::string detailFilename =
        "Data/processed/backtest_details_" +
        std::to_string(testSeason) + "_" +
        playerId + "_" +
        std::to_string(historyWindow) + ".csv";

    std::ofstream details(detailFilename);

    if (!details) {
        throw std::runtime_error(
            "Could not open " + detailFilename
        );
    }

    details << "player_id,test_season,history_window,group,"
            << "first_game_date,last_game_date,"
            << "baseline_mean,baseline_p10,baseline_p90,"
            << "rolling_mean,rolling_p10,rolling_p90,"
            << "actual_total\n";

    details << std::fixed << std::setprecision(4);

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

    int baselineBelowCount = 0;
    int baselineAboveCount = 0;

    int rollingBelowCount = 0;
    int rollingAboveCount = 0;

    std::vector<PlayerGameStats> availableHistory = training;

    double rollingAbsoluteError = 0.0;
    double rollingSignedError = 0.0;

    double rollingIntervalWidthTotal = 0.0;

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

        rollingIntervalWidthTotal +=
            rollingPrediction.p90 - rollingPrediction.p10;

        double actualTotal = 0.0;

        for (int offset = 0; offset < gamesPerGroup; ++offset) {
            actualTotal += calculateFantasyPoints(
                testing.at(start + offset),
                rules
            );
        }

        if (actualTotal < prediction.p10) {
            ++baselineBelowCount;
        } else if (actualTotal > prediction.p90) {
            ++baselineAboveCount;
        }

        if (actualTotal < rollingPrediction.p10) {
            ++rollingBelowCount;
        } else if (actualTotal > rollingPrediction.p90) {
            ++rollingAboveCount;
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

        details << playerId << ','
                << testSeason << ','
                << historyWindow << ','
                << groups << ','
                << testing.at(start).gameDate << ','
                << testing.at(start + gamesPerGroup - 1).gameDate << ','
                << prediction.meanPoints << ','
                << prediction.p10 << ','
                << prediction.p90 << ','
                << rollingPrediction.meanPoints << ','
                << rollingPrediction.p10 << ','
                << rollingPrediction.p90 << ','
                << actualTotal << '\n';

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

    std::cout << "\nAverage 80% interval width:\n"
              << "Baseline: "
              << prediction.p90 - prediction.p10 << '\n'
              << "Rolling: "
              << rollingIntervalWidthTotal / groups << '\n';

    BacktestResult result;

    result.playerId = playerId;
    result.groupsTested = groups;
    result.historyWindow = historyWindow;

    result.baseline.meanAbsoluteError =
        absoluteErrorTotal / groups;
    
    result.baseline.meanSignedError =
        signedErrorTotal / groups;
    
    result.baseline.coveragePercent =
        100.0 * insideCount / groups;
    
    result.baseline.averageIntervalWidth =
        prediction.p90 - prediction.p10;
    
    result.rolling.meanAbsoluteError =
        rollingAbsoluteError / groups;
    
    result.rolling.meanSignedError =
        rollingSignedError / groups;
    
    result.rolling.coveragePercent =
        100.0 * rollingInsideCount / groups;
    
    result.rolling.averageIntervalWidth =
        rollingIntervalWidthTotal / groups;

    result.baseline.belowRange = baselineBelowCount;
    result.baseline.insideRange = insideCount;
    result.baseline.aboveRange = baselineAboveCount;

    result.rolling.belowRange = rollingBelowCount;
    result.rolling.insideRange = rollingInsideCount;
    result.rolling.aboveRange = rollingAboveCount;

    return result;
}
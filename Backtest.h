#pragma once

#include "PlayerGameStats.h"
#include "FantasyScoring.h"

#include <string>
#include <vector>
#include <cstddef>

struct ModelMetrics {
    double meanAbsoluteError = 0.0;
    double meanSignedError = 0.0;
    double coveragePercent = 0.0;
    double averageIntervalWidth = 0.0;
    int belowRange = 0;
    int insideRange = 0;
    int aboveRange = 0;
};

struct BacktestResult {
    std::string playerId;
    int groupsTested = 0;

    std::size_t historyWindow = 0;

    ModelMetrics baseline;
    ModelMetrics rolling;
};

BacktestResult backtestPlayer(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    const ScoringRules& rules,
    std::size_t historyWindow = 30
);


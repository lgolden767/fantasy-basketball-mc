#pragma once

#include "PlayerGameStats.h"

struct ScoringRules {
    double points = 1.0;
    double rebounds = 1.0;
    double assists = 2.0;
    double steals = 4.0;
    double blocks = 4.0;
    double turnovers = -2.0;

    double fieldGoalsMade = 2.0;
    double fieldGoalsAttempted = -1.0;

    double freeThrowsMade = 1.0;
    double freeThrowsAttempted = -1.0;

    double threePointersMade = 1.0;
};

double calculateFantasyPoints(
    const PlayerGameStats& stats,
    const ScoringRules& rules = ScoringRules{}
);
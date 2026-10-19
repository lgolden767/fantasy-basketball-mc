#include "FantasyScoring.h"

double calculateFantasyPoints(
    const PlayerGameStats& stats,
    const ScoringRules& rules
) {
    if (stats.didNotPlay) 
    return 0.0;

    return
        stats.points * rules.points
        + stats.rebounds * rules.rebounds
        + stats.assists * rules.assists
        + stats.steals * rules.steals
        + stats.blocks * rules.blocks
        + stats.turnovers * rules.turnovers
        + stats.fieldGoalsMade * rules.fieldGoalsMade
        + stats.fieldGoalsAttempted * rules.fieldGoalsAttempted
        + stats.freeThrowsMade * rules.freeThrowsMade
        + stats.freeThrowsAttempted * rules.freeThrowsAttempted
        + stats.threePointersMade * rules.threePointersMade;
}
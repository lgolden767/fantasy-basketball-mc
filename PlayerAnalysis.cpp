#include "PlayerAnalysis.h"

#include <unordered_set>

PlayerSummary calculatePlayerAverages(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId
) {
    PlayerSummary summary;
    summary.playerId = playerId;

    int totalPoints    = 0;
    int totalRebounds  = 0;
    int totalAssists   = 0;

    std::unordered_set<std::string> countedGames;

    for (const auto& record : records) {
        if (record.playerId != playerId) {
            continue;
        }

        summary.playerName = record.playerName;

        if (record.didNotPlay) {
            continue;
        }

        if (!countedGames.insert(record.gameId).second) {
            continue;
        }

        totalPoints     += record.points;
        totalRebounds   += record.rebounds;
        totalAssists    += record.assists;

        ++summary.gamesPlayed;
    }

    if (summary.gamesPlayed > 0) {
        summary.avgPoints   = static_cast<double>(totalPoints) / summary.gamesPlayed;
        summary.avgRebounds = static_cast<double>(totalRebounds) / summary.gamesPlayed;
        summary.avgAssists  = static_cast<double>(totalAssists) / summary.gamesPlayed;
    }

    return summary;
}
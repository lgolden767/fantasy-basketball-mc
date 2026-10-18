#include "PlayerAnalysis.h"

#include <unordered_set>
#include <cmath>

PlayerSummary calculatePlayerAverages(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId
) {
    PlayerSummary summary;

    std::vector<double> pointsHistory;
    
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

        pointsHistory.push_back(record.points);

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

    if (pointsHistory.size() >= 2) {
        double squaredDifferences = 0.0;

        for (double points : pointsHistory) {
            double difference = points - summary.avgPoints;
            squaredDifferences = difference * difference;
        }

        double variance = 
            squaredDifferences / 
            static_cast<double>(pointsHistory.size() - 1);
        
        summary.pointsStdDev = std::sqrt(variance);
    }

    return summary;
}
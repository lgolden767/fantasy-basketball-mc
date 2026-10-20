#include "PlayerAnalysis.h"

#include <unordered_set>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <map>
#include <utility>

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

void printPlayerHistory(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId
) {
    std::unordered_set<std::string> countedGames;

    std::map<std::pair<int, int>, int> counts;

    std::string earliest;
    std::string latest;
    std::string playerName;

    for (const auto& record : records) {
        if (record.playerId != playerId || record.didNotPlay) {
            continue;
        }

        if (!countedGames.insert(record.gameId).second) {
            continue;
        }

        if (record.gameDate.empty() || record.seasonYear <= 0 || record.seasonType <= 0) {
            throw std::runtime_error("Missing metadata for the game.");
        }

        playerName = record.playerName;

        std::string date = record.gameDate.substr(0, 10);

        if (earliest.empty() || date < earliest) {
            earliest = date;
        }

        if (latest.empty() || date > latest) {
            latest = date;
        }

        ++counts[{record.seasonYear, record.seasonType}];
    }

    if (countedGames.empty()) {
        std::cout << "No played games found for " << playerId << '\n';

        return;
    }

    std::cout << "\nPlayer: " << playerName << '\n'
              << "Unique played games: " << countedGames.size() << '\n'
              << "Earliest UTC date: " << earliest << '\n'
              << "Latest UTC date: " << latest << '\n';
    
    for (const auto& [season, count] : counts) {
        std::cout << "Season year: " << season.first
                  << " | Games played: " << count << '\n';
    }
}
#include "PlayerAnalysis.h"
#include "EspnClient.h"
#include "StatsCsv.h"

#include <unordered_set>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <map>
#include <utility>
#include <algorithm>
#include <iostream>
#include <stdexcept>

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
            squaredDifferences += difference * difference;
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

std::vector<PlayerGameStats> getRecentPlayerGames(
    const std::vector<PlayerGameStats>& records,
    const std::string& playerId,
    const std::string& beforeDate,
    std::size_t limit
) {
    if (limit < 2) {
        throw std::invalid_argument(
            "History limit must be at least two."
        );
    }

    std::vector<PlayerGameStats> history;

    if (beforeDate.size() != 8 ||
        beforeDate.find_first_not_of("0123456789") !=
            std::string::npos) {
        throw std::invalid_argument(
            "Use YYYYMMDD for the cutoff date."
        );
    }

    const std::string cutoff =
        beforeDate.substr(0, 4) + "-" +
        beforeDate.substr(4, 2) + "-" +
        beforeDate.substr(6, 2);

    for (const auto& record : records) {
        if (record.playerId != playerId ||
            record.didNotPlay           ||
            record.seasonType != 2) {
            continue;
        }

        if (record.gameDate.size() < 10) {
            throw std::runtime_error(
                "Missing or invalid date for game " + record.gameId
            );
        }

        const std::string gameDay = record.gameDate.substr(0, 10);

        if (gameDay >= cutoff) {
            continue;
        }

        if (record.gameId == "401838140" ||
            record.gameId == "401838141" ||
            record.gameId == "401838142" ||
            record.gameId == "401838143") {
            continue;
        }

        if (record.gameDate.empty()) {
            throw std::runtime_error(
                "Missing date for game " + record.gameId
            );
        }

        history.push_back(record);
    }

    std::sort(
        history.begin(),
        history.end(),
        [](const PlayerGameStats& a, const PlayerGameStats& b) {
            if (a.gameDate == b.gameDate) {
                return a.gameId < b.gameId;
            }

            return a.gameDate < b.gameDate;
        }
    );

    std::unordered_set<std::string> countedGames;
    std::vector<PlayerGameStats> uniqueHistory;

    for (const auto& record : history) {
        if (countedGames.insert(record.gameId).second) {
            uniqueHistory.push_back(record);
        }
    }

    if (uniqueHistory.size() < 2) {
        throw std::invalid_argument(
            "Need at least two played games for this player."
        );
    }

    if (uniqueHistory.size() > limit) {
        const auto removeCount =
            static_cast<std::vector<PlayerGameStats>::difference_type>(
                uniqueHistory.size() - limit
            );
        
        uniqueHistory.erase(
            uniqueHistory.begin(),
            uniqueHistory.begin() + removeCount
        );
    }

    return uniqueHistory;
}

std::vector<PlayerGameStats> ensurePlayerHistory(
    EspnClient& espn,
    std::vector<PlayerGameStats>& allPlayers,
    const std::string& playerId,
    const std::string& startDate,
    std::size_t limit
) {
    if (limit < 2) {
        throw std::invalid_argument(
            "History limit must be at least two."
        );
    }

    if (startDate.size() != 8 ||
        startDate.find_first_not_of("0123456789") != std::string::npos) {
            throw std::invalid_argument(
                "Use YYYYMMDD for the start date."
            );
    }

    const std::string cutoff =
        startDate.substr(0, 4) + "-" +
        startDate.substr(4, 2) + "-" +
        startDate.substr(6, 2);

    std::size_t gamesChecked = 0;
    std::size_t gamesDownloaded = 0;

    const auto seasons = espn.getPlayerSeasons(playerId);
    const int cutoffYear = std::stoi(startDate.substr(0, 4));

    for (int season : seasons) {
        if (season > cutoffYear + 1) { continue; }

        std::cout << "\nChecking season " << season << "...\n";

        auto gameLog = espn.getPlayerGameLog(playerId, season);

        for (const auto& game : gameLog) {
            if (game.gameDate.substr(0, 10) >= cutoff) {
                continue;
            }

            auto existing = std::find_if(
                allPlayers.begin(),
                allPlayers.end(),
                [&](const PlayerGameStats& record) {
                    return record.playerId == playerId &&
                        record.gameId == game.gameId &&
                        !record.didNotPlay;
                }
            );

            if (existing == allPlayers.end()) {
                std::cout << "Downloading game "
                        << game.gameId << "...\n";

                auto players = espn.getPlayerStats(game.gameId);

                auto player = std::find_if(
                    players.begin(),
                    players.end(),
                    [&](const PlayerGameStats& record) {
                        return record.playerId == playerId;
                    }
                );

                if (player == players.end()) {
                    throw std::runtime_error(
                        "Player missing from box score " + game.gameId
                    );
                }

                if (player->didNotPlay) { continue; }
                PlayerGameStats record = *player;

                record.gameId = game.gameId;
                record.gameDate = game.gameDate;
                record.seasonYear = game.seasonYear;
                record.seasonType = 2;

                const std::string filename =
                    "metadata/autopull_" +
                    record.playerId + "_" +
                    record.gameId + ".csv";

                savePlayerStats({record}, filename);

                allPlayers.push_back(record);
                ++gamesDownloaded;
            }

            ++gamesChecked;

            if (gamesChecked == limit) { break; };
        }

        if (gamesChecked == limit) { break; }
    }

    std::cout << "\nEligible played games checked: "
                << gamesChecked
                << "\nMissing games downloaded: "
                << gamesDownloaded << '\n';

    return getRecentPlayerGames(
        allPlayers,
        playerId,
        startDate,
        limit
    );
}
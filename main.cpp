#include "HttpClient.h"
#include "EspnClient.h"
#include "Game.h"
#include "StatsCsv.h"
#include "PlayerAnalysis.h"

#include <curl/curl.h>
#include <iostream>
#include <exception>
#include <fstream>
#include <iomanip>

std::string csvQuote(const std::string& text) {
    std::string result = "\"";

    for (char character : text) {
        if (character == '"') {
            result += '"';
        }

        result += character;
    }

    return result + '"';
}

int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        std::cerr << "Could not initialize curl.\n";
        return 1;
    }

    int exitCode = 0;

    try {
        EspnClient espn;

        std::string date;

        std::cout << "Enter date (YYYYMMDD): ";
        std::cin >> date;

        auto games = espn.getGames(date);

        if (games.empty()) {
            std::cout << "No games returned.\n";
        }

        for (const auto& game : games) {
            if (!game.completed) {
                continue;
            }

            std::cout << game.id << " | "
                                 << game.awayTeam << " at "
                                 << game.homeTeam << " | "
                                 << game.status << '\n';
        }

        int savedCount = 0;

        std::vector<PlayerGameStats> allPlayers;

        for (const auto& game : games) {
            if (!game.completed) {
                continue;
            }

            try {
                auto players = espn.getPlayerStats(game.id);

                allPlayers.insert(allPlayers.begin(), players.begin(), players.end());

                if (players.empty()) {
                    std::cout << "No player stats for " << game.id << '\n';
                    continue;
                }

                std::string filename = game.id + "_stats.csv";
                savePlayerStats(players, filename);

                std::cout << "Saved " << filename << '\n';
                ++savedCount;
            } catch (const std::exception& error) {
                std::cerr << "Failed game " << game.id
                        << ": " << error.what() << '\n';
            }
        }

        std::cout << "Exported " << savedCount << " games.\n";

        std::string playerId;

        std::cout << "Enter player ID: ";
        std::cin >> playerId;

        PlayerSummary summary = 
            calculatePlayerAverages(allPlayers, playerId);

        if (summary.gamesPlayed == 0) {
            std::cout << "No games played for player "
                      << playerId << ".\n";
        } else {
            std::cout << std::fixed << std::setprecision(2);

            std::cout << "\nPlayer: " << summary.playerName << '\n'
                      << "Games played: " << summary.gamesPlayed << '\n'
                      << "Average points: " << summary.avgPoints << '\n'
                      << "Average Rebounds: " << summary.avgRebounds << '\n'
                      << "Average Assists: " << summary.avgAssists << '\n';
        }

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}
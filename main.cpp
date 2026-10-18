#include "HttpClient.h"
#include "EspnClient.h"
#include "Game.h"
#include "StatsCsv.h"

#include <curl/curl.h>
#include <iostream>
#include <exception>
#include <fstream>

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

        std::vector<Game> games = espn.getGames("20260314");

        if (games.empty()) {
            std::cout << "No games returned.\n";
        }

        for (const auto& game : games) {
            std::cout << game.id << " | "
                      << game.awayTeam << " at "
                      << game.homeTeam << " | "
                      << game.status << '\n';
        }

        std::string gameId;
        std::cout << "\n Enter a completed game's ID: ";
        std::cin >> gameId;

        auto players = espn.getPlayerStats(gameId);

        for (const auto& player : players) {
            std::cout << player.playerName;

            if (player.didNotPlay) {
                std::cout << "| DNP\n";
            }
            else {
                std::cout << " | MIN: " << player.minutes
                          << " | PTS: " << player.points
                          << " | REB: " << player.rebounds
                          << " | AST: " << player.assists
                          << '\n';
            }
        }

        if (players.empty()) {
            std::cout << "No player statistics returned.\n";
        }
        
        std::string filename = gameId + "_stats.csv";

        savePlayerStats(players, filename);

        std::cout << "Saved " + filename << "\n";

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}
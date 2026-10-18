#include "HttpClient.h"
#include "EspnClient.h"
#include "Game.h"

#include <curl/curl.h>
#include <iostream>
#include <exception>

int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        std::cerr << "Could not initialize curl.\n";
        return 1;
    }

    int exitCode = 0;

    try {
        EspnClient espn;

        std::vector<Game> games = espn.getGames();

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
                std::cout << "DNP";
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

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}
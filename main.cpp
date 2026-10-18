#include "HttpClient.h"
#include "EspnClient.h"
#include "Game.h"

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
                std::cout << "DNP\n";
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

        std::ofstream file(filename);

        if (!file) {
            throw std::runtime_error("Could not open CSV file.");
        }

        file << "game_id,player_id,player_name,did_not_play,minutes,points,rebounds,assists\n";

        for (const auto& player : players) {
            file << csvQuote(player.gameId) << ","
                 << csvQuote(player.playerId) << ","
                 << csvQuote(player.playerName) << ","
                 << csvQuote(player.didNotPlay ? "true" : "false") << ",";

            if (player.didNotPlay) {
                file << ",,,\n";
            }
            else {
                file << csvQuote(player.minutes) << ","
                     << player.points << ","
                     << player.rebounds << ","
                     << player.assists << "\n";
            }
            
        }

        file.close();

        if (!file) {
            throw std::runtime_error("Could not finish writing CSV.");
        }

        std::cout << "Saved " << filename << "\n";

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}
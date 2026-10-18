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

        for (const auto& game : games) {
            if (!game.completed) {
                continue;
            }

            try {
                auto players = espn.getPlayerStats(game.id);

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
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}
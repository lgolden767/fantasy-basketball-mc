#include "HttpClient.h"
#include "EspnClient.h"
#include "Game.h"
#include "StatsCsv.h"
#include "PlayerAnalysis.h"
#include "simulation.h"

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
        std::vector<PlayerGameStats> allPlayers;

        std::vector<std::string> filenames = {
            "401809238_stats.csv",
            "401809239_stats.csv",
            "401809240_stats.csv",
            "401809241_stats.csv",
            "401809242_stats.csv",
            "401810824_stats.csv",
            "401810825_stats.csv",
            "401810826_stats.csv",
            "401810827_stats.csv",
            "401810828_stats.csv",
            "401810829_stats.csv",
            "401810830_stats.csv"
        };

        for (const auto& filename : filenames) {
            auto records = loadPlayerStats(filename);

            allPlayers.insert(allPlayers.end(), records.begin(), records.end());
        }

        std::string playerId;
        int threshold = 0;

        std::cout << "Enter player ID: ";
        std::cin >> playerId;

        std::cout << "Enter points threshold: ";

        if (!(std::cin >> threshold)) {
            throw std::runtime_error("Please enter whole number threshold.");
        }

        auto result = simulatePoints(
            allPlayers,
            playerId,
            threshold,
            1000
        );

        std::cout << std::fixed << std::setprecision(2);

        std::cout << "\nHistorical games: " 
                  << result.historicalGames << '\n'
                  << "Simulated games: "
                  << result.simulations << '\n'
                  << "Estimated probability of: "
                  << threshold << "+ points: " 
                  << result.probability * 100 << "%\n";

        auto summary = calculatePlayerAverages(allPlayers, playerId);

        std::cout << std::fixed << std::setprecision(2);

        std::cout << "Records loaded: " << allPlayers.size() << '\n'
                  << "Player: " << summary.playerName << '\n'
                  << "Games played: " << summary.gamesPlayed << '\n'
                  << "Average points: " << summary.avgPoints << '\n'
                  << "Average rebounds: " << summary.avgRebounds << '\n'
                  << "Average assists: " << summary.avgAssists << '\n';

        if (summary.pointsStdDev.has_value()) {
            std::cout << "Points SD: " << summary.pointsStdDev.value() << '\n';
        } else {
            std::cout << "Points SD: Not enough games.\n";
        }

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}
#include "HttpClient.h"
#include "EspnClient.h"
#include "Game.h"
#include "StatsCsv.h"
#include "PlayerAnalysis.h"
#include "simulation.h"
#include "FantasyScoring.h"

#include <curl/curl.h>
#include <iostream>
#include <exception>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

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

        std::vector<long long> weeklyTotals;

        for (const auto& filename : filenames) {
            auto records = loadPlayerStats(filename);

            allPlayers.insert(allPlayers.end(), records.begin(), records.end());
        }

        std::string playerId;
        int gamesPerWeek = 0;
        double threshold = 0.0;

        std::cout << "Enter player ID: ";
        std::cin >> playerId;

        std::cout << "Games in week: ";

        if (!(std::cin >> gamesPerWeek)) {
            throw std::invalid_argument("Invalid game count..");
        }

        std::cout << "Weekly fantasy-points threshold: ";

        if (!(std::cin >> threshold)) {
            throw std::invalid_argument("Invalid threshold.");
        }

        ScoringRules rules;

        auto result = simulateWeeklyFantasyPoints(
            allPlayers,
            playerId,
            gamesPerWeek,
            threshold,
            10000,
            rules
        );

        std::cout << std::fixed << std::setprecision(2);

        std::cout << "\nHistorical games: "
                  << result.historicalGames << '\n'
                  << "Simulated weeks: "
                  << result.simulations << '\n'
                  << "Games per week: "
                  << gamesPerWeek << '\n'
                  << "Mean weekly fantasy points: "
                  << result.meanPoints << '\n'
                  << "10th percentile: "
                  << result.p10 << '\n'
                  << "Median: "
                  << result.median << '\n'
                  << "90th percentile: "
                  << result.p90 << '\n'
                  << "Probability of "
                  << threshold << "+ fantasy points: "
                  << result.probability * 100.0 << "%\n";
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}
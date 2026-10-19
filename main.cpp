#include "HttpClient.h"
#include "EspnClient.h"
#include "Game.h"
#include "StatsCsv.h"
#include "PlayerAnalysis.h"
#include "simulation.h"
#include "FantasyScoring.h"
#include "HistoricalImport.h"

#include <curl/curl.h>
#include <iostream>
#include <exception>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>

int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        std::cerr << "Could not initialize curl.\n";
        return 1;
    }

    int exitCode = 0;

    try {

        char loadNewData;

        std::cout << "Load new data? (y/n): ";
        std::cin >> loadNewData;

        if (loadNewData == 'y' || loadNewData == 'Y') {
            EspnClient espn;

            std::string startDate;
            std::string endDate;

            std::cout << "Start date (YYYYMMDD): ";
            std::cin >> startDate;

            std::cout << "End date (YYYYMMDD): ";
            std::cin >> endDate;

            importDateRange(espn, startDate, endDate);
        }

        std::vector<PlayerGameStats> allPlayers;
        int filesLoaded = 0;

        std::vector<long long> weeklyTotals;

        for (const auto& entry : std::filesystem::directory_iterator("Data/expanded")) {
            if (!entry.is_regular_file() || entry.path().extension() != ".csv") {
                continue;
            }

            std::string filename = 
            "expanded/" + entry.path().filename().string();

            auto records = loadPlayerStats(filename);

            allPlayers.insert(
                allPlayers.end(),
                records.begin(),
                records.end()
            );

            ++filesLoaded;
        }

        std::cout << "Files loaded: " << filesLoaded << '\n'
                  << "Player records loaded: " << allPlayers.size() << '\n';

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
#include "HttpClient.h"
#include "EspnClient.h"
#include "Game.h"
#include "StatsCsv.h"
#include "PlayerAnalysis.h"
#include "simulation.h"
#include "FantasyScoring.h"
#include "HistoricalImport.h"
#include "Backtest.h"

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

        for (const auto& entry : std::filesystem::directory_iterator("Data/metadata")) {
            if (!entry.is_regular_file() || entry.path().extension() != ".csv") {
                continue;
            }

            std::string filename = 
            "metadata/" + entry.path().filename().string();

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

        std::vector<PlayerGameStats> filteredPlayers;

        for (const auto& record : allPlayers) {
            if (record.seasonType != 2) {
                continue;
            }

            if (record.seasonYear < 2022 || record.seasonYear > 2026) {
                continue;
            }

            filteredPlayers.push_back(record);
        }

        ScoringRules rules;

        const std::vector<std::string> playerIds = {
            "1966",
            "3202",
            "3975",
            "3945274",
            "4594268",
            "4065648"
        };

        std::ofstream report("Data/processed/backtest_results.csv");

        if (!report) {
            throw std::runtime_error(
                "Could not open backtest_results.csv"
            );
        }

        report << "player_id,groups_tested,"
               << "baseline_mae,rolling_mae,"
               << "baseline_bias,rolling_bias,"
               << "baseline_coverage_percent,rolling_coverage_percent,"
               << "baseline_interval_width,rolling_interval_width\n";
        
        report << std::fixed << std::setprecision(4);

        int successfulPlayers = 0;

        for (const auto& playerId : playerIds) {
            std::cout << "\nBacktesting player " << playerId << "...\n";

            try {
                auto result = backtestPlayer(
                    allPlayers,
                    playerId,
                    rules
                );

                report << result.playerId << ','
                       << result.groupsTested << ','
                       << result.baseline.meanAbsoluteError << ','
                       << result.rolling.meanAbsoluteError << ','
                       << result.baseline.meanSignedError << ','
                       << result.rolling.meanSignedError << ','
                       << result.baseline.coveragePercent << ','
                       << result.rolling.coveragePercent << ','
                       << result.baseline.averageIntervalWidth << ','
                       << result.rolling.averageIntervalWidth << '\n';
                
                ++successfulPlayers;
            } catch (const std::exception& error) {
                std::cerr << "Skipped player " << playerId
                          << ": " << error.what() << '\n';
            }
        }

        report.close();

        if (!report) {
            throw std::runtime_error(
                "Could not finish writing backtest_results.csv"
            );
        }

        std::cout << "\nSaved results for " << successfulPlayers
                  << " players to backtest_results.csv\n";

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}
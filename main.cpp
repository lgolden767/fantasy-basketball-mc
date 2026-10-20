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

        std::string playerId;
        ScoringRules rules;

        std::cout << "Enter player ID: ";
        std::cin >> playerId;

        printPlayerHistory(filteredPlayers, playerId);
        backtestPlayer(allPlayers, playerId, rules);

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}
#include "HttpClient.h"
#include "EspnClient.h"
#include "Game.h"
#include "StatsCsv.h"
#include "PlayerAnalysis.h"
#include "simulation.h"
#include "FantasyScoring.h"
#include "HistoricalImport.h"
#include "Backtest.h"
#include "schedule.h"

#include <curl/curl.h>
#include <iostream>
#include <exception>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <cmath>

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

        int mode = 0;

        std::cout << "\n1. List team schedule\n"
                  << "2. Run backtests\n"
                  << "0. Exit\n"
                  << "Input: ";

        if (!(std::cin >> mode)) {
            throw std::invalid_argument("Enter a valid menu number.");
        }

        if (mode == 1) {
            std::string teamId;
            std::string startDate;

            std::cout << "Enter team ID: ";

            if (!(std::cin >> teamId)) {
                throw std::invalid_argument("Invalid team ID.");
            }

            std::cout << "Week starts (YYYYMMDD): ";

            if (!(std::cin >> startDate)) {
                throw std::invalid_argument(
                    "Invalid start date."
                );
            }

            EspnClient espn;

            auto schedule = getTeamWeek(
                espn,
                teamId,
                startDate
            );

            std::cout << "\nRegular season games returned: "
                      << schedule.size() << '\n';

            for (const auto& game : schedule) {
                std::cout << game.gameDate << " | "
                          << game.awayTeam << " at "
                          << game.homeTeam << " | "
                          << game.status << '\n';
            }

            if (schedule.empty()) {
                std::cout << "No matching games returned for this week.\n";
            }
        } else if (mode == 2) {
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
                "1966",       // Lebron James
                "3202",       // Kevin Durant
                "3975",       // Steph Curry
                "3945274",    // Luka Doncic
                "4594268",    // Anthony Edwards
                "4065648",    // Jayson Tatum
                "3992",       // James Harden
                "3934672",    // Jalen Brunson
                "4432166",    // Cade Cunningham
                "4431678",    // Tyrese Maxey
                "3136193",    // Devin Booker
                "3908809",    // Donovan Mitchell
                "3078576",    // Derrick White
                "2991350",    // Alex Caruso
                "3062679",    // Josh Hart
                "3147657",    // Mikal Bridges
                "4433134",    // Scottie Barnes
                "4432573",    // Paolo Banchero
                "3032977",    // Giannis Antetokounmpo
                "6589",       // Draymond Green
                "3064290",    // Aaron Gordon
                "3112335",    // Nikola Jokic
                "4066261",    // Bam Adebayo
                "3032976",    // Rudy Gobert
                "3448",       // Brook Lopez
                "4871144"     // Alperen Sengun
            };

            const std::vector<std::size_t> historyWindows = {
                15,
                30,
                45,
                60
            };

            std::filesystem::create_directories("Data/processed");
            std::ofstream report("Data/processed/backtest_results.csv");

            if (!report) {
                throw std::runtime_error(
                    "Could not open backtest_results.csv"
                );
            }

            report << "player_id,history_window,groups_tested,"
                << "baseline_mae,rolling_mae,"
                << "baseline_bias,rolling_bias,"
                << "baseline_coverage_percent,rolling_coverage_percent,"
                << "baseline_interval_width,rolling_interval_width\n";
            
            report << std::fixed << std::setprecision(4);

            int successfulRuns = 0;

            std::vector<BacktestResult> allResults;

            for (const auto& playerId : playerIds) {
                for (std::size_t window : historyWindows) {
                    std::cout << "\nBacktesting player " << playerId 
                            << " | Window: " << window << " games\n";

                    try {
                        auto result = backtestPlayer(
                            allPlayers,
                            playerId,
                            rules,
                            window
                        );

                        allResults.push_back(result);

                        report  << result.playerId << ','
                                << result.historyWindow << ','
                                << result.groupsTested << ','
                                << result.baseline.meanAbsoluteError << ','
                                << result.rolling.meanAbsoluteError << ','
                                << result.baseline.meanSignedError << ','
                                << result.rolling.meanSignedError << ','
                                << result.baseline.coveragePercent << ','
                                << result.rolling.coveragePercent << ','
                                << result.baseline.averageIntervalWidth << ','
                                << result.rolling.averageIntervalWidth << '\n';
                        
                        ++successfulRuns;
                    } catch (const std::exception& error) {
                        std::cerr << "Skipped player " << playerId
                                << ": " << error.what() << '\n';
                    }
                }
            }

            std::ofstream summaryFile(
                "Data/processed/backtest_summary.csv"
            );

            if (!summaryFile) {
                throw std::runtime_error(
                    "Could not open backtest_summary.csv"
                );
            }

            summaryFile << "history_window,players,groups,"
                        << "baseline_player_mae,rolling_player_mae,"
                        << "baseline_group_mae,rolling_group_mae,"
                        << "players_improved_percent,"
                        << "baseline_coverage_percent,rolling_coverage_percent,"
                        << "baseline_interval_width,rolling_interval_width\n";
            
            summaryFile << std::fixed << std::setprecision(4);

            for (std::size_t window : historyWindows) {
                int playerCount = 0;
                int improvedCount = 0;
                long long totalGroups = 0;

                long long baselineBelow = 0;
                long long baselineInside = 0;
                long long baselineAbove = 0;

                long long rollingBelow = 0;
                long long rollingInside = 0;
                long long rollingAbove = 0;

                double baselinePlayerMaeTotal = 0.0;
                double rollingPlayerMaeTotal = 0.0;

                double baselineWeightedMaeTotal = 0.0;
                double rollingWeightedMaeTotal = 0.0;

                double baselineWeightedCoverageTotal = 0.0;
                double rollingWeightedCoverageTotal = 0.0;

                double baselineWeightedWidthTotal = 0.0;
                double rollingWeightedWidthTotal = 0.0;

                for (const auto& result : allResults) {
                    if (result.historyWindow != window ||
                        result.groupsTested <= 0) {
                        continue;
                    }

                    ++playerCount;
                    totalGroups += result.groupsTested;

                    baselineBelow += result.baseline.belowRange;
                    baselineInside += result.baseline.insideRange;
                    baselineAbove += result.baseline.aboveRange;

                    rollingBelow += result.rolling.belowRange;
                    rollingInside += result.rolling.insideRange;
                    rollingAbove += result.rolling.aboveRange;

                    double weight =
                        static_cast<double>(result.groupsTested);
                    
                    baselinePlayerMaeTotal +=
                        result.baseline.meanAbsoluteError;
                    
                    rollingPlayerMaeTotal +=
                        result.rolling.meanAbsoluteError;

                    baselineWeightedMaeTotal +=
                        result.baseline.meanAbsoluteError * weight;

                    rollingWeightedMaeTotal +=
                        result.rolling.meanAbsoluteError * weight;

                    baselineWeightedCoverageTotal +=
                        result.baseline.coveragePercent * weight;

                    rollingWeightedCoverageTotal +=
                        result.rolling.coveragePercent * weight;

                    baselineWeightedWidthTotal +=
                        result.baseline.averageIntervalWidth * weight;

                    rollingWeightedWidthTotal +=
                        result.rolling.averageIntervalWidth * weight;

                    if (result.rolling.meanAbsoluteError <
                        result.baseline.meanAbsoluteError) {
                        ++improvedCount;
                    }
                }
                
                if (playerCount == 0 || totalGroups == 0) continue;

                auto percent = [totalGroups](long long count) {
                    return 100.0 * static_cast<double>(count) /
                    static_cast<double>(totalGroups);
                };

                std::cout << std::fixed << std::setprecision(2)
                        << "\nInterval outcomes | Window: "
                        << window << '\n'
                        << "Baseline below / inside / above: "
                        << percent(baselineBelow) << "% / "
                        << percent(baselineInside) << "% / "
                        << percent(baselineAbove) << "% / "
                        << "Rolling below / inside / above: "
                        << percent(rollingBelow) << "% / "
                        << percent(rollingInside) << "% / "
                        << percent(rollingAbove) << "%\n";

                double baselinePlayerMae =
                    baselinePlayerMaeTotal / playerCount;

                double rollingPlayerMae =
                    rollingPlayerMaeTotal / playerCount;

                double baselineGroupMae = 
                    baselineWeightedMaeTotal / totalGroups;

                double rollingGroupMae =    
                    rollingWeightedMaeTotal / totalGroups;
                
                double improvedPercent =    
                    100.0 * improvedCount / playerCount;

                double baselineCoverage =
                    baselineWeightedCoverageTotal / totalGroups;

                double rollingCoverage =   
                    rollingWeightedCoverageTotal / totalGroups;

                double baselineWidth =
                    baselineWeightedWidthTotal / totalGroups;

                double rollingWidth =
                    rollingWeightedWidthTotal / totalGroups;

                    summaryFile << window << ','
                                << playerCount << ','
                                << totalGroups << ','
                                << baselinePlayerMae << ','
                                << rollingPlayerMae << ','
                                << baselineGroupMae << ','
                                << rollingGroupMae << ','
                                << improvedPercent << ','
                                << baselineCoverage << ','
                                << rollingCoverage << ','
                                << baselineWidth << ','
                                << rollingWidth << '\n';
                    
                    std::cout << std::fixed << std::setprecision(2)
                            << "\nWindow: " << window << " games\n"
                            << "Players: " << playerCount << '\n'
                            << "Test groups: " << totalGroups << '\n'
                            << "Equal-player MAE: "
                            << baselinePlayerMae << " -> "
                            << rollingPlayerMae << '\n'
                            << "Group-weighted MAE: "
                            << baselineGroupMae << " -> "
                            << rollingGroupMae << '\n'
                            << "Players improved: "
                            << improvedPercent << "%\n"
                            << "Coverage: "
                            << baselineCoverage << "% -> "
                            << rollingCoverage << "%\n"
                            << "Average interval width: "
                            << baselineWidth << " -> "
                            << rollingWidth << '\n';
            }

            summaryFile.close();
            report.close();

            if (!report) {
                throw std::runtime_error(
                    "Could not finish writing backtest_results.csv"
                );
            }
            if (!summaryFile) {
                throw std::runtime_error(
                    "Could not finish writing backtest_summary.csv"
                );
            }

            std::cout << "\nSaved " << successfulRuns
                    << " comparisons to "
                    << "Data/processed/backtest_results.csv\n";
            std::cout << "\nSaved Data/processed/backtest_summary.csv\n";
        } else if (mode != 0) {
            throw std::invalid_argument("Unknown menu option.");
        }

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}
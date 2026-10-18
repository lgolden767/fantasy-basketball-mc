#include "StatsCsv.h"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace {
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

    std::vector<std::string> parseCsvRow(const std::string& line) {
        std::vector<std::string> fields;
        std::string field;
        bool insideQuotes = false;

        for (std::size_t i = 0; i < line.size(); ++i) {
            char character = line[i];

            if (character == '"') {
                if (insideQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                        field += '"';
                        ++i;
                    } else { 
                        insideQuotes = !insideQuotes;
                    }
            } else if (character == ',' && !insideQuotes) {
                fields.push_back(field);
                field.clear();
            } else {
                field += character;
            }
        }

        if (insideQuotes) {
            throw std::runtime_error("Unclosed quotes in CSV row.");
        }

        fields.push_back(field);

        return fields;
    }
}

void savePlayerStats(
    const std::vector<PlayerGameStats>& players,
    const std::string& filename
) {
    std::ofstream file("Data/" + filename);

    if (!file) {
        throw std::runtime_error("Could not open CSV file.");
    }

    file << "game_id,player_id,player_name,did_not_play,"
        "minutes,points,rebounds,assists,steals,blocks,"
        "turnovers,fgm,fga,ftm,fta,three_pm,three_pa\n";

    for (const auto& player : players) {
        file << csvQuote(player.gameId) << ","
                << csvQuote(player.playerId) << ","
                << csvQuote(player.playerName) << ","
                << csvQuote(player.didNotPlay ? "true" : "false") << ",";

        if (player.didNotPlay) {
            for (int column = 0; column < 13; ++column) {
                if (column > 0) {
                    file << ',';
                }
            }

            file << '\n';
        }
        else {
            file << csvQuote(player.minutes) << ','
                    << player.points << ','
                    << player.rebounds << ','
                    << player.assists << ','
                    << player.steals << ','
                    << player.blocks << ','
                    << player.turnovers << ','
                    << player.fieldGoalsMade << ','
                    << player.fieldGoalsAttempted << ','
                    << player.freeThrowsMade << ','
                    << player.freeThrowsAttempted << ','
                    << player.threePointersMade << ','
                    << player.threePointersAttempted << '\n';
        }
        
    }

    file.close();

    if (!file) {
        throw std::runtime_error("Could not finish writing CSV.");
    }
}

std::vector<PlayerGameStats> loadPlayerStats(
    const std::string& filename
) {
    std::ifstream file("Data/" + filename);

    if (!file) {
        throw std::runtime_error(
            "Could not load CSV."
        );
    }

    std::string line;

    if (!std::getline(file, line)) {
        throw std::runtime_error("CSV is empty: " + filename);
    }

    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    const std::vector<std::string> expectedHeader = {
        "game_id",
        "player_id",
        "player_name",
        "did_not_play",
        "minutes",
        "points",
        "rebounds",
        "assists",
        "steals",
        "blocks",
        "turnovers",
        "fgm",
        "fga",
        "ftm",
        "fta",
        "three_pm",
        "three_pa"
    };

    if (parseCsvRow(line) != expectedHeader) {
        throw std::runtime_error("Unexpected CSV Header: " + filename);
    }

    std::vector<PlayerGameStats> records;
    int lineNumber = 1;

    while (std::getline(file, line)) {
        ++lineNumber;

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        try{
            auto fields = parseCsvRow(line);

            if (fields.size() != 17) {
                throw std::runtime_error("Expected 17 fields.");
            }

            PlayerGameStats player;

            player.gameId       = fields.at(0);
            player.playerId     = fields.at(1);
            player.playerName   = fields.at(2);

            if (fields.at(3) == "true") {
                player.didNotPlay = true;
            } else if (fields.at(3) == "false") {
                player.didNotPlay = false;
            } else {
                throw std::runtime_error("Invalid DNP status: " + player.playerName);
            }

            if (!player.didNotPlay) {
                player.minutes = fields.at(4);

                auto ReadStat = [](const std::string& text) {
                    std::size_t consumed = 0;
                    int value = std::stoi(text, &consumed);

                    if (consumed != text.size() || value < 0 ) {
                        throw std::runtime_error ("Invalid statistic: " + text);
                    }

                    return value;
                };

                player.points                   = ReadStat(fields.at(5));
                player.rebounds                 = ReadStat(fields.at(6));
                player.assists                  = ReadStat(fields.at(7));
                player.steals                   = ReadStat(fields.at(8));
                player.blocks                   = ReadStat(fields.at(9));
                player.turnovers                = ReadStat(fields.at(10));
                player.fieldGoalsMade           = ReadStat(fields.at(11));
                player.fieldGoalsAttempted      = ReadStat(fields.at(12));
                player.freeThrowsMade           = ReadStat(fields.at(13));
                player.freeThrowsAttempted      = ReadStat(fields.at(14));
                player.threePointersMade        = ReadStat(fields.at(15));
                player.threePointersAttempted   = ReadStat(fields.at(16));
            }

            records.push_back(player);
        } catch (const std::exception& error) {
            throw std::runtime_error(
                filename + ", line " + std::to_string(lineNumber) +
                ": " + error.what()
            );
        }
    }
    
    if (file.bad()) {
        throw std::runtime_error(
            "Error reading CSV: " + filename
        );
    }

    return records;
}
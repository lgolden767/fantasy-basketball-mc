#include "StatsCsv.h"

#include <fstream>
#include <stdexcept>
#include <iostream>

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
}

void savePlayerStats(
    const std::vector<PlayerGameStats>& players,
    const std::string& filename
) {
    std::ofstream file("Data/" + filename);

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
}
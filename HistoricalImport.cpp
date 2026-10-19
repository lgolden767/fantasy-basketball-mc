#include "HistoricalImport.h"
#include "StatsCsv.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace {
    std::chrono::sys_days parseDate(const std::string& text) {
        if (text.size() != 8 || text.find_first_not_of("0123456789") != std::string::npos) {
            throw std::invalid_argument("Use YYYYMMDD dates.");
        }

        int y = std::stoi(text.substr(0, 4));
        unsigned m = std::stoul(text.substr(4, 2));
        unsigned d = std::stoul(text.substr(6, 2));

        std::chrono::year_month_day date{
            std::chrono::year{y},
            std::chrono::month{m},
            std::chrono::day{d}
        };

        if (y < 1 || !date.ok()) {
            throw std::invalid_argument("Invalid date: " + text);
        }

        return std::chrono::sys_days{date};
    }

    std::string formatDate(std::chrono::sys_days day) {
        std::chrono::year_month_day date{day};

        std::ostringstream output;

        output << std::setfill('0')
               << std::setw(4) << static_cast<int>(date.year())
               << std::setw(2) << static_cast<unsigned>(date.month())
               << std::setw(2) << static_cast<unsigned>(date.day());

        return output.str();
    }
}

void importDateRange(
    EspnClient& espn,
    const std::string& startDate,
    const std::string& endDate,
    bool overwrite
) {
    auto first = parseDate(startDate);
    auto last = parseDate(endDate);

    if (first > last) {
        throw std::invalid_argument(
            "Start date must be before or equal to end date."
        );
    }

    std::filesystem::create_directories("Data/expanded");

    int saved = 0;
    int skipped = 0;
    int failed = 0;

    for (auto day = first; day <= last; day += std::chrono::days{1}) {
        std::string date = formatDate(day);

        std::cout << "\nFetching" << date << "...\n";

        try {
            auto games = espn.getGames(date);

            std::this_thread::sleep_for(
                std::chrono::seconds{1}
            );

            for (const auto& game : games) {
                if (!game.completed) {
                    continue;
                }

                std::string filename = "expanded/" + game.id + "_stats.csv";

                std::filesystem::path path = std::filesystem::path{"Data"} / filename;

                if (!overwrite && std::filesystem::exists(path)) {
                    ++skipped;
                    continue;
                }

                try {
                    auto players = espn.getPlayerStats(game.id);

                    if (players.empty()) {
                        throw std::runtime_error("No player statistics returned.");
                    }

                    savePlayerStats(players, filename);

                    std::cout << "Saved " << path.string() << '\n';
                    ++saved;
                } catch (const std::exception& error) {
                    ++failed;

                    std::cerr << "Failed game " << game.id
                              << ": " << error.what() << '\n';
                }

                std::this_thread::sleep_for(
                    std::chrono::seconds{1}
                );
            }
        } catch (const std::exception& error) {
            ++failed;

            std::cerr << "Failed date " << date
                      << ": " << error.what() << '\n';

                    std::this_thread::sleep_for(
                        std::chrono::seconds{1}
                    );
        }
    }

    std::cout << "\nSaved: " << saved
              << "\nExisting files skipped: " << skipped
              << "\nFailed games or dates: " << failed << '\n';
}
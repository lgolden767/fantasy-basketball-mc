#include "schedule.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace {
    std::chrono::sys_days parseDate(const std::string& text) {
        if (text.size() != 8 ||
            text.find_first_not_of("0123456789") !=
            std::string::npos) {
            throw std::invalid_argument(
                "Use YYYYMMDD for the start date."
            );
        }

        int year = std::stoi(text.substr(0, 4));
        unsigned month = std::stoul(text.substr(4, 2));
        unsigned day = std::stoul(text.substr(6, 2));

        std::chrono::year_month_day date{
            std::chrono::year{year},
            std::chrono::month{month},
            std::chrono::day{day}
        };

        if (year < 1 || !date.ok()) {
            throw std::invalid_argument(
                "Invalid date: " + text
            );
        }

        return std::chrono::sys_days{date};
    }

    std::string formatDate(std::chrono::sys_days day) {
        std::chrono::year_month_day date{day};

        std::ostringstream output;

        output << std::setfill('0')
               << std::setw(4) << static_cast<int>(date.year())
               << std::setw(2)
               << static_cast<unsigned>(date.month())
               << std::setw(2)
               << static_cast<unsigned>(date.day());
        
        return output.str();
    }
}

std::vector<Game> getTeamWeek(
    EspnClient& espn,
    const std::string& teamId,
    const std::string& startDate
) {
    if (teamId.empty() ||
        teamId.find_first_not_of("0123456789") !=
            std::string::npos) {
        throw std::invalid_argument(
            "Team ID must contain only digits."
        );
    }

    const auto firstDay = parseDate(startDate);

    std::vector<Game> schedule;
    std::unordered_set<std::string> countedGames;

    for (int offset = 0; offset < 7; ++offset) {
        const auto day =
            firstDay + std::chrono::days(offset);

        const std::string date = formatDate(day);

        auto games = espn.getGames(date);

        for (const auto& game : games) {
            if (game.homeTeamId != teamId &&
                game.awayTeamId != teamId) {
                continue;
            }

            if (game.seasonType != 2) {
                continue;
            }

            if (!countedGames.insert(game.id).second) {
                continue;
            }

            schedule.push_back(game);
        }
    }

    std::sort(
        schedule.begin(),
        schedule.end(),
        [](const Game& a, const Game& b) {
            if (a.gameDate == b.gameDate) {
                return a.id < b.id;
            }

            return a.gameDate < b.gameDate;
        }
    );

    return schedule;
}
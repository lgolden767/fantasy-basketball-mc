#include "HttpClient.h"
#include "EspnClient.h"
#include "Game.h"
#include "PlayerGameStats.h"

#include <nlohmann/json.hpp>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <iostream>

namespace {
    int readNonnegativeInt (const std::string& text) {
        std::size_t consumed = 0;
        int value = std::stoi(text, &consumed);

        if (consumed != text.size() || value < 0) {
            throw std::runtime_error("Invalid statistics: " + text);
        }

        return value;
    }

    std::pair<int, int> parseShooting(
        const std::string& text
    ) {
        auto seperator = text.find('-');

        if (seperator == std::string::npos) {
            throw std::runtime_error("Expected made-attempted shooting stat: " + text);
        }

        int made = readNonnegativeInt(
            text.substr(0, seperator)
        );

        int attempted = readNonnegativeInt(
            text.substr(seperator + 1)
        );

        if (made > attempted) {
            throw std::runtime_error(
                "Made shots exceed attempts: " + text
            );
        }

        return {made, attempted};
    }
}

std::vector<std::string> EspnClient::getGameNames()
{
    std::string response = http.get(
        "https://site.api.espn.com/apis/site/v2/"
        "sports/basketball/nba/scoreboard");

    auto data = nlohmann::json::parse(response);

    std::vector<std::string> names;

    for (const auto &game : data.at("events"))
    {
        names.push_back(
            game.at("name").get<std::string>());
    }

    return names;
}

std::vector<Game> EspnClient::getGames(const std::string& date)
{
    std::string response = http.get(
        "https://site.api.espn.com/apis/site/v2/"
        "sports/basketball/nba/scoreboard?dates=" + date
    );

    auto data = nlohmann::json::parse(response);

    std::vector<Game> games;

    for (const auto &event : data.at("events"))
    {
        Game game;

        std::cout << "Event: " << event.at("id") << '\n'
                  << "Date: " << event.at("date") << '\n'
                  << "Season: "
                  << event.value("season", nlohmann::json::object()).dump(2)
                  << '\n';

        game.gameDate = event.at("date").get<std::string>();
        game.seasonYear = event.at("season").at("year").get<int>();
        game.seasonType = event.at("season").at("type").get<int>();

        game.id = event.at("id").get<std::string>();

        game.status = event.at("status")
                          .at("type")
                          .at("description")
                          .get<std::string>();

        game.completed = event.at("status")
                              .at("type")
                              .at("completed")
                              .get<bool>();

        const auto &competitors = event.at("competitions")
                                      .at(0)
                                      .at("competitors");

        for (const auto &competitor : competitors)
        {
            std::string side =
                competitor.at("homeAway").get<std::string>();

            std::string teamName = competitor.at("team")
                                       .at("displayName")
                                       .get<std::string>();

            if (side == "home")
            {
                game.homeTeam = teamName;
            }
            else if (side == "away")
            {
                game.awayTeam = teamName;
            }
        }

        games.push_back(game);
    }

    return games;
}

std::vector<PlayerGameStats> EspnClient::getPlayerStats(const std::string &gameId)
{
    std::string response = http.get(
        "https://site.api.espn.com/apis/site/v2/"
        "sports/basketball/nba/summary?event=" +
        gameId);

    auto data = nlohmann::json::parse(response);

    if (!data.contains("boxscore") ||
        !data.at("boxscore").contains("players"))
    {
        throw std::runtime_error("No player boxscore available.");
    }

    std::vector<PlayerGameStats> results;

    for (const auto &team : data.at("boxscore").at("players"))
    {
        for (const auto &group : team.at("statistics"))
        {
            auto labels = group.at("labels").get<std::vector<std::string>>();

            auto findIndex = [&](const std::string &label)
            {
                auto position =
                    std::find(labels.begin(), labels.end(), label);

                if (position == labels.end())
                {
                    throw std::runtime_error(
                        "Missing statistic label: " + label);
                }

                return static_cast<std::size_t>(
                    position - labels.begin());
            };

            const auto minutesIndex         = findIndex("MIN");
            const auto pointsIndex          = findIndex("PTS");
            const auto reboundsIndex        = findIndex("REB");
            const auto assistsIndex         = findIndex("AST");
            const auto stealsIndex          = findIndex("STL");
            const auto blocksIndex          = findIndex("BLK");
            const auto turnoversIndex       = findIndex("TO");
            const auto fieldGoalsIndex      = findIndex("FG");
            const auto threePointersIndex   = findIndex("3PT");
            const auto freeThrowsIndex      = findIndex("FT");

            for (const auto &entry : group.at("athletes"))
            {
                PlayerGameStats player;

                player.gameId = gameId;
                player.playerId = entry.at("athlete")
                                      .at("id")
                                      .get<std::string>();
                player.playerName = entry.at("athlete")
                                        .at("displayName")
                                        .get<std::string>();
                player.didNotPlay = entry.value("didNotPlay", false);

                if (!player.didNotPlay)
                {
                    const auto &stats = entry.at("stats");

                    auto textAt = [&](std::size_t index) {
                        return stats.at(index).get<std::string>();
                    };

                    player.minutes                  = textAt(minutesIndex);
                    player.points                   = readNonnegativeInt(textAt(pointsIndex));
                    player.rebounds                 = readNonnegativeInt(textAt(reboundsIndex));
                    player.assists                  = readNonnegativeInt(textAt(assistsIndex));
                    player.steals                   = readNonnegativeInt(textAt(stealsIndex));
                    player.blocks                   = readNonnegativeInt(textAt(blocksIndex));
                    player.turnovers                = readNonnegativeInt(textAt(turnoversIndex));
                    auto fieldGoals                 = parseShooting(textAt(fieldGoalsIndex));
                    player.fieldGoalsMade           = fieldGoals.first;
                    player.fieldGoalsAttempted      = fieldGoals.second;
                    auto threes                     = parseShooting(textAt(threePointersIndex));
                    player.threePointersMade        = threes.first;
                    player.threePointersAttempted   = threes.second;
                    auto freeThrows                 = parseShooting(textAt(freeThrowsIndex));
                    player.freeThrowsMade           = freeThrows.first;
                    player.freeThrowsAttempted      = freeThrows.second;

                    int calculatedPoints =
                        2 * player.fieldGoalsMade
                        + player.threePointersMade
                        + player.freeThrowsMade;

                    if (calculatedPoints != player.points) {
                        throw std::runtime_error(
                            "Shooting totals do not match points for " 
                            + player.playerName
                        );
                    }
                }

                results.push_back(player);
            }
        }
    }

    return results;
}
#include "HttpClient.h"
#include "EspnClient.h"
#include "Game.h"
#include "PlayerGameStats.h"

#include <nlohmann/json.hpp>
#include <string>
#include <algorithm>
#include <stdexcept>

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
        "sports/basketball/nba/scoreboard?dates" + date
    );

    auto data = nlohmann::json::parse(response);

    std::vector<Game> games;

    for (const auto &event : data.at("events"))
    {
        Game game;

        game.id = event.at("id").get<std::string>();

        game.status = event.at("status")
                          .at("type")
                          .at("description")
                          .get<std::string>();

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

            const auto minutesIndex = findIndex("MIN");
            const auto pointsIndex = findIndex("PTS");
            const auto reboundsIndex = findIndex("REB");
            const auto assistsIndex = findIndex("AST");

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

                    player.minutes =
                        stats.at(minutesIndex).get<std::string>();
                    player.points = std::stoi(
                        stats.at(pointsIndex).get<std::string>());
                    player.rebounds = std::stoi(
                        stats.at(reboundsIndex).get<std::string>());
                    player.assists = std::stoi(
                        stats.at(assistsIndex).get<std::string>());
                }

                results.push_back(player);
            }
        }
    }

    return results;
}
#include "HttpClient.h"
#include "EspnClient.h"
#include "Game.h"

#include <nlohmann/json.hpp>
#include <string>

std::vector<std::string> EspnClient::getGameNames() {
    std::string response = http.get(
        "https://site.api.espn.com/apis/site/v2/"
        "sports/basketball/nba/scoreboard"
    );

    auto data = nlohmann::json::parse(response);

    std::vector<std::string> names;

    for (const auto& game : data.at("events")) {
        names.push_back(
            game.at("name").get<std::string>()
        );
    }

    return names;
}

std::vector<Game> EspnClient::getGames() {
    std::string response = http.get(
        "https://site.api.espn.com/apis/site/v2/"
        "sports/basketball/nba/scoreboard"
    );

    auto data = nlohmann::json::parse(response);

    std::vector<Game> games;

    for (const auto& event : data.at("events")) {
        Game game;

        game.id = event.at("id").get<std::string>();

        game.status = event.at("status")
                           .at("type")
                           .at("description")
                           .get<std::string>();

        const auto& competitors = event.at("competitions")
                                       .at(0)
                                       .at("competitors");

        for (const auto& competitor : competitors) {
            std::string side = 
                competitor.at("homeAway").get<std::string>();

            std::string teamName = competitor.at("team")
                                             .at("displayName")
                                             .get<std::string>();

            if (side == "home"){
                game.homeTeam = teamName;
            } else if (side == "away") {
                game.awayTeam = teamName;
            }
        }

        games.push_back(game);
    }

    return games;
}
#include "EspnClient.h"
#include <nlohmann/json.hpp>

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
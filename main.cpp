#include "HttpClient.h"

#include <curl/curl.h>
#include <iostream>
#include <exception>

int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        std::cerr << "Could not initialize curl.\n";
        return 1;
    }

    int exitCode = 0;

    try {
        HttpClient client;

        std::string response = client.get(
            "https://site.api.espn.com/apis/site/v2/"
            "sports/basketball/nba/scoreboard"
        );

        std::cout << response << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}
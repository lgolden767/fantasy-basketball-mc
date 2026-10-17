#include "HttpClient.h"
#include "EspnClient.h"

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
        EspnClient espn;

        auto names = espn.getGameNames();

        for (const auto& name : names) {
            std::cout << name << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}
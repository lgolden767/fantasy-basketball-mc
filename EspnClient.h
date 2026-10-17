#pragma once

#include "HttpClient.h"
#include <string>
#include <vector>

class EspnClient{
    private:
        HttpClient http;

    public:
        std::vector<std::string> getGameNames();
};
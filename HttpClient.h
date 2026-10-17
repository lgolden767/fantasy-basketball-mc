#pragma once
#include <string>

class HttpClient {
    public:
        std::string get(const std::string& url);
};
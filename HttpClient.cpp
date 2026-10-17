#include "HttpClient.h"
#include <curl/curl.h>
#include <stdexcept>
#include <string>

namespace {
        std::size_t receiveData(
            char* data,
            std::size_t size,
            std::size_t count,
            void *destination
        ) {
            std::size_t bytes = size * count;

            auto* response = static_cast<std::string*>(destination);

            try {
                response -> append(data, bytes);
                return bytes;
            } catch (...) {
                return 0;
            }
        }
    }

std::string HttpClient::get(const std::string& url) {
    std::string response;

    CURL* request = curl_easy_init();
    
    if (!request) {
        throw std::runtime_error("Could not create curl handle");
    }

    curl_easy_setopt(request, CURLOPT_URL, url.c_str());
    curl_easy_setopt(request, CURLOPT_WRITEFUNCTION, receiveData);
    curl_easy_setopt(request, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(request, CURLOPT_TIMEOUT, 20L);

    CURLcode result = curl_easy_perform(request);

    long status = 0;
    curl_easy_getinfo(request, CURLINFO_RESPONSE_CODE, &status);

    curl_easy_cleanup(request);

    if (result != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(result));
    }

    if (status < 200 || status >= 300) {
        throw std::runtime_error (
            "HTTP error: " + std::to_string(status)
        );
    }
        
    return response;
}
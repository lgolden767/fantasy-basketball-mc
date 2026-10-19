#pragma once

#include "EspnClient.h"
#include <string>

void importDateRange(
    EspnClient& espn,
    const std::string& startDate,
    const std::string& endDate,
    bool overwrite = false
);
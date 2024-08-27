//
// Created by Cooper Larson on 8/26/24.
//

#ifndef FORTI_HOLE_FORTITHREATFEEDMANAGER_H
#define FORTI_HOLE_FORTITHREATFEEDMANAGER_H

#include "include/Scraper.h"
#include <yaml-cpp/yaml.h>

class FortiThreatFeedManager {
    Scraper scraper;
    YAML::Node config;
    unsigned int min_category, max_category, base_category;

public:
    FortiThreatFeedManager();

    void update_threat_feed();
};


#endif //FORTI_HOLE_FORTITHREATFEEDMANAGER_H

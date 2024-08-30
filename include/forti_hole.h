//
// Created by Cooper Larson on 8/26/24.
//

#ifndef FORTI_HOLE_FORTI_HOLE_H
#define FORTI_HOLE_FORTI_HOLE_H

#include "include/blocklist_scraper.h"
#include <yaml-cpp/yaml.h>

class FortiHole {
    BlocklistScraper scraper;
    YAML::Node config;
    unsigned int min_category, max_category, base_category;

    static void remove_extra_files(const std::string& naming_prefix, unsigned int index);

public:
    FortiHole();

    void update_threat_feeds();
};


#endif //FORTI_HOLE_FORTI_HOLE_H

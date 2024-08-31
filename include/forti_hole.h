//
// Created by Cooper Larson on 8/26/24.
//

#ifndef FORTI_HOLE_FORTI_HOLE_H
#define FORTI_HOLE_FORTI_HOLE_H

#include "include/blocklist_scraper.h"
#include "include/config.h"
#include <yaml-cpp/yaml.h>

class FortiHole {
    static constexpr unsigned int MAX_LINES_PER_FILE = 131000;
    BlocklistScraper scraper;
    Config config;
    std::unordered_map<unsigned int, std::unordered_set<std::string>> lists_by_security_level;

    void merge();
    void create_file(const std::string& filename, const std::vector<std::string>& lines) const;
    static void remove_all_custom_threat_feeds();
    static void remove_extra_files(const std::string& naming_prefix, unsigned int index);
    void enable_dns_filters(const std::string& naming_prefix, unsigned int num_files);
    void update_security_policies();

public:
    FortiHole();

    void update_threat_feeds();
};


#endif //FORTI_HOLE_FORTI_HOLE_H

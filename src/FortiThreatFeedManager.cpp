//
// Created by Cooper Larson on 8/26/24.
//

#include "include/FortiThreatFeedManager.h"
#include <forti_api/threat_feed.hpp>

FortiThreatFeedManager::FortiThreatFeedManager() : scraper(), config(scraper.config) {
    auto categories = config["categories"];
    min_category = categories["min"].as<unsigned int>();
    max_category = categories["max"].as<unsigned int>();
    base_category = categories["base"].as<unsigned int>();
}

void FortiThreatFeedManager::update_threat_feed() {
    auto lists = scraper();

    for (unsigned int security_level = 0; security_level < lists.size(); ++security_level) {
        unsigned int num_files = lists[security_level].size();
        auto category = base_category + security_level;

        std::string file_name_prefix = "forti-hole_security-level-" + std::to_string(security_level) + "_part-";

        for (unsigned int file_index = 0; file_index < num_files; ++file_index) {
            std::string file_name = file_name_prefix + std::to_string(file_index + 1);

            if (!ThreatFeed::contains(file_name)) ThreatFeed::add(file_name, category);

            ThreatFeed::update_feed(CommandEntry(file_name, "snapshot", lists[security_level][file_index]));
        }

        auto index = num_files;
        auto getFileName = [&]() { return file_name_prefix + std::to_string(index); };

        while (ThreatFeed::contains(getFileName())) {
            ThreatFeed::del(getFileName());
            ++index;
        }
    }
}

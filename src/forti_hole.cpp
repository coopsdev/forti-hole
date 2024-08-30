//
// Created by Cooper Larson on 8/26/24.
//

#include "include/forti_hole.h"
#include <forti_api/threat_feed.hpp>

FortiHole::FortiHole() : scraper(), config(scraper.config) {
    auto categories = config["categories"];
    min_category = categories["min"].as<unsigned int>();
    max_category = categories["max"].as<unsigned int>();
    base_category = categories["base"].as<unsigned int>();
}

void FortiHole::update_threat_feeds() {
    auto lists = scraper();

    for (unsigned int security_level = 0; security_level < lists.size(); ++security_level) {
        unsigned int num_files = lists[security_level].size();
        auto category = base_category + security_level;

        auto file_name_prefix = std::format("forti-hole_security-level-{}_part-", std::to_string(security_level));

        for (unsigned int file_index = 0; file_index < num_files; ++file_index) {
            std::string file_name = file_name_prefix + std::to_string(file_index + 1);

            if (!ThreatFeed::contains(file_name)) ThreatFeed::add(file_name, category);

            ThreatFeed::update_feed(CommandEntry(file_name, lists[security_level][file_index]));
        }

        remove_extra_files(file_name_prefix, num_files + 1);
    }
}

void FortiHole::remove_extra_files(const std::string &naming_prefix, unsigned int index) {
    while (true) {
        auto filename = naming_prefix + std::to_string(index);
        if (!ThreatFeed::contains(filename)) return;
        ThreatFeed::del(filename);
        ++index;
    }
}

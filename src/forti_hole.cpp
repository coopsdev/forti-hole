//
// Created by Cooper Larson on 8/26/24.
//

#include "include/forti_hole.h"
#include <forti_api.hpp>
#include <thread>
#include <filesystem>
#include <fstream>

FortiHole::FortiHole() : scraper(), config(scraper.config) {}

void FortiHole::update_threat_feeds() {
    scraper();
    auto category = config.categories.base;

    for (const auto& entry : std::filesystem::directory_iterator("output")) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().string();

            std::ifstream infile(filename);
            if (!infile) {
                std::cerr << "Failed to open file: " << filename << std::endl;
                continue;
            }

            std::vector<std::string> lines;
            std::string line;
            while (std::getline(infile, line)) lines.push_back(line);

            infile.close();

            if (!ThreatFeed::contains(filename)) ThreatFeed::add(filename, category);
            ThreatFeed::update_feed({filename, lines});

            // give the FortiGate a chance to process the new data,
            // prevents network interruptions from buffer overflow
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ++category;
        }
    }
}

// TODO: Reintegrate this in a more robust manner
void FortiHole::remove_extra_files(const std::string &naming_prefix, unsigned int index) {
    while (true) {
        auto filename = naming_prefix + std::to_string(index);
        if (!ThreatFeed::contains(filename)) return;
        ThreatFeed::del(filename);
        ++index;
    }
}

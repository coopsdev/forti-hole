//
// Created by Cooper Larson on 8/26/24.
//

#include "include/forti_hole.h"
#include <forti_api.hpp>
#include <thread>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

FortiHole::FortiHole() : scraper(), config(scraper.config) {
    lists_by_security_level = scraper();
    merge();
}

void FortiHole::merge() {
    std::cout << "Consolidating data..." << std::endl;
    for (unsigned int i = lists_by_security_level.size() - 1; i > 0; --i) {
        for (const auto& item : lists_by_security_level[i]) {
            lists_by_security_level[i - 1].insert(item);
        }
    }
}

void FortiHole::update_threat_feeds() {
    std::cout << config.remove_all_threat_feeds_on_run << std::endl;
    if (config.remove_all_threat_feeds_on_run) remove_all_custom_threat_feeds();

    auto category = config.categories.base;

    std::cout << "Building ThreatFeeds..." << std::endl;
    if (!std::filesystem::exists(config.output_dir)) std::filesystem::create_directories(config.output_dir);

    for (const auto& [security_level, _] : lists_by_security_level) {
        std::cout << "Resizing files to fit on FortiGate..." << std::endl;

        auto& lines = lists_by_security_level[security_level];
        size_t total_lines = lines.size();
        size_t file_count = std::max((total_lines + MAX_LINES_PER_FILE - 1) / MAX_LINES_PER_FILE, static_cast<size_t>(1));
        size_t lines_per_file = total_lines / file_count;
        size_t extra = total_lines % file_count;

        std::cout << "Security Level " << security_level << " Files: " << file_count
                  << ", LPF: " << lines_per_file << std::endl;

        std::string file_base = "forti-hole_security_level_" + std::to_string(security_level) + "_part_";

        auto iter = lines.begin();
        for (size_t i = 0; i < file_count; ++i) {
            std::string filename = file_base + std::to_string(i + 1);

            std::vector<std::string> to_upload;
            to_upload.reserve(lines_per_file + extra);

            size_t count = lines_per_file + (i < extra ? 1 : 0);
            for (size_t j = 0; j < count; ++j) {
                to_upload.push_back(*iter);
                ++iter;
            }

            std::cout << "Built file: " << filename << " with " << to_upload.size() << " lines." << std::endl;

            if (config.write_files_to_disk) create_file(filename, to_upload);

            if (!ThreatFeed::contains(filename)) ThreatFeed::add(filename, category);
            ThreatFeed::update_feed({{filename, to_upload}});
            std::cout << "Successfully uploaded to FortiGate: " << filename << std::endl;

            // give the FortiGate a chance to process the new data,
            // prevents network interruptions from buffer overflow
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ++category;
        }

        remove_extra_files(file_base, file_count + 1);
    }
}

void FortiHole::create_file(const std::string& filename, const std::vector<std::string>& lines) const {
    std::string filename_w_extension = std::filesystem::current_path().string() + '/' + config.output_dir
                                       + '/' + filename + ".txt";

    std::ofstream outfile(filename_w_extension);
    if (!outfile) {
        std::cerr << "Failed to create file: " << filename_w_extension << std::endl;
        return;
    }

    for (const auto& line : lines) outfile << line << "\n";

    outfile.close();
    std::cout << "Successfully wrote file: " << filename_w_extension << std::endl;
}

void FortiHole::remove_all_custom_threat_feeds() {
    auto size = ThreatFeed::get().size();
    for (const auto& connector : ThreatFeed::get()) { ThreatFeed::del(connector.name); }
    std::cout << "Successfully removed " << size << " threat feeds..." << std::endl;
}

void FortiHole::remove_extra_files(const std::string &naming_prefix, unsigned int index) {
    while (true) {
        auto filename = naming_prefix + std::to_string(index);
        if (!ThreatFeed::contains(filename)) return;
        ThreatFeed::del(filename);
        ++index;
    }
}

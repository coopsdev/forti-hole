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
    build_threat_feed_info();
    std::cout << "\n Successfully updated FortiGate threat feeds!"
}

void FortiHole::merge() {
    std::cout << "\nConsolidating data...\n" << std::endl;
    for (unsigned int i = lists_by_security_level.size() - 1; i > 0; --i) {
        for (const auto& item : lists_by_security_level[i]) {
            lists_by_security_level[i - 1].insert(item);
        }
    }
}

void FortiHole::build_threat_feed_info() {
    info_by_security_level.reserve(lists_by_security_level.size());
    for (auto & lines : lists_by_security_level) {
        auto total_lines = lines.second.size();
        auto file_count = std::max((total_lines + MAX_LINES_PER_FILE - 1) / MAX_LINES_PER_FILE, static_cast<size_t>(1));
        info_by_security_level.emplace_back(total_lines, file_count);
    }
}

void FortiHole::resize_plus_toggle_filters_and_policies() {
    for (unsigned int security_level = 0; security_level < info_by_security_level.size(); ++security_level) {
        auto info = info_by_security_level[security_level];
        for (unsigned int file_index = 1; file_index <= info.file_count; ++file_index) {
            auto file_name = get_file_name(security_level, file_index);

            auto category = config.categories.base + (security_level + 1) * file_index;

            if (!ThreatFeed::contains(file_name))
                ThreatFeed::add(file_name, category);

            for (const auto& filter : config.forti_hole_automated_dns_filters) {

                // enable necessary DNS filters
                if (filter.security_level <= security_level) {
                    auto dns_filter = DNSFilter::get(filter.name);
                    if (filter.access == "block") dns_filter.block_category(category);
                    else if (filter.access == "allow") dns_filter.allow_category(category);
                    else if (filter.access == "monitor") dns_filter.monitor_category(category);
                    else throw std::runtime_error("Not a valid DNSFilter setting: " + filter.access);
                }

                // enable necessary policies
                for (const auto& policy : filter.policies) {
                    // TODO: enable necessary policies for this dns filter
                }
            }
        }
    }
}

void FortiHole::update_threat_feeds() {
    if (config.remove_all_threat_feeds_on_run) remove_all_custom_threat_feeds();

    auto category = config.categories.base;

    std::cout << "Building ThreatFeeds...\n" << std::endl;
    if (!std::filesystem::exists(config.output_dir)) std::filesystem::create_directories(config.output_dir);

    for (const auto& [security_level, _] : lists_by_security_level) {
        std::cout << "Resizing files to fit on FortiGate...\n" << std::endl;

        auto info = info_by_security_level[security_level];

        std::cout << "Security Level " << security_level
                  << ": { "
                  << "Files: " << info.file_count
                  << ", LPF: " << info.lines_per_file
                  << " }" << std::endl;

        auto iter = lists_by_security_level[security_level].begin();
        for (size_t i = 0; i < info.file_count; ++i) {
            auto filename = get_file_name(security_level, i + 1);

            std::vector<std::string> to_upload;
            to_upload.reserve(info.lines_per_file + info.extra);

            size_t count = info.lines_per_file + (i < info.extra ? 1 : 0);
            for (size_t j = 0; j < count; ++j) {
                to_upload.push_back(*iter);
                ++iter;
            }

            std::cout << "Built file: " << filename << " with " << to_upload.size() << " lines." << std::endl;

            if (config.write_files_to_disk) create_file(filename, to_upload);

            if (!ThreatFeed::contains(filename)) ThreatFeed::add(filename, category);
            ThreatFeed::update_feed({{filename, to_upload}});
            std::cout << "\nSuccessfully uploaded to FortiGate: " << filename << std::endl;

            // give the FortiGate a chance to process the new data,
            // prevents network interruptions from buffer overflow
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ++category;
        }

        remove_extra_files(security_level, info.file_count);
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

void FortiHole::remove_extra_files(unsigned int security_level, unsigned int file_index) {
    while (true) {
        auto filename = get_file_name(security_level, file_index);
        if (!ThreatFeed::contains(filename)) return;
        ThreatFeed::del(filename);
        ++file_index;
    }
}

std::string FortiHole::get_file_name(unsigned int security_level, unsigned int file_index) {
    return std::format("{}_{}-{}_{}-{}",
                       config.naming_convention.prefix,
                       config.naming_convention.security_level,
                       security_level,
                       config.naming_convention.file_index,
                       file_index);
}

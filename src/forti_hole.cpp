//
// Created by Cooper Larson on 8/26/24.
//

#include "include/forti_hole.h"
#include <forti_api.hpp>
#include <thread>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdlib>

FortiHole::FortiHole(const std::string& config_file) :
        config(YAML::LoadFile(config_file)) {
    {
        BlocklistScraper scraper(config);
        lists_by_security_level = scraper();
    }
    FortiAuth::set_gateway_ip(config.fortigate.gateway_ip);
    FortiAuth::set_admin_https_port(config.fortigate.admin_https_port);
    FortiAuth::set_ca_cert_path(config.fortigate.certificates.ca_cert_path);
    FortiAuth::set_ssl_cert_path(config.fortigate.certificates.ssl_cert_path);
    FortiAuth::set_api_key(config.fortigate.api_key);
    FortiAuth::set_cert_password(config.fortigate.certificates.ssl_cert_password);
}

void FortiHole::operator()() {
    auto start = std::chrono::high_resolution_clock::now();

    if (config.write_files_to_disk && !std::filesystem::exists(config.output_dir))
        std::filesystem::create_directories(config.output_dir);
    if (config.remove_all_threat_feeds_on_run) remove_all_custom_threat_feeds();

    std::cout << "Consolidating data..." << std::endl;
    merge();

    std::cout << "Gathering threat feed information..." << std::endl;
    build_threat_feed_info();

    std::cout << "Creating threat feed containers..." << std::endl;
    create_threat_feeds();

    std::cout << "Constructing files and pushing threat feeds...\n" << std::endl;
    update_threat_feeds();

    std::cout << "Updating firewall policies..." << std::endl;
    enable_filters_and_policies();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "\nforti-hole finished successfully in " << duration.count() << 's' << std::endl;
}

void FortiHole::merge() {
    for (unsigned int i = lists_by_security_level.size() - 1; i > 0; --i) {
        for (const auto& item : lists_by_security_level[i]) {
            lists_by_security_level[i - 1].insert(item);
        }
    }
}

void FortiHole::build_threat_feed_info() {
    info_by_security_level.reserve(lists_by_security_level.size());
    unsigned int category = config.categories.base;
    for (auto & lines : lists_by_security_level) {
        auto total_lines = lines.second.size();
        auto file_count = std::max((total_lines + MAX_LINES_PER_FILE - 1) / MAX_LINES_PER_FILE, static_cast<size_t>(1));
        info_by_security_level.emplace_back(total_lines, file_count, category);
        category += file_count;
    }
}

void FortiHole::create_threat_feeds() {
    for (unsigned int security_level = 0; security_level < info_by_security_level.size(); security_level++) {
        auto info = info_by_security_level[security_level];
        auto category = info.category_base;
        for (unsigned int file_index = 0; file_index < info.file_count; ++file_index) {
            auto filename = get_file_name(security_level, file_index + 1);
            if (!ThreatFeed::contains(filename)) ThreatFeed::add(filename, category);
            ++category;
        }
    }
}

void FortiHole::enable_filters_and_policies() {
    for (const auto& dns_config : config.forti_hole_automated_dns_filters) {
        if (!DNSFilter::contains(dns_config.dns_filter)) DNSFilter::add(dns_config.dns_filter);

        auto dns_filter = DNSFilter::get(dns_config.dns_filter);
        auto& firewall_policies = dns_config.firewall_policies;
        auto& filters = dns_config.filters;

        // enable dns filters for threat feed categories
        for (const auto& filter : filters) {
            auto security_level = filter.security_level;
            auto info = info_by_security_level[security_level];
            auto category = info.category_base;

            for (unsigned int file_index = 0; file_index < info.file_count; ++file_index) {
                if (filter.access == "block") dns_filter.block_category(category);
                else if (filter.access == "allow") dns_filter.allow_category(category);
                else if (filter.access == "monitor") dns_filter.monitor_category(category);
                else throw std::runtime_error("Not a valid DNSFilter setting: " + filter.access);
                ++category;
            }
        }

        DNSFilter::update(dns_filter);

        // enable the dns filter in request firewall policies
        for (const auto& policy_name : firewall_policies) {
            auto policy = FortiGate::Policy::get(policy_name);
            policy.dnsfilter_profile = dns_filter.name;
            FortiGate::Policy::update(policy);
        }
    }
}

void FortiHole::update_threat_feeds() {
    for (const auto& [security_level, _] : lists_by_security_level) {
        auto info = info_by_security_level[security_level];

        std::cout << "Security Level " << security_level
                  << ": { "
                  << "Files: " << info.file_count
                  << ", LPF: " << info.lines_per_file
                  << " }" << std::endl;

        std::vector<std::string> to_upload;
        to_upload.reserve(info.lines_per_file + 1);

        auto iter = lists_by_security_level[security_level].begin();
        for (size_t i = 0; i < info.file_count; ++i) {
            auto filename = get_file_name(security_level, i + 1);

            size_t count = info.lines_per_file + (i < info.extra ? 1 : 0);
            for (size_t j = 0; j < count; ++j) {
                if (iter == lists_by_security_level[security_level].end()) break;
                to_upload.push_back(*iter);
                ++iter;
            }

            std::cout << "\nBuilt file: " << filename << " with " << to_upload.size() << " lines." << std::endl;

            if (config.write_files_to_disk) create_file(filename, to_upload);
            ThreatFeed::update_feed({{filename, to_upload}});
            std::cout << "Successfully pushed to Fortigate: " << filename << std::endl;

            to_upload.clear();

            // give the FortiGate a chance to process the new data,
            // prevents network interruptions from buffer overflow
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        remove_extra_files(security_level, info.file_count + 1);
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
    for (const auto& connector : ThreatFeed::get()) {
        auto name = connector.name;
        std::cout << "\nDeleting: " << name << std::endl;
        ThreatFeed::del(name);
        std::cout << "Successfully deleted: " << name << std::endl;
    }
    std::cout << "Successfully removed " << size << " threat feeds..." << std::endl;
    assert(ThreatFeed::get().empty());
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

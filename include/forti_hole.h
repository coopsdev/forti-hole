//
// Created by Cooper Larson on 8/26/24.
//

#ifndef FORTI_HOLE_FORTI_HOLE_H
#define FORTI_HOLE_FORTI_HOLE_H

#include "include/config.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <regex>
#include <unordered_set>
#include <vector>

class FortiHole {

    inline static std::regex domain_regex{R"(\|\|([^\^]*?)\^)"};
    inline static std::regex valid_dns_regex{R"(^([a-zA-Z0-9-]+\.)*[a-zA-Z0-9-]+\.[a-zA-Z]{2,}$)"};

    struct Request {
        std::string url;
        unsigned int security_level;
        std::string response;
    };

    struct ThreatFeedInfo {
        size_t total_lines, file_count, lines_per_file, extra, category_base;

        ThreatFeedInfo(unsigned int total_lines, unsigned int file_count, unsigned int category_base) :
                total_lines(total_lines), file_count(file_count),
                lines_per_file(total_lines / file_count), extra(total_lines % file_count),
                category_base(category_base) {}
    };

    static constexpr unsigned int MAX_LINES_PER_FILE = 131000;

    Config config;
    std::vector<Request> requests{};
    std::vector<std::unordered_set<std::string>> lists_by_security_level{};
    std::vector<ThreatFeedInfo> info_by_security_level{};
    std::mutex mutex;

    // scraping
    void process_config();
    void fetch_multi();
    void process_multi();

    // forti-hole
    void merge();
    void build_threat_feed_info();
    void create_threat_feeds();
    void enable_filters_and_policies();
    void update_threat_feeds();

    void create_file(const std::string& filename, const std::vector<std::string>& lines) const;
    void remove_extra_files(unsigned int security_level, unsigned int file_index);

    std::string get_file_name(unsigned int security_level, unsigned int file_index);

    static void remove_all_custom_threat_feeds();

public:
    explicit FortiHole(const std::string& config_file = "config.yaml");

    void operator()();
};


#endif //FORTI_HOLE_FORTI_HOLE_H

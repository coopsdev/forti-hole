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
#include <queue>
#include <utility>
#include <future>
#include "ThreadPool.h"
#include <memory>

class FortiHole {

    struct RequestComparator {
        bool operator()(const FortiHoleRequest& a, const FortiHoleRequest& b) { return a.security_level < b.security_level; }
    };

    static constexpr unsigned int MAX_LINES_PER_FILE = 131000;

    Config config;
    std::vector<FortiHoleRequest> requests{};
    std::shared_ptr<std::vector<std::unordered_set<std::string>>> lists_by_security_level{};
    std::vector<ThreatFeedInfo> info_by_security_level{};
    std::vector<std::future<ExpectedFuture>> futures{};
    ThreadPool threadPool{};
    unsigned int total_num_files{};
    std::shared_ptr<std::vector<std::mutex>> locks{};

    // scraping
    void process_config();
    void fetch_multi();
    void process_multi();

    // forti-hole
    void build_threat_feed_info();
    void create_threat_feeds();
    void enable_filters_and_policies();
    void update_threat_feeds();
    void build_threat_feed_futures(unsigned int security_level);
    void process_threat_feed_futures(unsigned int security_level);

    void create_file(const std::string& filename, const std::vector<std::string>& lines) const;
    void remove_extra_files(unsigned int security_level, unsigned int file_index);

    std::string get_file_name(unsigned int security_level, unsigned int file_index);

    static void remove_all_custom_threat_feeds();

public:

    explicit FortiHole(const std::string& config_file = "config.yaml");

    void operator()();
};


#endif //FORTI_HOLE_FORTI_HOLE_H

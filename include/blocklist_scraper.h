//
// Created by Cooper Larson on 8/26/24.
//

#ifndef FORTI_HOLE_BLOCKLIST_SCRAPER_H
#define FORTI_HOLE_BLOCKLIST_SCRAPER_H

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <regex>
#include <yaml-cpp/yaml.h>
#include <mutex>
#include "config.h"

class BlocklistScraper {
    friend class FortiHole;

    static constexpr unsigned int MAX_LINES_PER_FILE = 131000;
    inline static std::regex domain_regex{R"(\|\|([^\^]*?)\^)"};
    inline static std::regex valid_dns_regex{R"(^([a-zA-Z0-9-]+\.)*[a-zA-Z0-9-]+\.[a-zA-Z]{2,}$)"};

    struct Request {
        std::string url;
        unsigned int security_level;
        std::string response;
    };

    Config config;
    std::vector<Request> requests{};
    std::unordered_map<unsigned int, std::unordered_set<std::string>> lists_by_security_level{};
    std::vector<std::vector<std::vector<std::string>>> output{};
    std::mutex mutex;

    void process_config();
    void fetch_multi();
    void process_multi();
    void process_domains(const std::string& content, std::unordered_set<std::string>& target_set);
    void merge();
    void build();
    void construct(unsigned int security_level);

public:
    explicit BlocklistScraper(const std::string& config_file = "../config.yaml");

    std::vector<std::vector<std::vector<std::string>>> operator()();
};

#endif // FORTI_HOLE_BLOCKLIST_SCRAPER_H

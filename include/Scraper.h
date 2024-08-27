//
// Created by Cooper Larson on 8/26/24.
//

#ifndef FORTI_HOLE_SCRAPER_H
#define FORTI_HOLE_SCRAPER_H

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <regex>
#include <yaml-cpp/yaml.h>
#include <mutex>

class Scraper {
    friend class FortiThreatFeedManager;

    static constexpr unsigned int MAX_LINES_PER_FILE = 131000;
    inline static std::regex domain_regex{R"(\|\|([^\^]*?)\^)"};
    inline static std::regex valid_dns_regex{R"(^([a-zA-Z0-9-]+\.)*[a-zA-Z0-9-]+\.[a-zA-Z]{2,}$)"};

    struct Request {
        std::string url;
        int security_level;
        std::string response;
    };

    YAML::Node config;
    std::string output_dir;
    std::vector<Request> requests{};
    std::unordered_map<int, std::unordered_set<std::string>> lists_by_security_level{};
    std::vector<std::vector<std::vector<std::string>>> output{};
    std::mutex mutex;

    void process_config();
    void fetch_multi();
    void process_multi();
    void process_domains(const std::string& content, std::unordered_set<std::string>& target_set);
    void merge();
    void build();
    void construct(int security_level);

public:
    explicit Scraper(const std::string& config_file = "../config.yaml");

    std::vector<std::vector<std::vector<std::string>>> operator()();
};

#endif // FORTI_HOLE_SCRAPER_H

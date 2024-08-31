//
// Created by Cooper Larson on 8/30/24.
//

#ifndef FORTI_HOLE_CONFIG_H
#define FORTI_HOLE_CONFIG_H

#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <string>

inline static nlohmann::json yamlToJson(const YAML::Node& node) {
    nlohmann::json json;

    if (node.IsScalar()) {
        try { json = node.as<int>(); }  // try to parse as int first
        catch (const YAML::BadConversion&) {
            try { json = node.as<double>(); }  // not int, try as double
            catch (const YAML::BadConversion&) {
                try { json = node.as<bool>(); }  // try as bool
                catch (const YAML::BadConversion&) { json = node.as<std::string>(); } // fallback to string
            }
        }
    }
    else if (node.IsSequence()) for (const auto & i : node) json.push_back(yamlToJson(i));
    else if (node.IsMap())
        for (const auto& pair : node) json[pair.first.as<std::string>()] = yamlToJson(pair.second);

    return json;
}

struct Policy {
    std::string name, access;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Policy, name, access);
};

struct FortiHoleConfig {
    std::string name;
    unsigned int security_level{};
    std::vector<Policy> policies;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(FortiHoleConfig, name, security_level, policies);
};

struct Source {
    std::string name;
    unsigned int security_level{};

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Source, name, security_level);
};

struct Blocklist {
    std::string name_prefix, url, postfix;
    std::vector<Source> sources;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Blocklist, name_prefix, url, postfix, sources);
};

struct Categories {
    unsigned int min{}, max{}, base{};

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Categories, min, max, base);
};

struct Config {
    std::string output_dir;
    bool write_files_to_disk{}, remove_all_threat_feeds_on_run{};
    Categories categories;
    std::vector<FortiHoleConfig> forti_hole_automated_dns_filters;
    std::vector<Blocklist> blocklist_sources;

    Config() = default;
    explicit Config(const YAML::Node& node) { *this = yamlToJson(node); }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Config, output_dir, write_files_to_disk,
                                                remove_all_threat_feeds_on_run, categories,
                                                forti_hole_automated_dns_filters, blocklist_sources);
};


#endif //FORTI_HOLE_CONFIG_H

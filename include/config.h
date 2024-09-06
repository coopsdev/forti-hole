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
        try { json = node.as<unsigned int>(); }  // try to parse as unsigned int first
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

struct FilteringPolicy {
    unsigned int security_level{};
    std::string access;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(FilteringPolicy, security_level, access);
};

struct FortiHoleConfig {
    std::string dns_filter, access;
    std::vector<std::string> firewall_policies;
    std::vector<FilteringPolicy> filters;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(FortiHoleConfig, dns_filter, firewall_policies, filters);
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

struct NamingConvention {
    std::string prefix, security_level, file_index;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(NamingConvention, prefix, security_level, file_index)
};

struct Certificates {
    std::string ca_cert_path, ssl_cert_path, ssl_cert_password;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Certificates, ca_cert_path, ssl_cert_path, ssl_cert_password)
};

struct FortiGateConfig {
    std::string api_key, gateway_ip;
    unsigned int admin_https_port{};
    Certificates certificates;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(FortiGateConfig, api_key, gateway_ip, admin_https_port,
                                                certificates)
};

struct Admin {
    bool enable_admin_access_control = false;
    std::string api_admin;
    std::vector<std::string> sources;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Admin, enable_admin_access_control, api_admin, sources)
};

struct Config {
    Admin admin;
    FortiGateConfig fortigate;
    std::string output_dir;
    NamingConvention naming_convention;
    bool write_files_to_disk{}, remove_all_threat_feeds_on_run{};
    Categories categories;
    std::vector<FortiHoleConfig> forti_hole_automated_dns_filters;
    std::vector<Blocklist> blocklist_sources;

    Config() = default;
    explicit Config(const YAML::Node& node) { *this = yamlToJson(node); }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Config, fortigate, output_dir, naming_convention, write_files_to_disk,
                                                remove_all_threat_feeds_on_run, admin, categories,
                                                forti_hole_automated_dns_filters, blocklist_sources)
};


#endif //FORTI_HOLE_CONFIG_H

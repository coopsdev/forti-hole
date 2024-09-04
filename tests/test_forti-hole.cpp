//
// Created by Cooper Larson on 8/30/24.
//

#include "include/forti_hole.h"
#include <gtest/gtest.h>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

#define ALLOW_RISKY_TESTS_ON_LOCAL_DEVICE true

#if ALLOW_RISKY_TESTS_ON_LOCAL_DEVICE

TEST(TestFortiHole, TestPopulateThreatFeed) {
    try {
        FortiHole fortiHole("../config.yaml");
        fortiHole();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        FAIL();
    }
}

inline static nlohmann::json test_yaml_to_json(const YAML::Node& node, const std::string& parent_key = "") {
    nlohmann::json json;

    if (node.IsScalar()) {
        std::string scalar_value = node.as<std::string>();
        std::cout << "[LOG] Processing key: '" << parent_key << "', Value: '" << scalar_value << "' (Scalar)\n";

        try { json = node.as<unsigned int>(); }  // try to parse as unsigned int first
        catch (const YAML::BadConversion&) {
            try { json = node.as<double>(); }  // not int, try as double
            catch (const YAML::BadConversion&) {
                try { json = node.as<bool>(); }  // try as bool
                catch (const YAML::BadConversion&) { json = scalar_value; } // fallback to string
            }
        }
    }
    else if (node.IsSequence()) {
        std::cout << "[LOG] Processing key: '" << parent_key << "' (Sequence)\n";
        for (const auto & i : node) {
            json.push_back(test_yaml_to_json(i, parent_key));
        }
    }
    else if (node.IsMap()) {
        std::cout << "[LOG] Processing key: '" << parent_key << "' (Map)\n";
        for (const auto& pair : node) {
            std::string key = pair.first.as<std::string>();
            json[key] = test_yaml_to_json(pair.second, key);
        }
    }

    return json;
}


TEST(TestFortiHole, TestLoadConfig) {
    try {
        // Load the YAML configuration file
        std::string config_file_path = "../pi-config.yaml";
        YAML::Node config = YAML::LoadFile(config_file_path);

        // Convert the YAML to JSON and log the process
        std::cout << "[LOG] Starting YAML to JSON conversion...\n";
        nlohmann::json json_config = test_yaml_to_json(config);
        std::cout << "[LOG] Completed YAML to JSON conversion.\n";

        // Print final JSON result for inspection
        std::cout << "[LOG] Final JSON output:\n" << json_config.dump(4) << std::endl;

        Config conf(config);

        std::cout << conf.fortigate.certificates.ssl_cert_path << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during YAML to JSON conversion: " << e.what() << std::endl;
        FAIL();  // Fail the test if an exception is thrown
    }
}


#endif

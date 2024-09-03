#include <iostream>
#include <cstdlib>
#include "include/forti_hole.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "Starting forti-hole..." << std::endl;

    if (!std::filesystem::exists("./config.yaml")) {
        std::cerr << "Error: Configuration file config.yaml not found." << std::endl;
        return 1;
    }

    Config config(YAML::Load("./config.yaml"));

    if (config.fortigate.gateway_ip.empty() || config.fortigate.ca_cert_path.empty() ||
        config.fortigate.ssl_cert_path.empty() || config.fortigate.admin_https_port == 0) {
        std::cerr << "Error: One or more configuration fields are empty." << std::endl;
        return 1;
    }

    setenv("FORTIGATE_GATEWAY_IP", config.fortigate.gateway_ip.c_str(), 1);
    setenv("FORTIGATE_ADMIN_HTTPS_PORT", std::to_string(config.fortigate.admin_https_port).c_str(), 1);
    setenv("PATH_TO_FORTIGATE_CA_CERT", config.fortigate.ca_cert_path.c_str(), 1);
    setenv("PATH_TO_FORTIGATE_SSL_CERT", config.fortigate.ssl_cert_path.c_str(), 1);

    std::cout << "FORTIGATE_GATEWAY_IP set to: " << getenv("FORTIGATE_GATEWAY_IP") << std::endl;
    std::cout << "PATH_TO_FORTIGATE_CA_CERT set to: " << getenv("PATH_TO_FORTIGATE_CA_CERT") << std::endl;

    FortiHole fortiHole;
    fortiHole();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "forti-hole finished in " << duration.count() << "ms" << std::endl;

    return 0;
}

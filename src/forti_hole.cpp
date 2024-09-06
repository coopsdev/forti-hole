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
#include <future>
#include <memory>

inline static std::regex ipv4_subnet(
        R"(((([0-9]{1,3})\.){3}([0-9]{1,3}))\/([0-9]|[1-2][0-9]|3[0-2]))"); // Subnet CIDR for IPv4 (0-32)
inline static std::regex ipv6_subnet(
        R"((([0-9a-fA-F]{1,4}\:){7}[0-9a-fA-F]{1,4})\/([0-9]|[1-9][0-9]|1[0-1][0-9]|12[0-8]))"); // Subnet CIDR for IPv6 (0-128)

FortiHole::FortiHole(const std::string& config_file) : config(YAML::LoadFile(config_file)) {
    FortiAuth::set_gateway_ip(config.fortigate.gateway_ip);
    FortiAuth::set_admin_https_port(config.fortigate.admin_https_port);
    FortiAuth::set_ca_cert_path(config.fortigate.certificates.ca_cert_path);
    FortiAuth::set_ssl_cert_path(config.fortigate.certificates.ssl_cert_path);
    FortiAuth::set_api_key(config.fortigate.api_key);
    FortiAuth::set_cert_password(config.fortigate.certificates.ssl_cert_password);
}

void FortiHole::operator()() {
    auto start = std::chrono::high_resolution_clock::now();

    if (config.admin.enable_admin_access_control) {
        std::cout << "Scanning admin access policy sources..." << std::endl;
        allow_admin_sources();
    }

    std::cout << "Starting blocklist scraping process...\n" << std::endl;
    process_config();

    std::cout << "Scraping blocklists...\n" << std::endl;
    fetch_multi();

    std::cout << "Parsing response data...\n" << std::endl;
    process_multi();

    if (config.write_files_to_disk && !std::filesystem::exists(config.output_dir)) {
        std::cout << "Creating output directory: " << config.output_dir << std::endl;
        std::filesystem::create_directories(config.output_dir);
    }

    if (config.remove_all_threat_feeds_on_run) {
        std::cout << "Removing old threat feeds...\n" << std::endl;
        remove_all_custom_threat_feeds();
    }

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

size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* data = static_cast<std::string*>(userdata);
    data->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

void FortiHole::allow_admin_sources() {
    if (!config.admin.enable_admin_access_control) return;

    auto admin = System::Admin::API::get(config.admin.api_admin);

    for (const auto& source : config.admin.sources) {

        // Check if the source is a valid IPv4 or IPv6 subnet
        if (std::regex_match(source, ipv4_subnet) || std::regex_match(source, ipv6_subnet)) {
            admin.trust(source);
            std::cout << "Trusted source: " << source << std::endl;
            continue;
        }

        // If it's not a subnet, fetch the data from URL
        auto data = fetch(source);
        if (!data.empty()) {
            std::istringstream stream(data);
            std::string line;

            while (std::getline(stream, line)) {
                line = trim(line);

                // Check if the line is a valid IPv4 or IPv6 subnet
                bool is_ipv4 = std::regex_match(line, ipv4_subnet);
                if (is_ipv4 || std::regex_match(line, ipv6_subnet)) {
                    admin.trust(line);
                    std::string type = is_ipv4 ? "IPv4 subnet" : "IPv6 subnet";
                    std::cout << "Allowed " << type << ": " << line << " for admin access." << std::endl;
                } else {
                    std::cerr << "Invalid entry in file (not a valid subnet): " << line << std::endl;
                }
            }
        }
    }

    admin.update();
}

void FortiHole::process_config() {
    std::cout << "Processing config file...\n" << std::endl;
    unsigned int max_security = 0;
    for (const auto& entry : config.blocklist_sources) {
        for (const auto& src : entry.sources) {
            requests.push_back({entry.url + "/" + src.name + entry.postfix + ".txt", src.security_level, ""});
            if (src.security_level > max_security) max_security = src.security_level;
        }
    }

    // sort the requests to process lowest-security first
    std::sort(requests.begin(), requests.end(), RequestComparator());

    // instantiate the main lists with the number of security level + 1
    lists_by_security_level = std::vector<std::unordered_set<std::string>>(max_security + 1);
}

std::string FortiHole::fetch(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string read_buffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects if necessary

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Error: Failed to download file from " << url << " - " << curl_easy_strerror(res) << std::endl;
            read_buffer.clear();  // Clear the buffer on failure
        }

        curl_easy_cleanup(curl);
    } else std::cerr << "Error: Failed to initialize CURL for URL: " << url << std::endl;

    return read_buffer;
}

void FortiHole::fetch_multi() {
    curl_global_init(CURL_GLOBAL_ALL);

    auto multi_handle = std::unique_ptr<CURLM, decltype(&curl_multi_cleanup)>(curl_multi_init(), curl_multi_cleanup);
    if (!multi_handle) {
        std::cerr << "Failed to initialize CURLM handle" << std::endl;
        curl_global_cleanup();
        return;
    }

    auto easy_handle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>(curl_easy_init(), curl_easy_cleanup);
    if (!easy_handle) {
        std::cerr << "curl_easy_init() failed" << std::endl;
        curl_global_cleanup();
        return;
    }

    for (auto& request : requests) {
        std::cout << "Fetching URL: " << request.url << std::endl;

        curl_easy_reset(easy_handle.get());
        curl_easy_setopt(easy_handle.get(), CURLOPT_URL, request.url.c_str());
        curl_easy_setopt(easy_handle.get(), CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(easy_handle.get(), CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(easy_handle.get(), CURLOPT_WRITEDATA, &request.response);

        CURLMcode mc = curl_multi_add_handle(multi_handle.get(), easy_handle.get());
        if (mc != CURLM_OK) {
            std::cerr << "curl_multi_add_handle() failed: " << curl_multi_strerror(mc) << std::endl;
            continue;
        }

        int still_running = 0;
        do {
            mc = curl_multi_perform(multi_handle.get(), &still_running);
            if (mc != CURLM_OK) {
                std::cerr << "curl_multi_perform() failed: " << curl_multi_strerror(mc) << std::endl;
                break;
            }

            if (still_running) {
                mc = curl_multi_poll(multi_handle.get(), nullptr, 0, 1000, nullptr);
                if (mc != CURLM_OK) {
                    std::cerr << "curl_multi_poll() failed: " << curl_multi_strerror(mc) << std::endl;
                    break;
                }
            }
        } while (still_running);

        curl_multi_remove_handle(multi_handle.get(), easy_handle.get());
    }

    curl_global_cleanup();
}

void FortiHole::process_multi() {
    for (auto& request : requests) {
        auto content = request.response;
        const size_t length = content.length();
        const size_t num_threads = std::thread::hardware_concurrency();
        const size_t chunk_size = length / num_threads;

        std::vector<std::future<void>> futures;

        for (size_t i = 0; i < num_threads; ++i) {
            size_t start = i * chunk_size;
            size_t end = (i + 1 == num_threads) ? length : (i + 1) * chunk_size;

            if (end != length && end > 0) while (end < length && content[end] != ' ' && content[end] != '\n') ++end;

            std::string chunk = content.substr(start, end - start);

            futures.push_back(std::async(std::launch::async, [this, &request, chunk]() {
                std::smatch matches;
                std::string::const_iterator searchStart(chunk.cbegin());
                while (std::regex_search(searchStart, chunk.cend(), matches, domain_regex)) {
                    std::string domain = matches[1].str();

                    unsigned int lower_security_levels = request.security_level;
                    bool match_found = false;
                    while (lower_security_levels > 0) {
                        if (lists_by_security_level[--lower_security_levels].contains(domain)) {
                            match_found = true;
                            break;
                        }
                    }

                    searchStart = matches.suffix().first;  // leave this where it is

                    if (match_found) continue;
                    else if (std::regex_match(domain, valid_dns_regex)) {
                        std::scoped_lock lock(mutex);
                        lists_by_security_level[request.security_level].insert(domain);
                    }
                }
            }));
        }

        for (auto& future : futures) future.get();

        std::cout << "Finished: " << request.url << std::endl;
    }
    std::cout << "\nBlocklist processing successfully completed...\n" << std::endl;
}

void FortiHole::build_threat_feed_info() {
    info_by_security_level.reserve(lists_by_security_level.size());
    unsigned int category = config.categories.base;
    for (auto & lines : lists_by_security_level) {
        auto total_lines = lines.size();
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
    for (unsigned int security_level = 0; security_level < info_by_security_level.size(); ++security_level) {
        auto info = info_by_security_level[security_level];

        std::cout << "Security Level " << security_level
                  << ": { "
                  << "Files: " << info.file_count
                  << ", LPF: " << info.lines_per_file
                  << " }" << std::endl;

        std::vector<std::string> to_upload;
        to_upload.reserve(info.lines_per_file + 1);

        auto iter = lists_by_security_level[security_level].begin();
        std::cout << "Security Level " << security_level << ", size: " << lists_by_security_level[security_level].size() << std::endl;
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

std::string FortiHole::trim(const std::string& str) {
    const auto strBegin = str.find_first_not_of(" \t");
    if (strBegin == std::string::npos)
        return "";  // no content

    const auto strEnd = str.find_last_not_of(" \t");
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

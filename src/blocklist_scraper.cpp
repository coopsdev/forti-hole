//
// Created by Cooper Larson on 8/26/24.
//

#include "include/blocklist_scraper.h"
#include <iostream>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <curl/curl.h>
#include <filesystem>
#include <memory>
#include <future>

BlocklistScraper::BlocklistScraper(const std::string& config_file) : config(YAML::LoadFile(config_file)) {}

std::vector<std::vector<std::vector<std::string>>> BlocklistScraper::operator()() {
    std::cout << "Starting ThreatFeed construction..." << std::endl;
    process_config();
    fetch_multi();
    process_multi();
    merge();
    build();
    std::cout << "Successfully completed ThreatFeed construction!" << std::endl;
    return output;
}

void BlocklistScraper::process_config() {
    std::cout << "Processing config file..." << std::endl;
    for (const auto& entry : config.blocklist_sources) {
        for (const auto& src : entry.sources) {
            requests.push_back({entry.url + "/" + src.name + entry.postfix + ".txt",
                                src.security_level, ""});
        }
    }
}

size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* data = static_cast<std::string*>(userdata);
    data->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

void BlocklistScraper::fetch_multi() {
    std::cout << "Scraping blocklists..." << std::endl;

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

void BlocklistScraper::process_multi() {
    std::cout << "Parsing response data..." << std::endl;
    for (auto& request : requests) {
        std::cout << "Parsing response data: " << request.url << std::endl;
        process_domains(request.response, lists_by_security_level[request.security_level]);
    }
}

void BlocklistScraper::process_domains(const std::string& content, std::unordered_set<std::string>& target_set) {
    const size_t length = content.length();
    const size_t num_threads = std::thread::hardware_concurrency();
    const size_t chunk_size = length / num_threads;

    std::vector<std::future<void>> futures;

    for (size_t i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i + 1 == num_threads) ? length : (i + 1) * chunk_size;

        if (end != length && end > 0) while (end < length && content[end] != ' ' && content[end] != '\n') ++end;

        std::string chunk = content.substr(start, end - start);

        futures.push_back(std::async(std::launch::async, [this, &target_set, chunk]() {
            std::smatch matches;
            std::string::const_iterator searchStart(chunk.cbegin());
            while (std::regex_search(searchStart, chunk.cend(), matches, domain_regex)) {
                std::string domain = matches[1].str();
                if (std::regex_match(domain, valid_dns_regex)) {
                    std::scoped_lock lock(mutex);
                    target_set.insert(domain);
                }
                searchStart = matches.suffix().first;
            }
        }));
    }

    for (auto& future : futures) future.get();
}

void BlocklistScraper::merge() {
    std::cout << "Consolidating data..." << std::endl;
    for (unsigned int i = lists_by_security_level.size() - 1; i > 0; --i) {
        for (const auto& item : lists_by_security_level[i]) {
            lists_by_security_level[i - 1].insert(item);
        }
    }
}

void BlocklistScraper::build() {
    std::cout << "Building ThreatFeeds..." << std::endl;
    if (!std::filesystem::exists(config.output_dir)) {
        std::filesystem::create_directories(config.output_dir);
    }

    output.resize(lists_by_security_level.size());

    for (const auto& [security_level, _] : lists_by_security_level) {
        construct(security_level);
    }
}

void BlocklistScraper::construct(unsigned int security_level) {
    std::cout << "Resizing files to fit on FortiGate..." << std::endl;

    auto& lines = lists_by_security_level[security_level];
    size_t total_lines = lines.size();
    size_t file_count = std::max((total_lines + MAX_LINES_PER_FILE - 1) / MAX_LINES_PER_FILE, static_cast<size_t>(1));
    size_t lines_per_file = total_lines / file_count;
    size_t extra = total_lines % file_count;

    std::cout << "Security Level " << security_level << " Files: " << file_count << ", LPF: " << lines_per_file << std::endl;

    output[security_level].resize(file_count);

    auto iter = lines.begin();
    for (size_t i = 0; i < file_count; ++i) {
        output[security_level][i].reserve(lines_per_file + extra);

        std::string filename = std::filesystem::current_path().string()
                + '/' + config.output_dir
                + "/security_level_" + std::to_string(security_level)
                + "_part_" + std::to_string(i + 1) + ".txt";

        std::ofstream outfile(filename);
        if (!outfile) {
            std::cerr << "Failed to create file: " << filename << std::endl;
            return;
        }

        size_t count = lines_per_file + (i < extra ? 1 : 0);
        for (size_t j = 0; j < count; ++j) {
            outfile << *iter << "\n";
            output[security_level][i].push_back(*iter);
            ++iter;
        }

        outfile.close();
        std::cout << "Created file: " << filename << " with " << count << " lines." << std::endl;
    }
}

//
// Created by Cooper Larson on 8/26/24.
//

#include "include/Scraper.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <yaml-cpp/yaml.h>
#include <curl/curl.h>
#include <filesystem>
#include <memory>
#include <future>

Scraper::Scraper(const std::string& config_file) : config(YAML::LoadFile(config_file)),
                                                   output_dir(config["output_dir"].as<std::string>()) {}

std::vector<std::vector<std::vector<std::string>>> Scraper::operator()() {
    std::cout << "Starting ThreatFeed construction..." << std::endl;
    process_config();
    fetch_multi();
    process_multi();
    merge();
    build();
    std::cout << "Successfully completed ThreatFeed construction!" << std::endl;
    return output;
}

void Scraper::process_config() {
    std::cout << "Processing config file..." << std::endl;
    for (const auto& entry : config["blocklist_sources"]) {
        for (const auto& project : entry) {
            const auto& source = project.second;

            if (!source["name_prefix"] || !source["url"] || !source["postfix"] || !source["sources"]) {
                throw std::runtime_error("Missing expected key in source configuration for project: " + project.first.as<std::string>());
            }

            auto name_prefix = source["name_prefix"].as<std::string>();
            auto url = source["url"].as<std::string>();
            auto postfix = source["postfix"].as<std::string>();

            for (const auto& src : source["sources"]) {
                if (!src["name"] || !src["security_level"]) {
                    throw std::runtime_error("Missing expected key in source entry for project: " + project.first.as<std::string>());
                }

                requests.push_back({
                                           url + "/" + src["name"].as<std::string>() + postfix + ".txt",
                                           src["security_level"].as<int>(),
                                           ""
                                   });
            }
        }
    }
}

size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* data = static_cast<std::string*>(userdata);
    data->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

void Scraper::fetch_multi() {
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
                mc = curl_multi_poll(multi_handle.get(), NULL, 0, 1000, NULL);
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

void Scraper::process_multi() {
    std::cout << "Parsing response data..." << std::endl;
    for (auto& request : requests) {
        std::cout << "Parsing response data: " << request.url << std::endl;
        process_domains(request.response, lists_by_security_level[request.security_level]);
    }
}

void Scraper::process_domains(const std::string& content, std::unordered_set<std::string>& target_set) {
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

void Scraper::merge() {
    std::cout << "Consolidating data..." << std::endl;
    for (unsigned int i = lists_by_security_level.size() - 1; i > 0; --i) {
        for (const auto& item : lists_by_security_level[i]) {
            lists_by_security_level[i - 1].insert(item);
        }
    }
}

void Scraper::build() {
    std::cout << "Building ThreatFeeds..." << std::endl;
    if (!std::filesystem::exists(output_dir)) {
        std::filesystem::create_directories(output_dir);
    }

    output.reserve(lists_by_security_level.size());

    for (const auto& [security_level, _] : lists_by_security_level) {
        construct(security_level);
    }
}

void Scraper::construct(int security_level) {
    std::cout << "Resizing files to fit on FortiGate..." << std::endl;

    auto& lines = lists_by_security_level[security_level];
    size_t total_lines = lines.size();
    size_t file_count = std::max((total_lines + MAX_LINES_PER_FILE - 1) / MAX_LINES_PER_FILE, static_cast<size_t>(1));
    size_t lines_per_file = total_lines / file_count;
    size_t extra = total_lines % file_count;

    std::cout << "Security Level " << security_level << " Files: " << file_count << ", LPF: " << lines_per_file << std::endl;

    output[security_level].reserve(file_count);

    auto iter = lines.begin();
    for (size_t i = 0; i < file_count; ++i) {
        output[security_level][i].reserve(lines_per_file + extra);

        std::string filename = std::filesystem::current_path().string()
                + '/' + output_dir
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

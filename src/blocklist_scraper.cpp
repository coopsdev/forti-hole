//
// Created by Cooper Larson on 8/26/24.
//

#include "include/blocklist_scraper.h"
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <curl/curl.h>
#include <memory>
#include <future>

BlocklistScraper::BlocklistScraper(const Config& config) : config(config) {}

std::unordered_map<unsigned int, std::unordered_set<std::string>> BlocklistScraper::operator()() {
    std::cout << "Starting blocklist scraping process...\n" << std::endl;
    process_config();
    fetch_multi();
    std::cout << std::endl;
    process_multi();
    return lists_by_security_level;
}

void BlocklistScraper::process_config() {
    std::cout << "Processing config file...\n" << std::endl;
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
    std::cout << "Scraping blocklists...\n" << std::endl;

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
    std::cout << "Parsing response data...\n" << std::endl;
    for (auto& request : requests) {
        std::cout << "Parsing response data: " << request.url << std::endl;
        process_domains(request.response, lists_by_security_level[request.security_level]);
    }
    std::cout << "\nBlocklist processing successfully completed...\n" << std::endl;
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

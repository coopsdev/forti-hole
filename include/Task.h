//
// Created by Cooper Larson on 6/1/24.
//

#ifndef TASK_H
#define TASK_H

#include <utility>
#include <variant>
#include <vector>
#include <string>
#include <mutex>
#include <unordered_set>
#include <regex>

using ExpectedFuture = std::variant<bool, std::pair<std::string, std::vector<std::string>>>;

inline static const std::regex domain_regex{R"(\|\|([^\^]*?)\^)"};
inline static const std::regex valid_dns_regex{R"(^([a-zA-Z0-9-]+\.)*[a-zA-Z0-9-]+\.[a-zA-Z]{2,}$)"};

struct FortiHoleRequest {
    std::string url;
    unsigned int security_level;
    std::string response;
};

struct ThreatFeedInfo {
    size_t total_lines, file_count, lines_per_file, extra, category_base;

    ThreatFeedInfo(unsigned int total_lines, unsigned int file_count, unsigned int category_base) :
            total_lines(total_lines), file_count(file_count),
            lines_per_file(total_lines / file_count), extra(total_lines % file_count),
            category_base(category_base) {}
};

struct Task {
    virtual ~Task() = default;
    virtual ExpectedFuture operator()() = 0;
};

struct ResponseParser : public Task {
    std::shared_ptr<std::vector<std::unordered_set<std::string>>> lists_by_security_level;
    FortiHoleRequest request;
    unsigned int start, end;
    std::shared_ptr<std::vector<std::mutex>> locks;

    ResponseParser(const std::shared_ptr<std::vector<std::unordered_set<std::string>>>& lists,
                   FortiHoleRequest request,
                   unsigned int start,
                   unsigned int end,
                   const std::shared_ptr<std::vector<std::mutex>>& locks) :
                   lists_by_security_level(lists),
                   request(std::move(request)),
                   start(start),
                   end(end),
                   locks(locks) {}

    ExpectedFuture operator()() override {
        auto& content = request.response;
        auto start_it = content.cbegin() + start;
        auto end_it = content.cbegin() + end;

        std::smatch matches;
        std::string::const_iterator searchStart(start_it);
        while (std::regex_search(searchStart, end_it, matches, domain_regex)) {
            std::string domain = matches[1].str();

            unsigned int lower_security_levels = request.security_level;
            bool match_found = false;
            while (lower_security_levels > 0) {
                if (lists_by_security_level->at(--lower_security_levels).contains(domain)) {
                    match_found = true;
                    break;
                }
            }

            searchStart = matches.suffix().first;  // Move the searchStart iterator

            if (match_found) continue;
            else if (std::regex_match(domain, valid_dns_regex)) {
                {
                    std::scoped_lock lock(locks->at(request.security_level));
                    lists_by_security_level->at(request.security_level).insert(domain);
                }
            }
        }

        return true;  // dummy response for std::variant
    }
};

struct ThreatFeedBuilder : public Task {
    std::shared_ptr<std::vector<std::unordered_set<std::string>>> lists_by_security_level;
    std::string filename;
    ThreatFeedInfo info;
    unsigned int security_level, file_index;
    std::vector<std::string> to_upload;

    ThreatFeedBuilder(const std::shared_ptr<std::vector<std::unordered_set<std::string>>>& lists,
                      const std::string& filename,
                      const ThreatFeedInfo& info,
                      unsigned int security_level,
                      unsigned int file_index) :
            lists_by_security_level(lists),
            filename(filename),
            info(info),
            security_level(security_level),
            file_index(file_index),
            to_upload() { to_upload.reserve(info.lines_per_file + 1); }

    ExpectedFuture operator()() override {
        auto iter = lists_by_security_level->at(security_level).begin();
        auto end = lists_by_security_level->at(security_level).end();

        size_t count = info.lines_per_file + (file_index < info.extra ? 1 : 0);
        for (size_t j = 0; j < count; ++j) {
            if (iter == end) break;
            to_upload.push_back(*iter);
            ++iter;
        }

        return std::make_pair(filename, to_upload);
    }
};

#endif //TASK_H

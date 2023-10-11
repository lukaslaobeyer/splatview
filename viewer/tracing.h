#pragma once

#include "logging.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <optional>
#include <source_location>
#include <string>
#include <thread>
#include <unordered_map>

// Tracing utility producing json files in the Chrome tracing format.
// Tracing format spec:
// https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview

namespace tracing {

struct TraceEntry {
    TraceEntry(const std::string& name_,
               int64_t start_,
               int64_t end_,
               std::thread::id thread_id_)
        : name(name_), start(start_), end(end_), thread_id(thread_id_) {}

    std::string name;
    int64_t start;
    int64_t end;
    std::thread::id thread_id;
};

class Tracing {
    struct Session {
        Session(const std::string& filepath_) : filepath(filepath_) {}
        std::string filepath;
    };

private:
    Tracing() : session_(std::nullopt), count_(0) {}

public:
    ~Tracing() { end(); }

    static Tracing& get() {
        static Tracing instance;
        return instance;
    }

    void begin(const std::string& filepath = "results.json") {
        if (session_.has_value()) LOG_FATAL("tracing already begun");

        LOG_INFO("begin tracing to %s", filepath.c_str());
        out_.open(filepath);
        write_header();
        session_.emplace(filepath);

        thread_ids_.clear();
        thread_ids_[std::this_thread::get_id()] = 0;
    }

    void end() {
        if (!session_.has_value()) return;

        for (const auto& entry : trace_) write_profile(entry);

        write_footer();
        out_.close();

        LOG_INFO(
            "wrote %lu profile entries to %s", count_, session_->filepath.c_str());

        session_.reset();
        count_ = 0;
    }

    template <typename... Args>
    void record(Args&&... args) {
        std::lock_guard<std::mutex> guard(mutex_);
        if (!session_.has_value()) return;
        trace_.emplace_back(std::forward<Args>(args)...);
    }

private:
    void write_profile(const TraceEntry& result) {
        if (count_++ > 0) out_ << ",";

        // For some reason, perfetto does not show threads as separate
        // when using the raw thread id
        if (thread_ids_.count(result.thread_id) == 0)
            thread_ids_[result.thread_id] = thread_ids_.size();

        std::string name = result.name;
        std::replace(name.begin(), name.end(), '"', '\'');

        out_ << "{";
        out_ << "\"cat\":\"function\",";
        out_ << "\"dur\":" << (result.end - result.start) << ',';
        out_ << "\"name\":\"" << name << "\",";
        out_ << "\"ph\":\"X\",";
        out_ << "\"pid\":0,";
        out_ << "\"tid\":" << thread_ids_.at(result.thread_id) << ",";
        out_ << "\"ts\":" << result.start;
        out_ << "}";
    }

    void write_header() {
        out_ << "{\"otherData\": {},\"traceEvents\":[";
        out_.flush();
    }

    void write_footer() {
        out_ << "]}";
        out_.flush();
    }

private:
    std::optional<Session> session_;
    std::vector<TraceEntry> trace_;
    std::ofstream out_;
    size_t count_;
    std::unordered_map<std::thread::id, size_t> thread_ids_;
    std::mutex mutex_;
};

inline void begin(const std::string& filename = "results.json") {
    Tracing::get().begin(filename);
}

namespace {

std::string format_src_loc(const std::source_location& src_loc) {
    std::stringstream ss;
    ss << src_loc.file_name() << "("
       << src_loc.line() << "): `"
       << src_loc.function_name() << "`";
    return ss.str();
}
    
}
    
class RecorderGuard {
public:
    RecorderGuard(const std::string& name)
        : name_(name),
          stopped_(false),
          start_(std::chrono::high_resolution_clock::now()),
          print_on_stop_(false) {}

    RecorderGuard(const std::source_location src_loc = std::source_location::current())
        : RecorderGuard(format_src_loc(src_loc)) {}

    ~RecorderGuard() {
        if (!stopped_) stop();
    }

    void print() {
        print_on_stop_ = true;
    }
    
    void stop() {
        const auto end = std::chrono::high_resolution_clock::now();

        const int64_t start_micros =
            std::chrono::time_point_cast<std::chrono::microseconds>(start_)
            .time_since_epoch()
            .count();
        const int64_t end_micros =
            std::chrono::time_point_cast<std::chrono::microseconds>(end)
            .time_since_epoch()
            .count();

        Tracing::get().record(
            name_, start_micros, end_micros, std::this_thread::get_id());

        stopped_ = true;

        if (print_on_stop_)
            LOG_INFO("%s took %.2f ms",
                     name_.c_str(),
                     1e-3 * static_cast<double>(end_micros - start_micros));
    }

private:
    const std::string name_;
    bool stopped_;
    const std::chrono::time_point<std::chrono::high_resolution_clock> start_;
    bool print_on_stop_;
};

}  // namespace tracing

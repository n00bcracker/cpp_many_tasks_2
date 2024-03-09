#pragma once

#include <string>
#include <filesystem>

#ifdef __linux__
#include <sys/time.h>
#include <sys/resource.h>
#endif

inline std::filesystem::path GetFileDir(std::string file) {
    std::filesystem::path path{std::move(file)};
    if (path.is_absolute()) {
        return path.parent_path();
    } else {
        throw std::runtime_error{"Bad file name"};
    }
}

class CPUTimer {
    using Clock = std::chrono::steady_clock;
    struct Times {
        Clock::duration wall_time;
        std::chrono::microseconds cpu_time;
    };

public:
    enum class Type { THREAD = RUSAGE_THREAD, PROCESS = RUSAGE_SELF };

    using enum Type;

    explicit CPUTimer(Type type = Type::PROCESS) : type_{type} {
    }

    Times GetTimes() const {
        return {Clock::now() - wall_start_, GetCPUTime() - cpu_start_};
    }

private:
    std::chrono::microseconds GetCPUTime() const {
#ifdef __linux__
        if (rusage usage; ::getrusage(static_cast<int>(type_), &usage)) {
            throw std::system_error{errno, std::generic_category()};
        } else {
            auto time = usage.ru_utime;
            return std::chrono::microseconds{1'000'000ll * time.tv_sec + time.tv_usec};
        }
#else
        auto time = Clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(time);
#endif
    }

    const Type type_;
    const std::chrono::time_point<Clock> wall_start_ = Clock::now();
    const std::chrono::microseconds cpu_start_ = GetCPUTime();
};

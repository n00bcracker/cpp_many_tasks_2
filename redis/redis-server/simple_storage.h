#pragma once

#include <map>
#include <variant>
#include <limits>

#include "storage.h"

constexpr auto kIntMin = std::numeric_limits<int64_t>::min();
constexpr auto kIntMax = std::numeric_limits<int64_t>::max();

namespace redis {

class SimpleStorage : public IStorage {
public:
    std::optional<std::string> Get(std::string_view key) const override;
    void Set(std::string_view key, std::string_view value) override;
    int64_t Change(std::string_view key, int64_t delta) override;

private:
    std::map<std::string, std::variant<int64_t, std::string>> kv_storage_;
};

}  // namespace redis

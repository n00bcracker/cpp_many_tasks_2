#include "simple_storage.h"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <variant>

namespace redis {

std::optional<std::string> SimpleStorage::Get(std::string_view key) const {
    std::string str_key(key);
    if (kv_storage_.contains(str_key)) {
        const auto& value = kv_storage_.at(str_key);
        if (std::holds_alternative<int64_t>(value)) {
            return std::to_string(std::get<int64_t>(value));
        } else {
            return std::get<std::string>(value);
        }
    } else {
        return std::nullopt;
    }
}

void SimpleStorage::Set(std::string_view key, std::string_view value) {
    std::string str_key(key);
    std::string str_value(value);
    int64_t int_value;
    try {
        size_t pos;
        int_value = std::stoll(str_value, &pos);
        if (pos != str_value.size()) {
            throw std::bad_cast();
        }

        kv_storage_[str_key].emplace<int64_t>(int_value);
    } catch (...) {
        kv_storage_[str_key].emplace<std::string>(str_value);
    }
}

int64_t SimpleStorage::Change(std::string_view key, int64_t delta) {
    std::string str_key(key);
    if (kv_storage_.contains(std::string(key))) {
        const auto& value = kv_storage_.at(str_key);
        if (std::holds_alternative<std::string>(value)) {
            throw std::bad_typeid();
        }
    } else {
        kv_storage_[str_key] = 0;
    }

    int64_t int_value = std::get<int64_t>(kv_storage_.at(str_key));
    const auto check_limits_func = [](int64_t num, int64_t delta) {
        if (delta >= 0) {
            if (num > kIntMax - delta) {
                throw std::out_of_range("oveflow");
            }
        } else {
            if (num < kIntMin - delta) {
                throw std::out_of_range("oveflow");
            }
        }
    };

    check_limits_func(int_value, delta);
    kv_storage_[str_key] = int_value + delta;
    return std::get<int64_t>(kv_storage_.at(str_key));
}

}  // namespace redis

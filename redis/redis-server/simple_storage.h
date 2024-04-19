#pragma once

#include "storage.h"

namespace redis {

class SimpleStorage : public IStorage {
public:
    std::optional<std::string> Get(std::string_view key) const override;
    void Set(std::string_view key, std::string_view value) override;
    int64_t Change(std::string_view key, int64_t delta) override;
};

}  // namespace redis

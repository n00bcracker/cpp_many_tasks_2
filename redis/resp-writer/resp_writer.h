#pragma once

#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

#include <cactus/io/writer.h>

namespace redis {

class RespWriter {
public:
    explicit RespWriter(cactus::IWriter* writer) : writer_(writer) {
    }

    void WriteSimpleString(std::string_view s);
    void WriteError(std::string_view s);
    void WriteInt(int64_t n);

    void WriteBulkString(std::string_view s);
    void WriteNullBulkString();

    void StartArray(size_t size);
    void WriteNullArray();

    void WriteArrayInts(auto&& range) {
        std::vector<int64_t> array;
        for (auto&& number : range) {
            array.emplace_back(number);
        }

        StartArray(array.size());
        for (const auto& number : array) {
            WriteInt(number);
        }
    }

private:
    void FinishMessageField();

    cactus::IWriter* writer_;
};

}  // namespace redis

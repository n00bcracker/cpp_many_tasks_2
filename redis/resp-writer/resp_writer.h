#pragma once

#include <string_view>

#include <cactus/io/writer.h>

namespace redis {

class RespWriter {
public:
    explicit RespWriter(cactus::IWriter* writer);

    void WriteSimpleString(std::string_view s);
    void WriteError(std::string_view s);
    void WriteInt(int64_t n);

    void WriteBulkString(std::string_view s);
    void WriteNullBulkString();

    void StartArray(size_t size);
    void WriteNullArray();

    void WriteArrayInts(auto&& range);
};

}  // namespace redis

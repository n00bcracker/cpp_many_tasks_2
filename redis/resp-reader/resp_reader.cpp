#include "resp_reader.h"
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace redis {

ERespType RespReader::ReadType() {
    try {
        char type;
        reader_->ReadFull(cactus::View(type));
        return CharToRespType(type);
    } catch (...) {
        throw redis::RedisError("Error parsing simple string");
    }
}

void RespReader::ReadBasicStringToBuf() {
    buf_ = std::string(1023, '\0');
    size_t size = 0;
    while (true) {
        reader_->ReadFull(cactus::View(buf_.data() + size, 1));
        if (buf_[size] == '\r') {
            break;
        }

        if (buf_[size] == '\n') {
            throw redis::RedisError("Wrong string ending symbol");
        }

        ++size;
        if (buf_.size() == size) {
            buf_.resize(size * 2);
        }
    }
    reader_->ReadFull(cactus::View(buf_.data() + size, 1));
    if (buf_[size] != '\n') {
        throw redis::RedisError("Wrong string ending symbol");
    }

    buf_.resize(size);
}

std::string_view RespReader::ReadSimpleString() {
    try {
        ReadBasicStringToBuf();
        return buf_;
    } catch (...) {
        throw redis::RedisError("Error parsing simple string");
    }
}

std::string_view RespReader::ReadError() {
    try {
        ReadBasicStringToBuf();
        return buf_;
    } catch (...) {
        throw redis::RedisError("Error parsing error");
    }
}

int64_t RespReader::ReadInt() {
    try {
        ReadBasicStringToBuf();
        size_t pos;
        int64_t res = std::stoll(buf_, &pos);
        if (pos != buf_.size() || buf_[0] == ' ') {
            throw redis::RedisError("Wrong integer string format");
        }

        return res;
    } catch (...) {
        throw redis::RedisError("Error parsing integer");
    }
}

std::optional<std::string_view> RespReader::ReadBulkString() {
    try {
        ReadBasicStringToBuf();
        size_t pos;
        int64_t length = std::stoll(buf_, &pos);
        if (pos != buf_.size() || length < -1) {
            throw redis::RedisError("Wrong unsigned integer string format");
        }

        if (length == -1) {
            return std::nullopt;
        }

        buf_ = std::string(length + 2, '\0');
        reader_->ReadFull(cactus::View(buf_));
        if (buf_[length] != '\r' || buf_[length + 1] != '\n') {
            throw redis::RedisError("Wrong string ending symbol");
        }

        buf_.resize(length);
        return buf_;
    } catch (...) {
        throw redis::RedisError("Error parsing bulk string");
    }
}

int64_t RespReader::ReadArrayLength() {
    try {
        ReadBasicStringToBuf();
        size_t pos;
        int64_t length = std::stoll(buf_, &pos);
        if (pos != buf_.size() || length < -1) {
            throw redis::RedisError("Wrong unsigned integer string format");
        }

        return length;
    } catch (...) {
        throw redis::RedisError("Error parsing array legth");
    }
}

}  // namespace redis

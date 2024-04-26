#include "client.h"

#include <cactus/net/net.h>
#include <utility>
#include "error.h"
#include "resp-reader/resp_reader.h"
#include "resp-writer/resp_writer.h"
#include "resp_types.h"

namespace redis {

Client::Client(const cactus::SocketAddress& server_address)
    : conn_(cactus::DialTCP(server_address)), reader_(conn_.get()), writer_(conn_.get()) {
    commands_map_ = {{ECommands::GET, "GET"},
                     {ECommands::SET, "SET"},
                     {ECommands::INCR, "INCR"},
                     {ECommands::DECR, "DECR"}};
}

std::optional<std::string> Client::Get(std::string_view key) {
    writer_.StartArray(2);
    writer_.WriteBulkString(commands_map_.at(ECommands::GET));
    writer_.WriteBulkString(key);
    const auto resp_type = reader_.ReadType();
    if (resp_type == ERespType::BulkString) {
        const auto resp = reader_.ReadBulkString();
        if (resp.has_value()) {
            return std::string(*resp);
        } else {
            return std::nullopt;
        }
    } else if (resp_type == ERespType::Error) {
        std::string error(reader_.ReadError());
        throw RedisError(std::move(error));
    } else {
        throw RedisError("Wrong answer type");
    }
}

void Client::Set(std::string_view key, std::string_view value) {
    writer_.StartArray(3);
    writer_.WriteBulkString(commands_map_.at(ECommands::SET));
    writer_.WriteBulkString(key);
    writer_.WriteBulkString(value);
    const auto resp_type = reader_.ReadType();
    if (resp_type == ERespType::BulkString) {
        const auto resp = reader_.ReadBulkString();
        if (!resp.has_value() || *resp != "OK") {
            throw RedisError("Unsuccessful SET");
        }
    } else if (resp_type == ERespType::Error) {
        std::string error(reader_.ReadError());
        throw RedisError(std::move(error));
    } else {
        throw RedisError("Wrong answer type");
    }
}

int64_t Client::Increment(std::string_view key) {
    writer_.StartArray(2);
    writer_.WriteBulkString(commands_map_.at(ECommands::INCR));
    writer_.WriteBulkString(key);
    const auto resp_type = reader_.ReadType();
    if (resp_type == ERespType::Int) {
        return reader_.ReadInt();
    } else if (resp_type == ERespType::Error) {
        std::string error(reader_.ReadError());
        throw RedisError(std::move(error));
    } else {
        throw RedisError("Wrong answer type");
    }
}

int64_t Client::Decrement(std::string_view key) {
    writer_.StartArray(2);
    writer_.WriteBulkString(commands_map_.at(ECommands::DECR));
    writer_.WriteBulkString(key);
    const auto resp_type = reader_.ReadType();
    if (resp_type == ERespType::Int) {
        return reader_.ReadInt();
    } else if (resp_type == ERespType::Error) {
        std::string error(reader_.ReadError());
        throw RedisError(std::move(error));
    } else {
        throw RedisError("Wrong answer type");
    }
}

}  // namespace redis

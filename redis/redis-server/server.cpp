#include "server.h"
#include <sys/types.h>
#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include "cactus/core/fiber.h"
#include "cactus/core/scheduler.h"
#include "cactus/net/net.h"
#include "redis-client/client.h"
#include "resp-reader/resp_reader.h"
#include "resp-writer/resp_writer.h"
#include "resp_types.h"

namespace redis {

Server::Server(const cactus::SocketAddress& address, std::unique_ptr<IStorage> storage)
    : lsn_(cactus::ListenTCP(address)), storage_(std::move(storage)) {
    commands_ = {{"GET", ECommands::GET},
                 {"SET", ECommands::SET},
                 {"INCR", ECommands::INCR},
                 {"DECR", ECommands::DECR}};
}

Command::Command(ECommands command_type) : type_(command_type) {
}

ECommands Command::GetType() const {
    return type_;
}

CommandGet::CommandGet(const std::string_view& key) : Command(ECommands::GET), key_(key) {
}

const std::string_view CommandGet::GetKeyArg() const {
    return key_;
}

CommandSet::CommandSet(const std::string_view& key, const std::string_view& value)
    : Command(ECommands::SET), key_(key), value_(value) {
}

const std::string_view CommandSet::GetKeyArg() const {
    return key_;
}

const std::string_view CommandSet::GetValueArg() const {
    return value_;
}

CommandIncr::CommandIncr(const std::string_view& key) : Command(ECommands::INCR), key_(key) {
}

const std::string_view CommandIncr::GetKeyArg() const {
    return key_;
}

CommandDecr::CommandDecr(const std::string_view& key) : Command(ECommands::DECR), key_(key) {
}

const std::string_view CommandDecr::GetKeyArg() const {
    return key_;
}

std::unique_ptr<Command> Server::ReadCommand(RespReader& reader) {
    if (reader.ReadType() != ERespType::Array) {
        throw redis::RedisError("Wrong command format");
    }

    size_t command_len = reader.ReadArrayLength();
    if (command_len < 2 || command_len > 3) {
        throw redis::RedisError("Unknown command");
    }

    ECommands command_type;
    std::string command_key, command_value;
    for (size_t i = 0; i < command_len; ++i) {
        if (i == 0) {
            if (reader.ReadType() != ERespType::BulkString) {
                throw redis::RedisError("Wrong command format");
            }

            auto command_name = reader.ReadBulkString();
            if (!command_name.has_value() || !commands_.contains(std::string(*command_name))) {
                throw redis::RedisError("Wrong command name");
            }

            command_type = commands_.at(std::string(*command_name));
            if (((command_type == ECommands::GET || command_type == ECommands::INCR ||
                  command_type == ECommands::DECR) &&
                 command_len != 2) ||
                (command_type == ECommands::SET && command_len != 3)) {
                throw redis::RedisError("Wrong command format");
            }
        } else if (i == 1) {
            if (reader.ReadType() != ERespType::BulkString) {
                throw redis::RedisError("Wrong command format");
            }

            auto read_key = reader.ReadBulkString();
            if (!read_key.has_value()) {
                throw redis::RedisError("Command null key");
            }

            command_key = *read_key;
        } else if (i == 2) {
            if (reader.ReadType() != ERespType::BulkString) {
                throw redis::RedisError("Wrong command format");
            }

            auto read_value = reader.ReadBulkString();
            if (!read_value.has_value()) {
                throw redis::RedisError("Command null key");
            }

            command_value = *read_value;
        }
    }

    if (command_type == ECommands::GET) {
        return std::unique_ptr<CommandGet>(new CommandGet(command_key));
    } else if (command_type == ECommands::SET) {
        return std::unique_ptr<CommandSet>(new CommandSet(command_key, command_value));
    } else if (command_type == ECommands::INCR) {
        return std::unique_ptr<CommandIncr>(new CommandIncr(command_key));
    } else if (command_type == ECommands::DECR) {
        return std::unique_ptr<CommandDecr>(new CommandDecr(command_key));
    } else {
        throw redis::RedisError("Unknown command");
    }
}

void Server::ProcessCommand(RespWriter& writer, const std::unique_ptr<Command> command_ptr) {
    if (command_ptr->GetType() == ECommands::GET) {
        const auto& command = dynamic_cast<CommandGet&>(*command_ptr);
        const auto value = storage_->Get(command.GetKeyArg());
        if (value.has_value()) {
            writer.WriteBulkString(*value);
        } else {
            writer.WriteNullBulkString();
        }
    } else if (command_ptr->GetType() == ECommands::SET) {
        const auto& command = dynamic_cast<CommandSet&>(*command_ptr);
        storage_->Set(command.GetKeyArg(), command.GetValueArg());
        writer.WriteBulkString("OK");
    } else if (command_ptr->GetType() == ECommands::INCR) {
        const auto& command = dynamic_cast<CommandIncr&>(*command_ptr);
        const ino64_t new_value = storage_->Change(command.GetKeyArg(), 1);
        writer.WriteInt(new_value);
    } else if (command_ptr->GetType() == ECommands::DECR) {
        const auto& command = dynamic_cast<CommandDecr&>(*command_ptr);
        const ino64_t new_value = storage_->Change(command.GetKeyArg(), -1);
        writer.WriteInt(new_value);
    }
}

void Server::Listen() {
    while (std::shared_ptr<cactus::IConn> conn = lsn_->Accept()) {
        group_.Spawn([this, conn = std::move(conn)] {
            RespReader reader(conn.get());
            RespWriter writer(conn.get());

            while (true) {
                try {
                    auto command_ptr = ReadCommand(reader);
                    ProcessCommand(writer, std::move(command_ptr));
                } catch (std::exception& ex) {
                    break;
                }
            }
        });
    }
}

void Server::Run() {
    group_.Spawn([this] { Listen(); });
}

const cactus::SocketAddress& Server::GetAddress() const {
    return lsn_->Address();
}

}  // namespace redis

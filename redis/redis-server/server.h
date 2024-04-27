#pragma once

#include "cactus/cactus.h"
#include "cactus/core/fiber.h"
#include "resp-reader/resp_reader.h"
#include "resp-writer/resp_writer.h"
#include "storage.h"

#include <memory>
#include <string>
#include <string_view>

#include <cactus/net/address.h>
#include <cactus/net/net.h>
#include "redis-client/client.h"

namespace redis {

class Command {
public:
    Command(ECommands command_type);
    virtual ~Command() = default;
    ECommands GetType() const;

private:
    ECommands type_;
};

class CommandGet : public Command {
public:
    CommandGet(const std::string_view& key);
    const std::string_view GetKeyArg() const;

private:
    std::string key_;
};

class CommandSet : public Command {
public:
    CommandSet(const std::string_view& key, const std::string_view& value);
    const std::string_view GetKeyArg() const;
    const std::string_view GetValueArg() const;

private:
    std::string key_;
    std::string value_;
};

class CommandIncr : public Command {
public:
    CommandIncr(const std::string_view& key);
    const std::string_view GetKeyArg() const;

private:
    std::string key_;
};

class CommandDecr : public Command {
public:
    CommandDecr(const std::string_view& key);
    const std::string_view GetKeyArg() const;

private:
    std::string key_;
};

class Server {
public:
    Server(const cactus::SocketAddress& address, std::unique_ptr<IStorage> storage);

    void Run();
    void Listen();

    const cactus::SocketAddress& GetAddress() const;

private:
    std::unique_ptr<Command> ReadCommand(RespReader& reader);
    void ProcessCommand(RespWriter& writer, const std::unique_ptr<Command> command);

    std::unique_ptr<cactus::IListener> lsn_;
    cactus::ServerGroup group_;
    std::unique_ptr<IStorage> storage_;
    std::map<std::string, ECommands> commands_;
};

}  // namespace redis

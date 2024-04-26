#pragma once

#include <map>
#include <string>
#include <string_view>
#include <optional>

#include <cactus/net/address.h>
#include <cactus/net/net.h>
#include "../resp-reader/resp_reader.h"
#include "../resp-writer/resp_writer.h"

namespace redis {

enum class ECommands { GET, SET, INCR, DECR };

class Client {
public:
    explicit Client(const cactus::SocketAddress& server_address);

    std::optional<std::string> Get(std::string_view key);
    void Set(std::string_view key, std::string_view value);
    int64_t Increment(std::string_view key);
    int64_t Decrement(std::string_view key);

private:
    std::unique_ptr<cactus::IConn> conn_;
    RespReader reader_;
    RespWriter writer_;
    std::map<ECommands, std::string> commands_map_;
};

}  // namespace redis

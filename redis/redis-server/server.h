#pragma once

#include "storage.h"

#include <memory>

#include <cactus/net/address.h>

namespace redis {

class Server {
public:
    Server(const cactus::SocketAddress& address, std::unique_ptr<IStorage> storage);

    void Run();

    const cactus::SocketAddress& GetAddress() const;
};

}  // namespace redis

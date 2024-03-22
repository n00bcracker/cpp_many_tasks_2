#include "socks4.h"

void Sock4Connect(cactus::IConn* conn, const cactus::SocketAddress& endpoint,
                  const std::string& user) {
}

std::unique_ptr<cactus::IConn> DialProxyChain(const std::vector<Proxy>& proxies,
                                              const cactus::SocketAddress& endpoint) {
    return nullptr;
}

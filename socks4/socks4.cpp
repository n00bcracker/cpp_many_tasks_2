#include "socks4.h"
#include <netinet/in.h>
#include <sys/types.h>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <utility>
#include "cactus/io/reader.h"
#include "cactus/io/view.h"
#include "cactus/net/net.h"

void Sock4Connect(cactus::IConn* conn, const cactus::SocketAddress& endpoint,
                  const std::string& user) {
    u_char vn = 4;
    conn->Write(cactus::View(vn));
    u_char cd = 1;
    conn->Write(cactus::View(cd));
    uint16_t port = htons(endpoint.GetPort());
    conn->Write(cactus::View(port));
    uint32_t ip = endpoint.GetIp();
    conn->Write(cactus::View(ip));
    conn->Write(cactus::View(user));
    u_char null = 0;
    conn->Write(cactus::View(null));

    conn->ReadFull(cactus::View(vn));
    if (vn != 0) {
        throw std::exception();
    }
    conn->ReadFull(cactus::View(cd));
    if (cd != 90) {
        throw std::exception();
    }
    conn->ReadFull(cactus::View(port));
    conn->ReadFull(cactus::View(ip));
}

std::unique_ptr<cactus::IConn> DialProxyChain(const std::vector<Proxy>& proxies,
                                              const cactus::SocketAddress& endpoint) {
    try {
        if (proxies.empty()) {
            return cactus::DialTCP(endpoint);
        }

        auto conn = cactus::DialTCP(proxies[0].proxy_address);
        size_t i = 1;
        for (; i < proxies.size(); ++i) {
            Sock4Connect(conn.get(), proxies[i].proxy_address, proxies[i - 1].username);
        }

        Sock4Connect(conn.get(), endpoint, proxies[i - 1].username);
        return conn;
    } catch (...) {
        return nullptr;
    }
}

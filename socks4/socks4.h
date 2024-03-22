#pragma once

#include <memory>
#include <vector>
#include <string>

#include <cactus/cactus.h>

// Socks4Connect посылает в conn запрос CONNECT и принимает ответ от прокси сервера.
//
// Функция бросает исключение в случае ошибки.
void Sock4Connect(cactus::IConn* conn, const cactus::SocketAddress& endpoint,
                  const std::string& user);

struct Proxy {
    cactus::SocketAddress proxy_address;
    std::string username;
};

// DialProxyChain подключается к endpoint через цепочку proxies.
std::unique_ptr<cactus::IConn> DialProxyChain(const std::vector<Proxy>& proxies,
                                              const cactus::SocketAddress& endpoint);

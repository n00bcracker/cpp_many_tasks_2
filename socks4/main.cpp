#include <iostream>
#include "socks4.h"

int main() {
    cactus::Scheduler scheduler;
    scheduler.Run([&] {
        std::vector<Proxy> proxies{
            {cactus::SocketAddress("cpp-cactus-24.ru", 12000, true), "prime"},
            {cactus::SocketAddress("4.4.4.4", 80, false), "prime"}};
        cactus::SocketAddress endpoint("8.8.8.8", 443, false);
        auto conn = DialProxyChain(proxies, endpoint);
        std::string msg(1000, '\0');
        conn->Read(cactus::View(msg));
        std::cout << "Message: " << std::string_view(msg.data()) << std::endl;
    });
}

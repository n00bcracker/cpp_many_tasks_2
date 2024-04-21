#include "portknock.h"
#include <chrono>
#include <iostream>

int main() {
    cactus::Scheduler scheduler;
    scheduler.Run([&] {
        cactus::SocketAddress addr("cpp-cactus-24.ru", 0, true);
        Ports ports{
            {10001, KnockProtocol::TCP}, {10022, KnockProtocol::TCP}, {10003, KnockProtocol::TCP}};
        KnockPorts(addr, ports, std::chrono::seconds(3));

        addr.SetPort(10080);
        auto conn = DialTCP(addr);
        std::vector<char> msg(100);
        conn->Read(cactus::View(msg));
        conn->Close();
        std::cout << "Message: " << std::string_view(msg.data()) << std::endl;
    });
}

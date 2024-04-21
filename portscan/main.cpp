#include <cactus/cactus.h>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include "cactus/io/view.h"
#include "cactus/net/address.h"
#include "portscan.h"

int main() {
    cactus::Scheduler scheduler;
    scheduler.Run([&] {
        cactus::SocketAddress addr("51.250.102.65", 11173);
        auto conn = DialTCP(addr);
        std::vector<char> msg(100);
        conn->Read(cactus::View(msg));
        std::cout << "Message: " << std::string_view(msg.data()) << std::endl;
    });
}

#include "fanout.h"
#include <iostream>

int main() {
    cactus::Scheduler scheduler;
    std::string msg;
    scheduler.Run([&] {
        cactus::SocketAddress server("cpp-cactus-24.ru", 13000, true);
        msg = Fanout(server);
    });

    std::cout << "Message: " << msg << std::endl;
}

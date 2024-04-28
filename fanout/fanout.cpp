#include "fanout.h"
#include <sys/types.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <vector>
#include "cactus/cactus.h"
#include "cactus/io/view.h"
#include "cactus/net/net.h"

using namespace std::chrono_literals;

std::vector<uint64_t> GetTokens(cactus::IConn* conn) {
    uint64_t tokens_size;
    conn->ReadFull(cactus::View(tokens_size));
    std::vector<uint64_t> res(tokens_size);
    conn->ReadFull(cactus::View(res));
    return res;
}

uint64_t GetSecret(const cactus::SocketAddress& address, const std::vector<uint64_t>& tokens) {
    std::vector<uint64_t> secret_parts(tokens.size(), 0);
    cactus::WaitGroup group;
    for (size_t i = 0; i < tokens.size(); ++i) {
        group.Spawn([&, i]() {
            cactus::SocketAddress cur_address(address);
            cur_address.SetPort(cur_address.GetPort() + i + 1);
            auto conn = cactus::DialTCP(cur_address);
            conn->Write(cactus::View(tokens[i]));
            uint64_t secret_part;
            conn->ReadFull(cactus::View(secret_part));
            secret_parts[i] = secret_part;
        });
    }
    group.Wait();

    uint64_t secret = std::reduce(std::begin(secret_parts), std::end(secret_parts), 0ul);
    return secret;
}

std::string Fanout(const cactus::SocketAddress& address) {
    auto conn = cactus::DialTCP(address);
    const auto tokens = GetTokens(conn.get());
    uint64_t secret = GetSecret(address, tokens);
    conn->Write(cactus::View(secret));
    std::string flag = conn->ReadAllToString();
    return flag;
}

#pragma once

#include <memory>

#include <cactus/cactus.h>

class SocksServer {
public:
    explicit SocksServer(const cactus::SocketAddress& at);
    void Serve();
    const cactus::SocketAddress& GetAddress() const;

private:
    void MakeConn(cactus::IConn* conn);
    void TransferData(cactus::IConn* in_conn, cactus::IConn* out_conn);

    std::unique_ptr<cactus::IListener> lsn_;
    cactus::ServerGroup group_;
};

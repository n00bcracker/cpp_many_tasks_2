#include "balancer.h"
#include <chrono>
#include <memory>
#include <utility>
#include "cactus/net/address.h"
#include "cactus/net/net.h"
#include "cactus/sync/mutex.h"

struct Peer {
    cactus::SocketAddress address;
    size_t load;
    bool is_dead;
};

class Balancer : public IBalancer {
public:
    Balancer(const cactus::SocketAddress& address);
    void SetBackends(const std::vector<cactus::SocketAddress>& peers) override;
    void Run() override;
    const cactus::SocketAddress& GetAddress() const override;

private:
    void MakeConnection(std::shared_ptr<cactus::IConn> client_conn);
    void ProcessConnection(std::shared_ptr<cactus::IConn> client_conn,
                           std::shared_ptr<cactus::IConn> peer_conn, std::shared_ptr<Peer> peer);
    std::unique_ptr<cactus::IListener> lsn_;
    mutable std::vector<std::shared_ptr<Peer>> peers_;
    cactus::Mutex mutex_;
    cactus::ServerGroup server_;
};

Balancer::Balancer(const cactus::SocketAddress& address) : lsn_(cactus::ListenTCP(address)) {
}

void Balancer::SetBackends(const std::vector<cactus::SocketAddress>& peers) {
    cactus::MutexGuard lock(mutex_);
    peers_.clear();
    for (const auto& addr : peers) {
        peers_.emplace_back(std::make_shared<Peer>(addr, 0, false));
    }
}

const cactus::SocketAddress& Balancer::GetAddress() const {
    return lsn_->Address();
}

void TransferData(cactus::IConn* in_conn, cactus::IConn* out_conn) {
    std::string buf(256, '\0');
    size_t read_byte_cnt = 0;

    try {
        while (true) {
            read_byte_cnt = in_conn->Read(cactus::View(buf));
            if (!read_byte_cnt) {
                break;
            }

            out_conn->Write(cactus::View(buf.data(), read_byte_cnt));
        }

        out_conn->CloseWrite();
    } catch (...) {
    }
}

void Balancer::ProcessConnection(std::shared_ptr<cactus::IConn> client_conn,
                                 std::shared_ptr<cactus::IConn> peer_conn,
                                 std::shared_ptr<Peer> peer) {
    cactus::WaitGroup peer_group;
    peer_group.Spawn([in_conn = client_conn.get(), out_conn = peer_conn.get()] {
        TransferData(in_conn, out_conn);
    });

    peer_group.Spawn([in_conn = peer_conn.get(), out_conn = client_conn.get()] {
        TransferData(in_conn, out_conn);
    });

    peer_group.Wait();

    cactus::MutexGuard lock(mutex_);
    --peer->load;
}

void Balancer::MakeConnection(std::shared_ptr<cactus::IConn> client_conn) {
    std::shared_ptr<Peer> suitable_peer;
    while (true) {
        cactus::MutexGuard lock(mutex_);
        suitable_peer = nullptr;

        for (const auto& peer : peers_) {
            if (!peer->is_dead && (!suitable_peer || peer->load < suitable_peer->load)) {
                suitable_peer = peer;
            }
        }

        const auto connect = [&]() -> std::unique_ptr<cactus::IConn> {
            cactus::TimeoutGuard guard{std::chrono::milliseconds(100)};
            try {
                return DialTCP(suitable_peer->address);
            } catch (const cactus::TimeoutException& ex) {
                return nullptr;
            } catch (...) {
                return nullptr;
            }
        };

        std::shared_ptr<cactus::IConn> proxy_conn = nullptr;
        if (suitable_peer) {
            proxy_conn = connect();
        } else {
            throw std::system_error();
        }

        if (proxy_conn) {
            ++suitable_peer->load;
            server_.Spawn([this, client_conn = std::move(client_conn),
                           peer_conn = std::move(proxy_conn), peer = std::move(suitable_peer)] {
                ProcessConnection(client_conn, peer_conn, peer);
            });
            break;
        } else {
            suitable_peer->is_dead = true;
        }
    }
}

void Balancer::Run() {
    server_.Spawn([this] {
        while (std::shared_ptr conn = lsn_->Accept()) {
            server_.Spawn([this, client_conn = std::move(conn)] { MakeConnection(client_conn); });
        }
    });
}

std::unique_ptr<IBalancer> CreateBalancer(const cactus::SocketAddress& address) {
    return std::make_unique<Balancer>(address);
}
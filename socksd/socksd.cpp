#include "socksd.h"
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include "cactus/cactus.h"
#include "cactus/io/view.h"
#include "cactus/net/net.h"

using namespace std::chrono_literals;

void ReadUserName(cactus::IConn* conn) {
    std::string user_name(257, '\0');
    size_t length = 0;

    do {
        auto read_cnt = conn->Read(cactus::View(user_name.data() + length, 1));
        if (!read_cnt) {
            throw cactus::EOFException{};
        }
        ++length;
    } while (length < user_name.size() && user_name[length - 1] != 0);

    if (user_name[length - 1] != 0) {
        throw std::runtime_error("Wrong user name");
    }

    user_name.resize(length - 1);
}

SocksServer::SocksServer(const cactus::SocketAddress& at) : lsn_(cactus::ListenTCP(at)) {
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

void MakeConn(cactus::IConn* conn) {
    u_char vn;
    u_char cd;
    uint16_t port;
    uint32_t ip;
    std::shared_ptr<cactus::IConn> proxy_conn;

    try {
        conn->ReadFull(cactus::View(vn));
        if (vn != 4) {
            throw std::runtime_error("Wrong request");
        }

        conn->ReadFull(cactus::View(cd));
        if (cd != 1) {
            throw std::runtime_error("Wrong request");
        }

        conn->ReadFull(cactus::View(port));
        conn->ReadFull(cactus::View(ip));

        ReadUserName(conn);

        cactus::SocketAddress endpoint(ip, htons(port));
        proxy_conn = cactus::DialTCP(endpoint);

    } catch (const cactus::EOFException&) {
        return;
    } catch (...) {
        vn = 0;
        conn->Write(cactus::View(vn));

        cd = 91;
        conn->Write(cactus::View(cd));
        port = 0;
        conn->Write(cactus::View(port));
        ip = 0;
        conn->Write(cactus::View(ip));
        conn->CloseWrite();
    }

    vn = 0;
    conn->Write(cactus::View(vn));
    cd = 90;
    conn->Write(cactus::View(cd));
    port = 0;
    conn->Write(cactus::View(port));
    ip = 0;
    conn->Write(cactus::View(ip));

    cactus::WaitGroup proxy_group;
    proxy_group.Spawn(
        [in_conn = conn, out_conn = proxy_conn.get()] { TransferData(in_conn, out_conn); });

    proxy_group.Spawn(
        [in_conn = proxy_conn.get(), out_conn = conn] { TransferData(in_conn, out_conn); });

    proxy_group.Wait();
}

void SocksServer::Serve() {
    group_.Spawn([this] {
        while (std::shared_ptr conn = lsn_->Accept()) {
            group_.Spawn([conn = std::move(conn)] { MakeConn(conn.get()); });
        }
    });
}

const cactus::SocketAddress& SocksServer::GetAddress() const {
    return lsn_->Address();
}
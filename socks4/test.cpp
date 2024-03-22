#include "socks4.h"

#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include <cactus/cactus.h>
#include <cactus/test.h>

namespace {

struct FakeConn : cactus::IConn {
    explicit FakeConn(std::string in = "") : in_{std::move(in)}, in_view_{cactus::View(in_)} {
    }

    size_t Read(cactus::MutableView buf) override {
        auto n = cactus::Copy(buf, in_view_);
        in_view_ = in_view_.subspan(n);
        return n;
    }

    void Write(cactus::ConstView buf) override {
        out_.append(reinterpret_cast<const char*>(buf.data()), buf.size());
    }

    void CloseWrite() override {
    }

    void Close() override {
    }

    const cactus::SocketAddress& RemoteAddress() const override {
        throw std::runtime_error{"Not implemented"};
    }

    const cactus::SocketAddress& LocalAddress() const override {
        throw std::runtime_error{"Not implemented"};
    }

    std::string_view OutView(size_t offset, size_t count) const {
        return std::string_view{out_}.substr(offset, count);
    }

    size_t OutSize() const {
        return out_.size();
    }

private:
    const std::string in_;
    cactus::ConstView in_view_;
    std::string out_;
};

}  // namespace

FIBER_TEST_CASE("Protocol Check") {
    FakeConn c{{0x00, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

    Sock4Connect(&c, {"1.2.3.4", 80}, "prime");

    REQUIRE(c.OutSize() == 8 + 6);
    CHECK(c.OutView(0, 2) == "\x04\x01");
    CHECK(c.OutView(2, 2) == std::string_view{"\x00\x50", 2});
    CHECK(c.OutView(4, 4) == "\x01\x02\x03\x04");
    CHECK(c.OutView(8, 6) == std::string_view{"prime\0", 6});
}

FIBER_TEST_CASE("Error check") {
    FakeConn c{{0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    CHECK_THROWS(Sock4Connect(&c, {"1.2.3.4", 80}, "prime"));
}

FIBER_TEST_CASE("Error check another") {
    FakeConn c{{0x01, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    CHECK_THROWS(Sock4Connect(&c, {"1.2.3.4", 80}, "prime"));
}

FIBER_TEST_CASE("Size Check") {
    FakeConn c;
    CHECK_THROWS(Sock4Connect(&c, {"1.2.3.4", 80}, "prime"));
}

FIBER_TEST_CASE("Direct connect") {
    auto lsn = cactus::ListenTCP({"127.0.0.1", 0});

    auto conn = DialProxyChain({}, lsn->Address());

    auto server_conn = lsn->Accept();

    REQUIRE(conn->RemoteAddress() == lsn->Address());
    REQUIRE(conn->LocalAddress() == server_conn->RemoteAddress());

    auto expected = 42;
    conn->Write(cactus::View(expected));
    int read;
    server_conn->ReadFull(cactus::View(read));
    CHECK(read == expected);
}

FIBER_TEST_CASE("Chain connect") {
    auto lsn = cactus::ListenTCP({"127.0.0.1", 0});

    std::vector<Proxy> proxies = {
        {lsn->Address(), "prime"},
        {{"8.8.8.8", 22}, "lev"},
    };

    cactus::Fiber fiber{[&] {
        char good_reply[] = {0x00, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        auto conn = lsn->Accept();

        std::string req(8 + 6, '\0');
        conn->ReadFull(cactus::View(req));

        REQUIRE(req.substr(0, 2) == "\x04\x01");
        REQUIRE(req.substr(2, 2) == std::string{"\x00\x16", 2});
        REQUIRE(req.substr(4, 4) == std::string{"\x08\x08\x08\x08"});
        REQUIRE(req.substr(8) == std::string{"prime\0", 6});

        conn->Write(cactus::View(good_reply));

        std::string req2(8 + 4, '\0');
        conn->ReadFull(cactus::View(req2));

        REQUIRE(req2.substr(0, 2) == "\x04\x01");
        REQUIRE(req2.substr(2, 2) == std::string{"\x04\xd2", 2});
        REQUIRE(req2.substr(4, 4) == std::string{"\x01\x02\x03\x04"});
        REQUIRE(req2.substr(8) == std::string{"lev\0", 4});

        conn->Write(cactus::View(good_reply));
        conn->Write(cactus::View("pong", 4));
    }};

    auto conn = DialProxyChain(proxies, {"1.2.3.4", 1'234});
    CHECK(conn->ReadAllToString() == "pong");
}

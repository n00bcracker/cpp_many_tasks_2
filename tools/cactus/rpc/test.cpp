#include <cactus/test.h>

#include <cactus/rpc/rpc_test.pb.h>

struct TestServiceDummy : cactus::TestServiceHandler {
    void Search(const cactus::TestRequest& request, cactus::TestResponse* response) override {
        response->set_results(request.query());
    }

    void Ping(const cactus::PingRequest&, cactus::PingResponse*) override {
        throw std::runtime_error{"pong"};
    }
};

FIBER_TEST_CASE("Simple call") {
    folly::SocketAddress address{"127.0.0.1", 15860};

    TestServiceDummy dummy;
    cactus::SimpleRpcServer server(address);
    server.Register(&dummy);

    cactus::ServerGroup g;
    g.Spawn([&] { server.Serve(); });

    cactus::SimpleRpcChannel channel(address);
    cactus::TestServiceClient client(&channel);

    cactus::TestRequest search_request;
    search_request.set_query("пластиковые окна");
    auto search_response = client.Search(search_request);
    REQUIRE("пластиковые окна" == search_response->results());

    REQUIRE_THROWS_AS(client.Ping(cactus::PingRequest{}), cactus::RpcCallError);
}

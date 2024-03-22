#include <cactus/test.h>
#include <cactus/rpc/rpc_test.pb.h>

TEST_CASE("Example message") {
    cactus::TestRequest req;
    req.set_query("ping");

    cactus::TestResponse rsp;
    rsp.set_results("pong");
}

class TestChannel : public cactus::IRpcChannel {
public:
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    const google::protobuf::Message& request,
                    google::protobuf::Message* response) override {

        REQUIRE(method->name() == "Search");

        auto req = dynamic_cast<const cactus::TestRequest&>(request);
        auto rsp = dynamic_cast<cactus::TestResponse*>(response);

        rsp->set_results(req.query());
    }
};

TEST_CASE("Check client codegen") {
    TestChannel channel;

    cactus::TestServiceClient stub(&channel);

    cactus::TestRequest req;
    req.set_query("foobar");

    auto rsp = stub.Search(req);
    REQUIRE(rsp->results() == "foobar");
}

struct TestServiceImpl : cactus::TestServiceHandler {
    int searched = 0;
    int pinged = 0;

    void Search(const cactus::TestRequest&, cactus::TestResponse*) override {
        searched++;
    }

    void Ping(const cactus::PingRequest&, cactus::PingResponse*) override {
        pinged++;
    }
};

TEST_CASE("Check server codegen") {
    TestServiceImpl impl;
    cactus::TestServiceClient client(&impl);

    client.Search(cactus::TestRequest{});
    REQUIRE(impl.searched == 1);
    REQUIRE(impl.pinged == 0);

    client.Ping(cactus::PingRequest{});
    REQUIRE(impl.searched == 1);
    REQUIRE(impl.pinged == 1);
}

TEST_CASE("Example descriptors") {
    TestServiceImpl handler;
    auto descriptor = handler.ServiceDescriptor();

    REQUIRE(descriptor->name() == "TestService");
    REQUIRE(descriptor->full_name() == "cactus.TestService");

    auto method = descriptor->FindMethodByName("Search");
    REQUIRE(method->name() == "Search");
    REQUIRE(method->full_name() == "cactus.TestService.Search");

    // default instance of message.
    auto request_prototype =
        google::protobuf::MessageFactory::generated_factory()->GetPrototype(method->input_type());

    // new request object.
    std::unique_ptr<google::protobuf::Message> request{request_prototype->New()};
    REQUIRE(request->GetTypeName() == "cactus.TestRequest");
}

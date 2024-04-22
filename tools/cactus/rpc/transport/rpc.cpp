#include "../rpc.h"

#include <cactus/rpc/rpc.pb.h>
#include <array>
#include <string>
#include "cactus/io/view.h"

namespace {

struct FixedHeader {
    uint32_t header_size;
    uint32_t body_size;
} __attribute__((packed));

}  // namespace

namespace cactus {

void SimpleRpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                                  const google::protobuf::Message& request,
                                  google::protobuf::Message* response) {
    try {
        auto conn = DialTCP(server_);
        RequestHeader request_header_msg;
        request_header_msg.set_service(method->service()->full_name());
        request_header_msg.set_method(method->name());
        const std::string serialized_request_header = request_header_msg.SerializeAsString();
        const std::string serialized_request_body = request.SerializeAsString();
        FixedHeader request_header(serialized_request_header.size(),
                                   serialized_request_body.size());

        conn->Write(View(request_header));
        conn->Write(View(serialized_request_header));
        conn->Write(View(serialized_request_body));

        FixedHeader response_header;
        conn->ReadFull(View(response_header));

        ResponseHeader response_header_msg;
        std::string serialized_response_header(response_header.header_size, '\0');
        conn->ReadFull(View(serialized_response_header));
        response_header_msg.ParseFromString(serialized_response_header);

        if (response_header_msg.has_rpc_error()) {
            throw RpcCallError(response_header_msg.rpc_error().message());
        }

        std::string serialized_response_body(response_header.body_size, '\0');
        conn->ReadFull(View(serialized_response_body));
        response->ParseFromString(serialized_response_body);
    } catch (...) {
        throw RpcCallError("Connection error");
    }
}

void SimpleRpcServer::Serve() {
    while (true) {
        auto conn = lsn_->Accept();
        try {
            FixedHeader request_header;
            conn->ReadFull(View(request_header));

            RequestHeader request_header_msg;
            std::string serialized_request_header(request_header.header_size, '\0');
            conn->ReadFull(View(serialized_request_header));
            if (!request_header_msg.ParseFromString(serialized_request_header)) {
                throw std::runtime_error("invalid msg");
            }

            auto& service = services_.at(request_header_msg.service());
            const auto* method =
                service->ServiceDescriptor()->FindMethodByName(request_header_msg.method());

            std::string serialized_request_body(request_header.body_size, '\0');
            conn->ReadFull(View(serialized_request_body));
            auto factory = google::protobuf::MessageFactory::generated_factory();
            auto request_prototype = factory->GetPrototype(method->input_type());
            std::unique_ptr<google::protobuf::Message> request(request_prototype->New());
            request->ParseFromString(serialized_request_body);
            auto response_prototype = factory->GetPrototype(method->output_type());
            std::unique_ptr<google::protobuf::Message> response(response_prototype->New());
            service->CallMethod(method, *request, response.get());

            ResponseHeader response_header_msg;
            const std::string serialized_response_header = response_header_msg.SerializeAsString();
            const std::string serialized_response_body = response->SerializeAsString();
            FixedHeader response_header(serialized_response_header.size(),
                                        serialized_response_body.size());

            conn->Write(View(response_header));
            conn->Write(View(serialized_response_header));
            conn->Write(View(serialized_response_body));
            conn->CloseWrite();
        } catch (...) {
            ResponseHeader response_header_msg;
            response_header_msg.mutable_rpc_error()->set_message("Server error");
            const std::string serialized_response_header = response_header_msg.SerializeAsString();
            FixedHeader response_header(serialized_response_header.size(), 0);
            conn->Write(View(response_header));
            conn->Write(View(serialized_response_header));
            conn->CloseWrite();
        }
    }
}

}  // namespace cactus
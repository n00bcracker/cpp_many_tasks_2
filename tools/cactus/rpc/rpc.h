#pragma once

#include <google/protobuf/descriptor.h>

#include <cactus/cactus.h>

namespace cactus {

class IRpcChannel {
public:
    virtual ~IRpcChannel() = default;

    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            const google::protobuf::Message& request,
                            google::protobuf::Message* response) = 0;
};

class IRpcService : public IRpcChannel {
public:
    virtual const google::protobuf::ServiceDescriptor* ServiceDescriptor() = 0;
};

class RpcCallError : public std::exception {
public:
    RpcCallError(const std::string& error) : error_{error} {
    }

    const char* what() const noexcept override {
        return error_.c_str();
    }

private:
    std::string error_;
};

class SimpleRpcChannel : public IRpcChannel {
public:
    explicit SimpleRpcChannel(const folly::SocketAddress& server) : server_{server} {
    }

    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    const google::protobuf::Message& request,
                    google::protobuf::Message* response) override;

private:
    folly::SocketAddress server_;
};

class SimpleRpcServer {
public:
    explicit SimpleRpcServer(const folly::SocketAddress& at) : lsn_{ListenTCP(at)} {
    }

    void Register(IRpcService* service) {
        services_[service->ServiceDescriptor()->full_name()] = service;
    }

    void Serve();

private:
    std::unique_ptr<IListener> lsn_;
    std::unordered_map<std::string, IRpcService*> services_;
    ServerGroup group_;
};

}  // namespace cactus

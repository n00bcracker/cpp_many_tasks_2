# gen-rpc

Вам нужно реализовать плагин к `protoc`, который будет генерировать
код для работы с rpc.

Для каждого сервиса в proto файле, ваш плагин должен генерировать два класса.

```proto
service TestService {
    rpc Search (TestRequest) returns (TestResponse) {}
}
```

```c++
class TestServiceClient {
public:
    TestServiceClient(IRpcChannel* channel) : channel_{channel} {}
    
    // Внутри вызывает метод CallMethod у сhannel.
    std::unique_ptr<TestResponse> Search(const TestRequest&);
    
private:
    IRpcChannel* channel_ = nullptr;
};

class TestServiceHandler : public IRpcService {
public:
    // Смотрит на method->index() и вызывает соответствующий метод.
    // Сообщения даункастить static_cast-ом.
    // Неправильные входные данные можно считать UB.
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    const google::protobuf::Message& request,
                    google::protobuf::Message* response) override;
    
    // Виртуальный метод для реализации пользовательским кодом.                
    virtual void Search(const TestRequest& req, TestResponse* rsp) = 0;
    
    // Реализация IRpcService. Нужно просто вернуть TestService::descriptor();
    // Класс TestService уже генерирует protoc.
    const google::protobuf::ServiceDescriptor* ServiceDescriptor() override;
};
```

Плагин находится в файле rpc_generator.cpp. С помошью `GeneratorContext::OpenForInsert`
он должен дописывать в `pb.cc` и `pb.h` файлы свой код. В файле уже есть пример работы с этим api.

Чтобы тестировать код, запускайте `test_cactus_rpc_gen` и смотрите на ошибки компиляции и тестов.

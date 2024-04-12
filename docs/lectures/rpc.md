---
marp: true
theme: gaia
class: invert
---

# Собираем RPC

---

# service descriptor

```
service SearchService {
	rpc Search (SearchRequest) returns (SearchResponse) {}
}
```

```
REQUIRE(descriptor->name() == "SearchService");
REQUIRE(descriptor->full_name() == "cactus.SearchService");
auto method = descriptor->FindMethodByName("Search");
REQUIRE(method->name() == "Search");

auto factory = MessageFactory::generated_factory();
auto request_prototype = factory->GetPrototype(method->input_type());

std::unique_ptr<Message> request(request_prototype->New());
```

---

# client stub

```
service SearchService {
	rpc Search (SearchRequest) returns (SearchResponse) {}
}
```

```
class SearchServiceClient {
public:
	SearchServiceClient(IRpcChannel* channel);
	std::unique_ptr<SearchResponse> Search(const SearchRequest& request);
};

SearchServiceClient client(&channel);
SearchRequest req; req.set_query("foobar");
auto rsp = client.Search(req);
```

---

# service handler

```
class SearchServiceHandler : public IRpcService {
public:
	virtual void Search(const SearchRequest& req, SearchResponse* rsp) = 0;
};


class MyService : public SearchServiceHandler {
public:
	void Search(const SearchRequest& req, SearchResponse* rsp) override {
		// do stuff
	}
};
```

---

# Путь запроса

1. `SearchServiceClient`
2. `IRpcChannel`
3. Клиент к конретному rpc протоколу
4. Сеть
5. Сервер конкретного rpc протокола
6. `IRpcService`
7. `SearchServiceHandler`

---

# RPC транспорт

```
class IRpcChannel {
	public:
	virtual void CallMethod(const MethodDescriptor *method,
							const Message &request,
							Message *response) = 0;
};

class IRpcService : public IRpcChannel {
public:
	virtual const ServiceDescriptor *ServiceDescriptor() = 0;
}
```

---

# Реализация RPC транспорта

```
class SimpleRpcChannel {
public:
	SimpleRpcChannel(folly::SocketAddress endpoint);
};

void SimpleRpcChannel::CallMethod(const MethodDescriptor *method,
								  const Message& request,
								  Message *response) {
	// open connection
	// write header
	// write request body
	// read header
	// read response body
}
```

---

# Реализация RPC сервера

```
class SimpleRpcServer {
public:
	SimpleRpcServer(folly::SocketAddress at);
	void Register(IRpcService *service);
	void Serve();
};

void SimpleRpcServer::HandleClient(IConn* client) {
	// read header
	// lookup service and method
	// read request body
	// invoke handler
	// write header
	// write response body
}
```

---

# Всё вместе

```
MyTestService dummy;
SimpleRpcServer server(kTestAddress);
server.Register(&dummy);
server.Serve();
```

```
SimpleRpcChannel channel(kTestAddress);
TestServiceClient client(&channel);

TestRequest search_request;
search_request.set_query("пластиковые окна");
auto search_response = client.Search(search_request);
```

---

# Обработка ошибок

* Нельзя передать `std::exception_ptr` по сети
* Любое исключение превращается в `RpcCallError`. Сохраняется только текст.
* Специальный тип ошибки явно передаются в протоколе.

---

# Прямая и обратная совместимость

```
message User {
	string name = 1;
	string full_name = 2;
}

message User {
	string full_name = 2;
	string address = 3;
}
```

1. Записать старым кодом и прочитать новым.
2. Записать новым кодом и прочитать старым.

---

# Совместимость формата

```
message User {
	string name = 1;
	string full_name = 2;
}

message User {
	string full_name = 2;
	string address = 3;
}
```

1. Сообщение - последовательность пар (тэг, значение).
2. Любое поле может отсутствовать.
3. Неизвестные поля игнорируются.

---


# transport

Реализуйте простой транспорт для RPC.

Реализация должна находиться в классах `SimpleRpcChannel` и `SimpleRpcServer`.

## Протокол

Клиент подключается к серверу по TCP и пишет запрос.

В начале запроса идёт фиксированный заголовок.

```c++
struct FixedHeader {
    uint32_t header_size;
    uint32_t body_size;
};
```

За заголовком следует proto сообщение типа RequestHeader и размера header_size.
Затем идёт proto сообщение запроса размера body_size.

Сервер отвечает фиксированным заголовком. За заголовком идёт proto сообщение типа ResponseHeader размера header_size.
За ним идёт сообщение типа с ответом на запрос.

Тесты находятся в исполняемом файле `test_cactus_rpc`.

## Замечания

- Классы RequestHeader и ResponseHeader определены в rpc.proto.
- Если вы правильно сделали задание rpc-gen, то файл `rpc.pb.h` у вас
  инклюдит файл `rpc.h`. Это значит, что `rpc.pb.h` нельзя инклюдить в `rpc.h`.
  Если вы нарушите это правило, то скорее всего компиляция будет падать
  с ошибкой `error: ‘RequestHeader’ was not declared in this scope`.

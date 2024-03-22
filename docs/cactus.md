# Cactus

Cactus (кактус) - учебная библиотека сетевого программирования.
Разработана для второй части курса C++ в Школе Анализа Данных.

## Setup

1. Установите зависимости

```bash
$ sudo apt-get update -y
$ sudo apt-get install -y m4 protobuf-compiler libprotoc-dev libgflags-dev libgoogle-glog-dev
```

2. Перезапустите cmake

## API

### Scheduler

Чтобы работать с библиотекой, нужно создать класс `Scheduler` и
вызвать его метод `Run`.

```c++
#include <cactus/cactus.h>

int main() {
    cactus::Scheduler scheduler;

    scheduler.Run([&] {
        // Этот код выполняется в контексте файбера и может использовать
        // все классы и методы библиотеки.
        cactus::Yield();
    });
}
```

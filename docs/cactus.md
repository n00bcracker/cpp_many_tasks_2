# Cactus

Cactus (кактус) - учебная библиотека сетевого программирования.
Разработана для второй части курса С++ в Школе Анализа Данных.

## Setup

### Linux

1. Установите зависимости

```bash
$ sudo apt-get update -y
$ sudo apt-get install -y m4 protobuf-compiler libprotoc-dev libgflags-dev
```

2. Установите библиотеку folly:

```bash
$ git clone https://github.com/facebook/folly
$ cd folly
$ ./build.sh --no-tests --install-prefix=<абсолютный путь до репозитория курса>/tools/folly
```

3. Перезапустите cmake

### OSX или Windows

Библиотека не соберётся на вашей системе.

Рекомендуется использовать remote-toolchain в Clion, чтобы собирать код на Linux,
но редактировать код в Clion.

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

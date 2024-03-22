---
marp: true
theme: gaia
class: invert
---

# Network Programming in C++

---

# IP

* У каждой машины в сети есть IP-адрес.
* IPv4 адрес состоит из 32-бит.
* Текстовое представление IPv4 - `173.194.44.41`.
* IPv6 адрес состоит из 128-бит.
* Текстовое представление IPv6 - `2a00:1450:4010:c0d::71`.

---

# Interfaces

* Машина имеет множество интерфейсов.
* У интерфейса есть список адресов.
* lo - особый интерфейс с адресами `127.0.0.1` и `::1`

```shell
$ ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 ...
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 ...
    inet6 ::1/128 ...
3: wlp4s0: BROADCAST,MULTICAST,UP,LOWER_UP; mtu 1500 ...
    link/ether f4:8c:50:ed:96:f8 brd ff:ff:ff:ff:ff:ff
    inet6 2a02:6b8:0:40c:cc46:7228:928:1412/64 ...
```

---

# Port

* На одном адресе может "висеть" несколько процессов.
* Чтобы различать разные процессы используют порт
* port - число от 0 до 65536.
* По соглашению, у каждого протокола есть стандартный порт.
  * ssh 22
  * http 80
  * https 443

---

# (IP + Port) == SocketAddress

```c++
cactus::SocketAddress local_http{"127.0.0.1", 8080};

local_http.SetPort(8888);

// 0.0.0.0 имеет особый смысл.
cactus::SocketAddress local_http{"0.0.0.0", 80};

// 0 порт имеет особый смысл.
cactus::SocketAddress local_http{"127.0.0.1", 0};
```

---

# DNS

* Чтобы общаться в сети, нужно знать IP адрес пира.
* Работать напрямую с IP адресами не удобно.
* DNS - превращает имя в адрес.

```shell
$ dig ya.ru

;; ANSWER SECTION:
ya.ru.			198	IN	A	5.255.255.242
ya.ru.			198	IN	A	77.88.55.242
```

---

# UDP или TCP

* Есть два протокола для передачи данных в сети.
* UDP - без гарантии доставки.
* TCP - с гарантией доставки.

---

# UDP

* Передаёт пакеты размера до ~1000 байт.
* `SendTo(data, (ip, port))` посылает пакет в сеть.
* `Listen((ip, port)) + Recv() -> (data, (ip, port))` принимает.
* Никаких гарантий на доставку нет.
* DNS работает поверх UDP.

---

# TCP

* Передаёт поток байт, скрывая потери в сети.
* Чтобы использовать TCP, нужно _установить соединение_.
* TCP cоединение - **два** потока байт.
* Идентификатор соединения - 4 tuple. `(src_ip, src_port, dst_ip, dst_port)`

---

# TCP - открытие соединения

1. Сервер выполняет операцию `Listen((ip, port))`.
2. Сервер выполняет операцию `Accept()`
3. Клиент выполняет операцию `Dial((ip, port))`.
4. `Accept` и `Dial` завершаются, возвращая объект соединения.

---

# TCP - передача данных

* Два симметричных потока байт.

```
Write("pi")
Write("ng")
                                                        Read() = "ping"
                                                        Write("pong")
Read() = "pong"
                                                        Close()
Read() = ""
```

* Поток байт не знает про отдельные вызовы `Write`.
* Посылающая сторона может закрыть свой поток.

---

# cactus

* Чтобы написать что-то интересное, нужны библиотеки.
* В C++ с этим всё не так хорошо, как хотелось бы.
* Учебный проект. Как `xv6`, но для курса C++.

```c++
#include <cactus/cactus.h>

int main() {
    cactus::Scheduler sched;
    sched.Run([&] {
        // Code here.
    });
}
```

---

# View

* В сеть передаёт байты. Как представить их в C++?

---

# View API

* `View` - это невладеющая ссылка на участок памяти.
* `cactus::ConstView` и `cactus::MutableView`.
* `cactus::View` - функция конструктор.

```c++
int x = 42;
cactus::View(x); // MutableView
std::array<char, 4> ping = {'p', 'i', 'n', 'g'};
cactus::View(ping); // MutableView
std::vector<char> buf(1024);
cactus::View(buf); // MutableView
const Pod pod;
cactus::View(pod); // ConstView
```

---

# Reader

* `Reader` - входной поток байт.
* Пишет байты в `buf`. Возвращает, сколько байт удалось считать.
* Возвращает `0`, если входной поток завершился.

```c++
class IReader {
public:
    virtual size_t Read(MutableView buf) = 0;
};
```

---

# Writer && Closer

* `Write` пишет буффер целиком. (В отличии от `Read`).
* `Close` закрывает поток.

```c++
class IWriteCloser {
public:
    virtual void Write(ConstView buf) = 0;
    virtual void Close() = 0;
};
```

---

# Time

* Для того, чтобы писать сетевой код, нужно время.
* `Duration` - интервал времени, например 1 секунда.
* `TimePoint` - точка по времени.
* `TimePoint + Duration = TimePoint`
* `TimePoint - TimePoint = Duration`

```c++
using Duration = std::chrono::nanoseconds;
using TimePoint = std::chrono::steady_clock::time_point;
```

---

# DialTCP

* `DialTCP` устанавливает соединение.

```c++
std::unique_ptr<IConn> DialTCP(const cactus::SocketAddress& to);
```

---

# IConn

* Интерфейс TCP соединения.

```c++
class IConn {
public:
    virtual size_t Read(MutableView buf) = 0;
    virtual void Write(ConstView buf) = 0;
    virtual void Close() = 0;
    virtual const cactus::SocketAddress& RemoteAddress() const = 0;
    virtual const cactus::SocketAddress& LocalAddress() const = 0;
};
```

---

# Timeout

```c++
void Connect(const cactus::SocketAddress& addr) {
    cactus::TimeoutGuard guard{std::chrono::seconds(5)};
    try {
        auto conn = DialTCP(addr);
    } catch (const cactus::TimeoutException& ex) {
        LOG(INFO) << "DialTCP(" << addr << "): " << ex.what();
    }
}
```

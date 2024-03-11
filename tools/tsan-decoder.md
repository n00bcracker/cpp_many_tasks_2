# tsan-decoder

ThreadSanitizer не всегда преобразовывает адреса в памяти в имя файла
и номер строки, тогда его вывод может выглядеть следующим образом
(обратите внимание на `<null>` в конце строк):

    #0 std::forward_list<std::pair<int, int>, std::allocator<std::pair<int, int> > >::begin() const <null> (test_hash_map+0x0000000c531f)
    #1 ConcurrentHashMap<int, int, std::hash<int> >::rehash() <null> (test_hash_map+0x0000000c5000)
    #2 ConcurrentHashMap<int, int, std::hash<int> >::insert(int const&, int const&) <null> (test_hash_map+0x0000000bdf09)

Адрес можно преобразовать в имя файла и строку с помощью программы `addr2line`:

    addr2line -e <program-name> <address>

Выполнять данную команду каждый раз вручную неудобно, поэтому для этой
цели предлагается воспользоваться утилитой `tsan-decoder` (находится в
корне репозитория):

    ./<program-name> 2>&1 | tsan-decoder

Преобразованный вывод ThreadSanitizer-а будет выглядеть так:

    #0 std::forward_list<std::pair<int, int>, std::allocator<std::pair<int, int> > >::begin() const /usr/include/c++/7/bits/forward_list.h:700
    #1 ConcurrentHashMap<int, int, std::hash<int> >::rehash() /home/evgvir/shad/shad-cpp/hash-table/./concurrent_hash_map.h:174 (discriminator 2)
    #2 ConcurrentHashMap<int, int, std::hash<int> >::insert(int const&, int const&) /home/evgvir/shad/shad-cpp/hash-table/./concurrent_hash_map.h:44

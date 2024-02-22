# Subset-sum

Вам задан вектор чисел `std::vector<int64_t>`. Реализуйте функцию `FindEqualSumSubsets`, которая находит в нем
2 _непересекающихся_ подмножества с одинаковой суммой чисел. Подмножества выбираются с помощью индексов, иными словами
не должны совпадать индексы найденных элементов (входной вектор может содержать одинаковые числа).
Подмножества должны быть не пустыми.

### Подсказки и полезные ссылки
* Идея решения рассказана на семинаре.
* [std::thread](https://en.cppreference.com/w/cpp/thread/thread)
* [std::jthread](https://en.cppreference.com/w/cpp/thread/jthread)
* [std::atomic](https://en.cppreference.com/w/cpp/atomic/atomic)
* [std::atomic_flag](https://en.cppreference.com/w/cpp/atomic/atomic_flag)
* https://stackoverflow.com/questions/9685486/unordered-map-thread-safety
* https://en.cppreference.com/w/cpp/container#Thread_safety
* Для начала лучше написать однопоточную версию решения.
* При распараллеливании можно наткнуться на тот же false sharing, что и в задаче reduce.

### Ограничения
Время выполнения каждого бенчмарка не должно превышать 5 секунд.

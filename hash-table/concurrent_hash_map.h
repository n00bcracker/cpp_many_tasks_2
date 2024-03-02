#pragma once

#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <list>
#include <atomic>

template <class K, class V, class Hash = std::hash<K>>
class ConcurrentHashMap {
public:
    explicit ConcurrentHashMap(const Hash& hasher = {})
        : ConcurrentHashMap{kUndefinedSize, hasher} {
    }

    explicit ConcurrentHashMap(int expected_size, const Hash& hasher = {})
        : ConcurrentHashMap{expected_size, kDefaultConcurrencyLevel, hasher} {
    }

    ConcurrentHashMap(int expected_size, int expected_threads_count, const Hash& hasher = {})
        : table_(), hasher_(hasher) {
        if (expected_size != kUndefinedSize) {
            const size_t lists_cnt =
                ((((expected_size - 1) / kLoadFactor + 1) - 1) / expected_threads_count + 1) *
                expected_threads_count;
            table_ =
                std::vector<std::list<std::pair<K, V>>>(lists_cnt, std::list<std::pair<K, V>>{});
        } else {
            table_ = std::vector<std::list<std::pair<K, V>>>(4 * expected_threads_count,
                                                             std::list<std::pair<K, V>>{});
        }

        mutexes_ = std::vector<std::mutex>(table_.size());

        is_rehashing_.clear();
    }

    bool Insert(const K& key, const V& value) {
        const auto hash = hasher_(key);
        const auto mutex_idx = hash % mutexes_.size();
        bool res;
        bool need_rehash = false;
        {
            std::lock_guard lock(mutexes_[mutex_idx]);
            const auto list_idx = hash % table_.size();

            if ((res = InsertIntoList(list_idx, std::pair<K, V>(key, value)))) {
                ++size_;
            }

            if (size_ / table_.size() > kLoadFactor) {
                need_rehash = true;
            }
        }

        if (need_rehash && !is_rehashing_.test_and_set()) {
            Rehash();
            is_rehashing_.clear();
        }

        return res;
    }

    bool Erase(const K& key) {
        const auto hash = hasher_(key);
        const auto mutex_idx = hash % mutexes_.size();
        std::lock_guard lock(mutexes_[mutex_idx]);
        const auto list_idx = hash % table_.size();

        if (EraseFromList(list_idx, key)) {
            --size_;
            return true;
        } else {
            return false;
        }
    }

    void Clear() {
        for (int i = 0; i < std::ssize(mutexes_); ++i) {
            mutexes_[i].lock();
        }

        for (size_t i = 0; i < table_.size(); ++i) {
            table_[i].clear();
        }

        size_ = 0;

        for (int i = std::ssize(mutexes_) - 1; i >= 0; --i) {
            mutexes_[i].unlock();
        }
    }

    std::pair<bool, V> Find(const K& key) const {
        const auto hash = hasher_(key);
        const auto mutex_idx = hash % mutexes_.size();
        std::lock_guard lock(mutexes_[mutex_idx]);
        const auto list_idx = hash % table_.size();

        auto f_iter = FindPosInList(list_idx, key);
        const auto& list = table_[list_idx];
        if (f_iter != list.end() && f_iter->first == key) {
            return {true, f_iter->second};
        } else {
            return {false, {}};
        }
    }

    V At(const K& key) const {
        auto elem = Find(key);
        if (elem.first) {
            return elem.second;
        } else {
            throw std::out_of_range("WTF");
        }
    }

    size_t Size() const {
        return size_;
    }

    static constexpr auto kUndefinedSize = -1;

private:
    std::list<std::pair<K, V>>::const_iterator FindPosInList(size_t list_idx, const K& key) const {
        const auto& list = table_[list_idx];
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (it->first >= key) {
                return it;
            }
        }

        return list.end();
    }

    std::list<std::pair<K, V>>::iterator FindPosInList(size_t list_idx, const K& key) {
        auto& list = table_[list_idx];
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (it->first >= key) {
                return it;
            }
        }

        return list.end();
    }

    bool InsertIntoList(size_t list_idx, std::pair<K, V>&& elem) {
        auto& list = table_[list_idx];
        auto f_iter = FindPosInList(list_idx, elem.first);
        if (f_iter == list.end() || f_iter->first != elem.first) {
            list.emplace(f_iter, std::move(elem));
            return true;
        } else {
            return false;
        }
    }

    bool EraseFromList(size_t list_idx, const K& key) {
        auto& list = table_[list_idx];
        auto f_iter = FindPosInList(list_idx, key);
        if (f_iter != list.end() && f_iter->first == key) {
            list.erase(f_iter);
            return true;
        } else {
            return false;
        }
    }

    void Rehash() {
        for (int i = 0; i < std::ssize(mutexes_); ++i) {
            mutexes_[i].lock();
        }

        table_.resize(table_.size() * 2);

        for (size_t list_idx = 0; list_idx < table_.size(); ++list_idx) {
            auto& list = table_[list_idx];
            for (auto it = list.begin(); it != list.end();) {
                const auto hash = hasher_(it->first);
                const auto new_list_idx = hash % table_.size();
                if (new_list_idx != list_idx) {
                    auto elem = std::move(*it);
                    InsertIntoList(new_list_idx, std::move(elem));
                    if (it == list.begin()) {
                        list.erase(it);
                        it = list.begin();
                    } else {
                        auto remove_it = it;
                        --it;
                        list.erase(remove_it);
                        ++it;
                    }
                } else {
                    ++it;
                }
            }
        }

        for (int i = std::ssize(mutexes_) - 1; i >= 0; --i) {
            mutexes_[i].unlock();
        }
    }

    static constexpr auto kDefaultConcurrencyLevel = 8;
    static constexpr auto kLoadFactor = 2;

    std::vector<std::list<std::pair<K, V>>> table_;
    Hash hasher_;
    mutable std::vector<std::mutex> mutexes_;
    std::atomic_size_t size_;
    std::atomic_flag is_rehashing_;
};

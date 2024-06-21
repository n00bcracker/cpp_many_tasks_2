#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include "../hazard-ptr/hazard_ptr.h"

template <class K, class V>
class SyncMap {
public:
    SyncMap();
    ~SyncMap();
    bool Lookup(const K& key, V* value);
    bool Insert(const K& key, const V& value);

private:
    struct Snapshot {
        const std::shared_ptr<const std::unordered_map<K, V>> read_only;

        // Indicates that read_only snapshot may be incomplete and lookup should take lock.
        const bool dirty;
    };

    std::atomic<Snapshot*> snapshot_;
    std::shared_ptr<std::unordered_map<K, V>> mutable_map_ = nullptr;
    size_t operation_count_ = 0;
    std::mutex mutex_;
    static constexpr size_t kMaxOperationsCnt = 15;
};

template <class K, class V>
SyncMap<K, V>::SyncMap() {
    Snapshot* snapshot = new Snapshot{std::make_shared<const std::unordered_map<K, V>>(), false};
    snapshot_.store(snapshot);
}

template <class K, class V>
SyncMap<K, V>::~SyncMap() {
    delete snapshot_.load();
}

template <class K, class V>
bool SyncMap<K, V>::Lookup(const K& key, V* value) {
    bool need_lock = true;
    bool found = false;

    Snapshot* snapshot = Acquire(&snapshot_);
    if (snapshot->read_only->contains(key)) {
        *value = snapshot->read_only->at(key);
        found = true;
        need_lock = false;
    } else if (!snapshot->dirty) {
        need_lock = false;
    }
    Release();

    if (!need_lock) {
        return found;
    }

    std::lock_guard lock(mutex_);
    Snapshot* old_snapshot = snapshot_.load();
    if (old_snapshot->read_only->contains(key)) {
        *value = old_snapshot->read_only->at(key);
        return true;
    }

    if (!mutable_map_) {
        return false;
    }

    if (mutable_map_->contains(key)) {
        *value = mutable_map_->at(key);
        found = true;
    }

    if (++operation_count_ == kMaxOperationsCnt) {
        mutable_map_->insert(old_snapshot->read_only->begin(), old_snapshot->read_only->end());
        Snapshot* new_snapshot = new Snapshot{mutable_map_, false};
        snapshot_.store(new_snapshot);
        Retire(old_snapshot);
        mutable_map_ = nullptr;
        operation_count_ = 0;
    };

    return found;
}

template <class K, class V>
bool SyncMap<K, V>::Insert(const K& key, const V& value) {
    bool need_lock = true;

    Snapshot* snapshot = Acquire(&snapshot_);
    if (snapshot->read_only->contains(key)) {
        need_lock = false;
    }
    Release();

    if (!need_lock) {
        return false;
    }

    std::lock_guard lock(mutex_);
    Snapshot* old_snapshot = snapshot_.load();
    if (old_snapshot->read_only->contains(key)) {
        return false;
    }

    if (!mutable_map_) {
        mutable_map_ = std::make_shared<std::unordered_map<K, V>>();
        mutable_map_->emplace(key, value);
        Snapshot* new_snapshot = new Snapshot{old_snapshot->read_only, true};
        snapshot_.store(new_snapshot);
        Retire(old_snapshot);
        return true;
    } else {
        if (!mutable_map_->contains(key)) {
            mutable_map_->emplace(key, value);
            operation_count_ = 0;
            return true;
        } else {
            return false;
        }
    }
}
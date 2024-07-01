#pragma once

#include <sys/types.h>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include <functional>
#include <exception>

enum class TaskStates : u_char {
    Created = 0,
    Submitted = 1,
    Enqueued = 2,
    Completed = 3,
    Failed = 4,
    Canceled = 5
};

class TasksQueue;

class Task : public std::enable_shared_from_this<Task> {
public:
    virtual ~Task() {
    }

    virtual void Run() = 0;

    void AddDependency(std::shared_ptr<Task> dep);
    void AddDepended(std::shared_ptr<Task> dep);
    void AddTrigger(std::shared_ptr<Task> dep);
    void AddTriggered(std::shared_ptr<Task> dep);
    void SetTimeTrigger(std::chrono::system_clock::time_point at);

    std::chrono::system_clock::time_point GetStartTime() const;

    void TryToEnque();
    void Enque();
    bool IsReadyToEnque() const;

    bool IsReadyToExecute() const;

    // Task::Run() completed without throwing exception
    bool IsCompleted() const;

    // Task::Run() throwed exception
    bool IsFailed() const;

    // Task was Canceled
    bool IsCanceled() const;

    // Task either completed, failed or was Canceled
    bool IsFinished() const;

    void OnFinished();

    void SetError(std::exception_ptr exc);
    std::exception_ptr GetError();

    void Submit(std::shared_ptr<TasksQueue> queue);
    void Complete();
    void Fail();
    void Cancel();

    void Wait();

private:
    std::atomic<TaskStates> state_ = TaskStates::Created;
    std::weak_ptr<TasksQueue> queue_;
    std::mutex edit_task_;

    std::vector<std::shared_ptr<Task>> depended_;
    std::atomic_size_t dependecies_cnt_ = 0;
    bool has_dependencies_ = false;
    std::vector<std::shared_ptr<Task>> triggered_;
    bool has_triggers_ = false;
    std::atomic_flag triggers_activated_;
    std::chrono::system_clock::time_point start_at_;
    bool has_time_trigger_ = false;

    std::exception_ptr exc_ptr_;
};

class TasksQueue {
public:
    TasksQueue();
    void Push(std::shared_ptr<Task> task);
    std::shared_ptr<Task> Pop();
    void Close();

private:
    static bool Compare(const std::shared_ptr<Task>& task1, const std::shared_ptr<Task>& task2);
    std::vector<std::shared_ptr<Task>> queue_;
    std::mutex edit_queue_;
    std::condition_variable waiting_pop_;
    std::atomic_flag closed_;
};

template <class T>
class Future;

template <class T>
using FuturePtr = std::shared_ptr<Future<T>>;

// Used instead of void in generic code
struct Unit {};

class Executor {
public:
    Executor(uint32_t num_threads);
    ~Executor();

    void Submit(std::shared_ptr<Task> task);

    void StartShutdown();
    void WaitShutdown();

    template <class T>
    FuturePtr<T> Invoke(std::function<T()> fn);

    template <class Y, class T>
    FuturePtr<Y> Then(FuturePtr<T> input, std::function<Y()> fn);

    template <class T>
    FuturePtr<std::vector<T>> WhenAll(std::vector<FuturePtr<T>> all);

    template <class T>
    FuturePtr<T> WhenFirst(std::vector<FuturePtr<T>> all);

    template <class T>
    FuturePtr<std::vector<T>> WhenAllBeforeDeadline(std::vector<FuturePtr<T>> all,
                                                    std::chrono::system_clock::time_point deadline);

private:
    std::vector<std::thread> threads_;
    std::shared_ptr<TasksQueue> queue_;
};

std::shared_ptr<Executor> MakeThreadPoolExecutor(uint32_t num_threads);

template <class T>
class Future : public Task {
public:
    Future(std::function<T()> fn);
    T Get();
    void Run() override;

private:
    std::function<T()> func_;
    T result_;
};

template <class T>
Future<T>::Future(std::function<T()> fn) : func_(std::move(fn)) {
}

template <class T>
T Future<T>::Get() {
    Wait();
    if (IsCompleted()) {
        return result_;
    } else if (IsFailed()) {
        std::rethrow_exception(GetError());
    } else {
        return T();
    }
}

template <class T>
void Future<T>::Run() {
    result_ = func_();
}

template <class T>
FuturePtr<T> Executor::Invoke(std::function<T()> fn) {
    auto fut_ptr = std::make_shared<Future<T>>(fn);
    Submit(fut_ptr);
    return fut_ptr;
}

template <class Y, class T>
FuturePtr<Y> Executor::Then(FuturePtr<T> input, std::function<Y()> fn) {
    auto fut_ptr = std::make_shared<Future<Y>>(fn);
    fut_ptr->AddDependency(input);
    Submit(fut_ptr);
    return fut_ptr;
}

template <class T>
FuturePtr<std::vector<T>> Executor::WhenAll(std::vector<FuturePtr<T>> all) {
    auto fut_ptr = std::make_shared<Future<std::vector<T>>>([all]() -> std::vector<T> {
        std::vector<T> res;
        for (auto& fut : all) {
            res.emplace_back(fut->Get());
        }

        return res;
    });

    for (auto& fut : all) {
        fut_ptr->AddDependency(fut);
    }

    Submit(fut_ptr);
    return fut_ptr;
}

template <class T>
FuturePtr<T> Executor::WhenFirst(std::vector<FuturePtr<T>> all) {
    auto fut_ptr = std::make_shared<Future<T>>([all]() -> T {
        for (auto& fut : all) {
            if (fut->IsFinished()) {
                return fut->Get();
            }
        }

        return T();
    });

    for (auto& fut : all) {
        fut_ptr->AddTrigger(fut);
    }

    Submit(fut_ptr);
    return fut_ptr;
}

template <class T>
FuturePtr<std::vector<T>> Executor::WhenAllBeforeDeadline(
    std::vector<FuturePtr<T>> all, std::chrono::system_clock::time_point deadline) {
    auto fut_ptr = std::make_shared<Future<std::vector<T>>>([all]() -> std::vector<T> {
        std::vector<T> res;
        for (auto& fut : all) {
            if (fut->IsFinished()) {
                res.emplace_back(fut->Get());
            }
        }

        return res;
    });

    fut_ptr->SetTimeTrigger(deadline);
    Submit(fut_ptr);
    return fut_ptr;
}

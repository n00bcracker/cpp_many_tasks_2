#include "executor.h"
#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>

using namespace std::chrono_literals;

Executor::Executor(uint32_t num_threads)
    : queue_(std::make_shared<TasksQueue>())
    , timer_queue_(std::make_shared<TimerQueue>()) {
    for (size_t i = 0; i < num_threads + 1; ++i) {
        if (i < num_threads) {
            threads_.emplace_back([tasks_queue = queue_]() {
                std::shared_ptr<Task> task = nullptr;
                while ((task = tasks_queue->Pop())) {
                    if (task->IsCanceled()) {
                        continue;
                    }

                    try {
                        task->Run();
                        task->Complete();
                    } catch (...) {
                        task->SetError(std::current_exception());
                        task->Fail();
                    }
                }
            });
        } else {
            threads_.emplace_back([tasks_queue = timer_queue_]() {
                std::shared_ptr<Task> task = nullptr;
                while ((task = tasks_queue->Pop())) {
                    if (task->IsCanceled()) {
                        continue;
                    }

                    task->Enque();
                }
            });
        }
    }
}

std::shared_ptr<Executor> MakeThreadPoolExecutor(uint32_t num_threads) {
    return std::make_shared<Executor>(num_threads);
}

void Executor::Submit(std::shared_ptr<Task> task) {
    task->Submit(queue_);
    if (!(task->IsEnqued() || task->IsFinished()) && task->HasTimeTrigger()) {
        timer_queue_->Push(std::move(task));
    }
}

void Executor::StartShutdown() {
    queue_->Close();
    timer_queue_->Close();
}

void Executor::WaitShutdown() {
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

Executor::~Executor() {
    StartShutdown();
    WaitShutdown();
}

void Task::AddDependency(std::shared_ptr<Task> dep) {
    dep->AddDepended(shared_from_this());
    dependencies_.emplace_back(std::move(dep));
    has_dependencies_ = true;
}

void Task::AddDepended(std::shared_ptr<Task> dep) {
    std::lock_guard lock(edit_task_);
    if (!IsFinished()) {
        ++dep->dependecies_cnt_;
        depended_.emplace_back(dep);
    }
}

void Task::AddTrigger(std::shared_ptr<Task> dep) {
    dep->AddTriggered(shared_from_this());
    triggers_.emplace_back(std::move(dep));
    has_triggers_ = true;
}

void Task::AddTriggered(std::shared_ptr<Task> dep) {
    std::lock_guard lock(edit_task_);
    if (!IsFinished()) {
        triggered_.emplace_back(dep);
    } else {
        dep->triggers_activated_.test_and_set();
    }
}

void Task::SetTimeTrigger(std::chrono::system_clock::time_point at) {
    has_time_trigger_ = true;
    start_at_ = at;
}

bool Task::HasTimeTrigger() const {
    return has_time_trigger_;
}

std::chrono::system_clock::time_point Task::GetStartTime() const {
    return start_at_;
}

void Task::SetError(std::exception_ptr exc) {
    exc_ptr_ = std::move(exc);
}

std::exception_ptr Task::GetError() {
    if (IsFailed()) {
        return exc_ptr_;
    } else {
        return nullptr;
    }
}

bool Task::IsReadyToEnque() const {
    if (has_triggers_ && triggers_activated_.test()) {
        return true;
    }

    if (has_dependencies_ && !dependecies_cnt_) {
        return true;
    }

    if (!has_triggers_ && !has_dependencies_ && !has_time_trigger_) {
        return true;
    }

    return false;
}

bool Task::IsReadyToExecute() const {
    if (has_triggers_ && triggers_activated_.test()) {
        return true;
    }

    if (has_dependencies_ && !dependecies_cnt_) {
        return true;
    }

    if (has_time_trigger_ && std::chrono::system_clock::now() >= GetStartTime()) {
        return true;
    }

    if (!has_triggers_ && !has_dependencies_ && !has_time_trigger_) {
        return true;
    }

    return false;
}

bool Task::IsEnqued() const {
    return state_ == TaskStates::Enqueued;
}

bool Task::IsCompleted() const {
    return state_ == TaskStates::Completed;
}

bool Task::IsFailed() const {
    return state_ == TaskStates::Failed;
}

bool Task::IsCanceled() const {
    return state_ == TaskStates::Canceled;
}

bool Task::IsFinished() const {
    return state_ > TaskStates::Enqueued;
}

void Task::OnFinished() {
    std::lock_guard lock(edit_task_);
    for (auto& task : depended_) {
        if (!task.expired()) {
            auto task_ptr = task.lock();
            --task_ptr->dependecies_cnt_;
            task_ptr->TryToEnque();
        }
    }
    depended_.clear();

    for (auto& task : triggered_) {
        if (!task.expired()) {
            auto task_ptr = task.lock();
            task_ptr->triggers_activated_.test_and_set();
            task_ptr->TryToEnque();
        }
    }
    triggered_.clear();

    state_.notify_all();
}

void Task::TryToEnque() {
    if (state_ != TaskStates::Submitted) {
        return;
    }

    if (IsReadyToEnque()) {
        Enque();
    }
}

void Task::Enque() {
    TaskStates old_state = TaskStates::Submitted;
    if (!queue_.expired()) {
        if (state_.compare_exchange_strong(old_state, TaskStates::Enqueued)) {
            auto queue = queue_.lock();
            dependencies_.clear();
            triggers_.clear();
            queue->Push(shared_from_this());
        }
    }
}

void Task::Submit(std::shared_ptr<TasksQueue> queue) {
    queue_ = queue;
    TaskStates old_state = TaskStates::Created;
    if (state_.compare_exchange_strong(old_state, TaskStates::Submitted)) {
        TryToEnque();
    }
}

void Task::Complete() {
    state_ = TaskStates::Completed;
    OnFinished();
}

void Task::Fail() {
    state_ = TaskStates::Failed;
    OnFinished();
}

void Task::Cancel() {
    state_ = TaskStates::Canceled;
    OnFinished();
}

void Task::Wait() {
    TaskStates cur_state = state_;
    while (cur_state <= TaskStates::Enqueued) {
        state_.wait(cur_state);
        cur_state = state_;
    }
}

void TasksQueue::Push(std::shared_ptr<Task> task) {
    if (!closed_.test()) {
        std::lock_guard lock(edit_queue_);
        queue_.emplace_back(std::move(task));
        waiting_pop_.notify_one();
    } else {
        task->Cancel();
    }
}

std::shared_ptr<Task> TasksQueue::Pop() {
    std::unique_lock lock(edit_queue_);
    waiting_pop_.wait(lock, [this] {
        return closed_.test() || !queue_.empty();
    });

    if (queue_.empty()) {
        return nullptr;
    }

    std::shared_ptr<Task> task(std::move(queue_.front()));
    queue_.pop_front();
    return task;
}

void TasksQueue::Close() {
    if (!closed_.test_and_set()) {
        std::lock_guard lock(edit_queue_);
        for (auto& task : queue_) {
            task->Cancel();
        }

        waiting_pop_.notify_all();
    }
}

void TimerQueue::Push(std::shared_ptr<Task> task) {
    if (!closed_.test()) {
        std::lock_guard lock(edit_queue_);
        queue_.emplace_back(std::move(task));
        std::push_heap(queue_.begin(), queue_.end(), Compare);
        waiting_pop_.notify_one();
    } else {
        task->Cancel();
    }
}

std::shared_ptr<Task> TimerQueue::Pop() {
    std::unique_lock lock(edit_queue_);
    waiting_pop_.wait(lock, [this] {
        return closed_.test() || !queue_.empty();
    });

    if (queue_.empty()) {
        return nullptr;
    }

    waiting_pop_.wait_until(lock, queue_.front()->GetStartTime());

    std::pop_heap(queue_.begin(), queue_.end(), Compare);
    std::shared_ptr<Task> task(std::move(queue_.back()));
    queue_.pop_back();
    return task;
}

bool TimerQueue::Compare(const std::shared_ptr<Task>& task1, const std::shared_ptr<Task>& task2) {
    return task1->GetStartTime() > task2->GetStartTime();
}

void TimerQueue::Close() {
    if (!closed_.test_and_set()) {
        std::lock_guard lock(edit_queue_);
        for (auto& task : queue_) {
            task->Cancel();
        }

        waiting_pop_.notify_all();
    }
}
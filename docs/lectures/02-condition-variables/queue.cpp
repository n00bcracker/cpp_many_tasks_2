#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

class FixedQueue {
public:
    FixedQueue(int max_size)
        : max_size_(max_size)
    {}

    void Stop() {
        std::unique_lock guard(m_);
        stopped_ = true;
        not_full_.notify_all();
        not_empty_.notify_all();
    }

    bool Push(int x) {
        std::unique_lock guard(m_);
        not_full_.wait(guard, [this] {
            return stopped_ || q_.size() < max_size_;
        });
        if (stopped_) {
            return false;
        }
        q_.push(x);
        not_empty_.notify_one();
        return true;
    }

    std::optional<int> Pop() {
        std::unique_lock guard(m_);
        not_empty_.wait(guard, [this] {
            return stopped_ || !q_.empty();
        });
        if (stopped_) {
            return std::nullopt;
        }
        auto x = q_.front();
        q_.pop();
        not_full_.notify_one();
        return x;
    }

private:
    int max_size_;
    std::queue<int> q_;
    std::mutex m_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    bool stopped_ = false;
};

int main() {
    return 0;
}

#pragma once

#include <cstddef>
#include <ranges>
#include <concepts>

namespace details {
template <std::ranges::forward_range R>
    requires std::ranges::view<R>
class StrideView : public std::ranges::view_interface<StrideView<R>> {
    using RangeValueT = std::ranges::range_value_t<R>;
    using BaseIteratorType = std::ranges::iterator_t<R>;

    class Iterator {
    public:
        using iterator_category = std::iterator_traits<BaseIteratorType>::iterator_category;
        using value_type = RangeValueT;
        using difference_type = ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        Iterator() = default;

        Iterator(BaseIteratorType it, size_t pos, std::ranges::range_difference_t<R> step)
            : it_(it), pos_(pos), k_(step) {
        }

        Iterator& operator++()
            requires std::input_iterator<BaseIteratorType>
        {
            it_ = std::ranges::next(it_, k_);
            ++pos_;
            return *this;
        }

        void operator++(int)
            requires std::input_iterator<BaseIteratorType>
        {
            ++*this;
        }

        value_type& operator*() const
            requires std::input_iterator<BaseIteratorType>
        {
            return *it_;
        }

        value_type* operator->() const
            requires std::input_iterator<BaseIteratorType>
        {
            return &(*it_);
        }

        Iterator operator++(int)
            requires std::forward_iterator<BaseIteratorType>
        {
            Iterator it = *this;
            ++(*this);
            return it;
        }

        auto operator==(const Iterator& other) const
            requires std::forward_iterator<BaseIteratorType>
        {
            return it_ == other.it_ && pos_ == other.pos_;
        }

        Iterator& operator--()
            requires std::bidirectional_iterator<BaseIteratorType>
        {
            it_ = std::ranges::prev(it_, k_);
            --pos_;
            return *this;
        }

        Iterator operator--(int)
            requires std::bidirectional_iterator<BaseIteratorType>
        {
            Iterator it = *this;
            --(*this);
            return it;
        }

        Iterator& operator+=(int n)
            requires std::random_access_iterator<BaseIteratorType>
        {
            it_ = std::ranges::next(it_, k_ * n);
            pos_ += n;
            return *this;
        }

        Iterator operator+(int n) const
            requires std::random_access_iterator<BaseIteratorType>
        {
            Iterator it = *this;
            return it += n;
        }

        friend Iterator operator+(int n, const Iterator& it)
            requires std::random_access_iterator<BaseIteratorType>
        {
            return it + n;
        }

        Iterator& operator-=(int n)
            requires std::random_access_iterator<BaseIteratorType>
        {
            it_ = std::ranges::prev(it_, k_ * n);
            pos_ -= n;
            return *this;
        }

        Iterator operator-(int n) const
            requires std::random_access_iterator<BaseIteratorType>
        {
            Iterator it = *this;
            return it -= n;
        }

        difference_type operator-(const Iterator& other) const
            requires std::random_access_iterator<BaseIteratorType>
        {
            return pos_ - other.pos_;
        }

        value_type& operator[](size_t index) const
            requires std::random_access_iterator<BaseIteratorType>
        {
            Iterator it = (*this);
            return *(it + index);
        }

        auto operator<(const Iterator& other) const
            requires std::totally_ordered<BaseIteratorType>
        {
            return pos_ < other.pos_;
        }

        auto operator<=(const Iterator& other) const
            requires std::totally_ordered<BaseIteratorType>
        {
            return pos_ <= other.pos_;
        }

        auto operator>(const Iterator& other) const
            requires std::totally_ordered<BaseIteratorType>
        {
            return pos_ > other.pos_;
        }

        auto operator>=(const Iterator& other) const
            requires std::totally_ordered<BaseIteratorType>
        {
            return pos_ >= other.pos_;
        }

    private:
        BaseIteratorType it_;
        size_t pos_;
        std::ranges::range_difference_t<R> k_;
    };

public:
    StrideView() = default;

    StrideView(R base, std::ranges::range_difference_t<R> step)
        : base_{std::move(base)}, step_{step} {
    }

    auto begin() {
        return Iterator(std::ranges::begin(base_), 0, step_);
    }

    auto end() {
        size_t i = 0;
        auto it = std::ranges::begin(base_);
        auto tmp = it;

        while ((tmp = std::ranges::next(it, step_, std::ranges::end(base_))) !=
               std::ranges::end(base_)) {
            it = tmp;
            ++i;
        }

        it = std::ranges::next(it, step_);
        ++i;
        return Iterator(it, i, step_);
    }

    auto size() const
        requires std::ranges::sized_range<R>
    {
        auto n = std::ranges::size(base_);
        return n > 0 ? (n - 1) / step_ + 1 : 1;
    }

private:
    R base_;
    std::ranges::range_difference_t<R> step_;
};

template <class R>
StrideView(R&& base, std::ranges::range_difference_t<R>) -> StrideView<std::ranges::views::all_t<R>>;

template <std::integral Difference>
struct StrideAdaptorClosure {
    Difference step;

    template <std::ranges::viewable_range R>
    auto operator()(R&& r) {
        return StrideView{std::forward<R>(r), step};
    }

    template <std::ranges::viewable_range R>
    friend auto operator|(R&& r, StrideAdaptorClosure closure) {
        return StrideView{std::forward<R>(r), closure.step};
    }
};

struct StrideAdaptor {
    template <std::ranges::viewable_range R>
    auto operator()(R&& r, std::ranges::range_difference_t<R> step) {
        return StrideView{std::forward<R>(r), step};
    }

    template <std::integral T>
    auto operator()(T step) {
        return StrideAdaptorClosure<T>{step};
    }
};
}  // namespace details

details::StrideAdaptor Stride;

#pragma once

#include <cstddef>
#include <ranges>
#include <concepts>
#include <vector>

namespace details {
template <std::ranges::forward_range R>
    requires std::ranges::view<R>
class StrideView : public std::ranges::view_interface<StrideView<R>> {
    using RangeValueT = std::ranges::range_value_t<R>;
    using BaseIteratorType = std::ranges::iterator_t<R>;
    using VIterator = std::vector<BaseIteratorType>::iterator;

    class Iterator {
    public:
        using iterator_category = std::iterator_traits<BaseIteratorType>::iterator_category;
        using value_type = RangeValueT;
        using difference_type = ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        Iterator() = default;

        Iterator(VIterator it) : it_(it) {
        }

        Iterator& operator++()
            requires std::input_iterator<BaseIteratorType>
        {
            ++it_;
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
            return **it_;
        }

        value_type* operator->() const
            requires std::input_iterator<BaseIteratorType>
        {
            return &(**it_);
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
            return it_ == other.it_;
        }

        Iterator& operator--()
            requires std::bidirectional_iterator<BaseIteratorType>
        {
            --it_;
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
            it_ += n;
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
            it_ -= n;
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
            return it_ - other.it_;
        }

        value_type& operator[](size_t index) const
            requires std::random_access_iterator<BaseIteratorType>
        {
            return **(it_ + index);
        }

        auto operator<(const Iterator& other) const
            requires std::totally_ordered<BaseIteratorType>
        {
            return it_ < other.it_;
        }

        auto operator<=(const Iterator& other) const
            requires std::totally_ordered<BaseIteratorType>
        {
            return it_ <= other.it_;
        }

        auto operator>(const Iterator& other) const
            requires std::totally_ordered<BaseIteratorType>
        {
            return it_ > other.it_;
        }

        auto operator>=(const Iterator& other) const
            requires std::totally_ordered<BaseIteratorType>
        {
            return it_ >= other.it_;
        }

    private:
        VIterator it_;
    };

public:
    StrideView() = default;

    StrideView(R base, std::ranges::range_difference_t<R> step)
        : base_{std::move(base)}, step_{step} {

        auto it = std::ranges::begin(base_);
        iterators_.push_back(it);

        while ((it = std::ranges::next(it, step_, std::ranges::end(base_))) !=
               std::ranges::end(base_)) {
            iterators_.push_back(it);
        }
    }

    auto begin() {
        return Iterator(iterators_.begin());
    }

    auto end() {
        return Iterator(iterators_.end());
    }

    auto size() const
        requires std::ranges::sized_range<R>
    {
        return iterators_.size();
    }

private:
    R base_;
    std::ranges::range_difference_t<R> step_;
    std::vector<BaseIteratorType> iterators_;
};

template <class R>
StrideView(R&& base, std::ranges::range_difference_t<R>)
    -> StrideView<std::ranges::views::all_t<R>>;

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

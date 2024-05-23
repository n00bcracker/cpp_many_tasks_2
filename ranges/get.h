#pragma once

#include <concepts>
#include <cstddef>
#include <ranges>
#include <iterator>
#include <algorithm>
#include <utility>

namespace details {

template <class T>
struct Extractor {};

template <class R, class T>
struct Extractor<R T::*> {
    using Result = R;
    using Class = T;
};


template <std::ranges::forward_range R, typename FieldPtrType>
    requires std::ranges::view<R>
class GetFieldView : public std::ranges::view_interface<GetFieldView<R, FieldPtrType>> {
    using FieldType = Extractor<FieldPtrType>::Result;
    using BaseIteratorType = std::ranges::iterator_t<R>;

    class Iterator {
    public:
        using iterator_category = std::iterator_traits<BaseIteratorType>::iterator_category;
        using value_type = FieldType;
        using difference_type = ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        Iterator() : it_(nullptr), field_ptr_(nullptr) {
        }

        Iterator(BaseIteratorType it, FieldPtrType field_ptr) : it_(it), field_ptr_(field_ptr) {
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
            return (*it_).*field_ptr_;
        }

        value_type* operator->() const
            requires std::input_iterator<BaseIteratorType>
        {
            return &(it_->*field_ptr_);
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
            return it_ == other.it_ && field_ptr_ == other.field_ptr_;
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
            return *this += -n;
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
            return *(it_ + index).*field_ptr_;
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
        BaseIteratorType it_;
        FieldPtrType field_ptr_;
    };

public:
    GetFieldView() = default;

    GetFieldView(R base, FieldPtrType field_ptr) : base_{std::move(base)}, field_ptr_{field_ptr} {
    }

    auto begin() {
        return Iterator(std::ranges::begin(base_), field_ptr_);
    }

    auto end() {
        return Iterator(std::ranges::end(base_), field_ptr_);
    }

    auto size() const
        requires std::ranges::sized_range<R>
    {
        return std::ranges::size(base_);
    }

private:
    R base_;
    FieldPtrType field_ptr_;
};

template <class R, typename FieldPtrType>
GetFieldView(R&& base, FieldPtrType) -> GetFieldView<std::ranges::views::all_t<R>, FieldPtrType>;

template <typename FieldPtrType>
struct GetAdaptorClosure {
    FieldPtrType field_ptr;

    template <std::ranges::viewable_range R>
    auto operator()(R&& r) {
        return GetFieldView{std::forward<R>(r), field_ptr};
    }

    template <std::ranges::viewable_range R>
    friend auto operator|(R&& r, GetAdaptorClosure closure) {
        return GetFieldView{std::forward<R>(r), closure.field_ptr};
    }
};

template <auto field>
struct GetAdaptor : GetAdaptorClosure<decltype(field)> {

    GetAdaptor() : GetAdaptorClosure<decltype(field)>{field} {};
};

}  // namespace details

template <auto field>
details::GetAdaptor<field> Get;

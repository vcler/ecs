#pragma once

#include <boost/dynamic_bitset.hpp>
#include <format>
#include <iterator>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <print>

namespace ecs {
namespace detail {

template <class T>
class block {
public:
    using size_type = size_t;
    using value_type = T;
    using pointer = T *;

    block() = default;
    explicit block(size_type capacity);
    ~block() noexcept;
    block(block &&other);
    block &operator=(block &&other);

    template <class U>
    size_type push_back(U &&value)
    {
        if (size_ == capacity_)
            throw std::length_error("block is full");

        assert(free_ != nullptr);

        const size_t pos = std::distance(data_, to_data(free_));
        const pointer next = *free_;
        std::construct_at(to_data(free_), std::move(value));
        free_ = to_meta(next);
        ++size_;

        return pos;
    }

    template <class U>
    size_type push_back(const U &value)
    {
        if (size_ == capacity_)
            throw std::length_error("block is full");

        assert(free_ != nullptr);

        const size_t pos = std::distance(data_, to_data(free_));
        const pointer next = *free_;
        std::construct_at(to_data(free_), value);
        free_ = to_meta(next);
        ++size_;

        return pos;
    }

    template <class... Args>
    size_type emplace_back(Args &&...args)
    {
        if (size_ == capacity_)
            throw std::length_error("block is full");

        assert(free_ != nullptr);

        const size_t pos = std::distance(data_, to_data(free_));
        const pointer next = *free_;
        std::construct_at(to_data(free_), std::forward<Args>(args)...);
        free_ = to_meta(next);
        ++size_;

        return pos;
    }

    bool contains(pointer pos) const noexcept
    {
        if (data_ == nullptr)
            return false;

        return pos >= data_ && pos < data_ + capacity_;
    }

    void erase(pointer pos)
    {
        if (pos < data_ || pos >= data_ + capacity_)
            throw std::out_of_range(
                    std::format("{} is invalid", std::distance(data_, pos)));

        std::destroy_at(pos);

        *to_meta(pos) = to_data(free_);
        free_ = to_meta(pos);
        --size_;
    }

    void erase(size_type pos)
    {
        erase(data_ + pos);
    }


    T &at(size_type pos)
    {
        assert(pos < capacity_);
        return *(data_ + pos);
    }

    const T &at(size_type pos) const
    {
        assert(pos < capacity_);
        return *(data_ + pos);
    }

    friend void swap(block &lhs, block &rhs)
    {
        using std::swap;
        swap(lhs.capacity_, rhs.capacity_);
        swap(lhs.size_, rhs.size_);
        swap(lhs.data_, rhs.data_);
        swap(lhs.free_, rhs.free_);
    }

    void clear()
    {
        if (free_ == nullptr) {
            const auto end = data_ + capacity_;
            for (auto it = data_; it != end; ++it)
                erase(it);
        } else {
            boost::dynamic_bitset skip(capacity_);
            auto free = free_;
            for ( ; free != nullptr; free = to_meta(*free))
                skip.set(std::distance(data_, to_data(free)));

            for (size_type i = 0; i < capacity_; ++i) {
                if (skip.at(i) == false)
                    erase(i);
            }
        }
    }

    bool has_space() const noexcept { return size_ < capacity_; }
    size_type space() const noexcept { return capacity_ - size_; }

private:
    void print_free_list()
    {
        auto free = free_;
        for ( ; free != nullptr; free = to_meta(*free))
            std::print("{} -> ", std::distance(data_, to_data(free)));
        std::println();
    }


    T **to_meta(T *pos) { return reinterpret_cast<T **>(pos); }
    T *to_data(T **pos) { return reinterpret_cast<T *>(pos); }

    size_type capacity_ = 0;
    size_type size_ = 0;
    T *data_ = nullptr;
    T **free_ = nullptr;
};

template <class T>
block<T>::block(size_type capacity)
    : capacity_(capacity)
    , data_(static_cast<T *>(::operator new (capacity_ * sizeof(T))))
    , free_(to_meta(data_))
{
    for (auto i = 0uz; i < capacity_ - 1; ++i)
        *to_meta(data_ + i) = data_ + i + 1;
    *to_meta(data_ + capacity_ - 1) = nullptr;
}

template <class T>
block<T>::~block() noexcept
{
    if (data_ == nullptr)
        return;

    clear();

    ::operator delete (data_);
}

template <class T>
block<T>::block(block &&other)
{
    swap(*this, other);
}

template <class T>
block<T> &block<T>::operator=(block &&other)
{
    swap(*this, other);
    return *this;
}

} // namespace detail
} // namespace ecs


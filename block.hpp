#pragma once

#include <boost/dynamic_bitset.hpp>
#include <format>
#include <iterator>
#include <memory>
#include <stdexcept>

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
        if (free_ == nullptr)
            throw std::length_error("block is full");

        const size_t pos = std::distance(data_, to_data(free_));
        const pointer next = *free_;
        std::construct_at(to_data(free_), std::move(value));
        free_ = to_meta(next);

        return pos;
    }

    template <class U>
    size_type push_back(const U &value)
    {
        if (free_ == nullptr)
            throw std::length_error("block is full");

        const size_t pos = std::distance(data_, to_data(free_));
        const pointer next = *free_;
        std::construct_at(to_data(free_), value);
        free_ = next;

        return pos;
    }

    template <class... Args>
    size_type emplace_back(Args &&...args)
    {
        if (free_ == nullptr)
            throw std::length_error("block is full");

        const size_t pos = std::distance(data_, to_data(free_));
        const pointer next = *free_;
        std::construct_at(to_data(free_), std::forward<Args>(args)...);
        free_ = next;

        return pos;
    }

    void erase(size_type pos)
    {
        if (pos >= capacity_)
            throw std::out_of_range(std::format("{} >= {}", pos, capacity_));

        std::destroy_at(data_ + pos);
        *to_meta(data_ + pos) = free_;
        free_ = data_ + pos;
    }

    void erase(pointer pos)
    {
        if (pos < data_ || pos >= data_ + capacity_)
            throw std::out_of_range("pos out of range");

        std::destroy_at(pos);
        *static_cast<T **>(pos) = free_;
        free_ = pos;
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
        swap(lhs.data_, rhs.data_);
        swap(lhs.free_, rhs.free_);
    }

    void clear()
    {
        if (free_ == nullptr) {
            std::destroy(data_, data_ + capacity_);
        } else {
            boost::dynamic_bitset skip(capacity_);
            for ( ; free_ != nullptr; free_ = to_meta(*free_))
                skip.set(std::distance(data_, to_data(free_)));

            for (size_type i = 0; i < capacity_; ++i) {
                if (skip.at(i) == false)
                    std::destroy_at(data_ + i);
            }
        }
    }

    bool has_space() const noexcept { return free_ != nullptr; }

private:
    T **to_meta(T *pos) { return reinterpret_cast<T **>(pos); }
    T *to_data(T **pos) { return reinterpret_cast<T *>(pos); }

    size_type capacity_ = 0;
    T *data_ = nullptr;
    T **free_ = nullptr;
};

template <class T>
block<T>::block(size_type capacity)
    : capacity_(capacity)
    , data_(static_cast<T *>(::operator new (capacity_ * sizeof(T))))
    , free_(to_meta(data_))
{
    for (auto i = 0uz; i < capacity - 1; ++i)
        *to_meta(data_ + i) = data_ + i + 1;
    *to_meta(data_ + capacity - 1) = nullptr;
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


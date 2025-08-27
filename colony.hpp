#pragma once

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "block.hpp"


template <class Colony>
class colony_iterator;

template <class T>
class colony {
    static_assert(sizeof(T) >= sizeof(void *)
            && "value type must have at least word size");
public:
    using size_type = size_t;
    using value_type = T;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = colony_iterator<colony>;
    using const_iterator = colony_iterator<const colony>;

    colony() = default;

    template <class U>
    size_type push_back(const U &value);

    template <class U>
    size_type push_back(U &&value);

    template <class... Args>
    size_type emplace_back(Args &&...args);

    void erase(size_type pos);
    iterator erase(iterator it);

    void clear();

    reference at(size_type pos);
    const_reference at(size_type pos) const;
    size_type next(size_type pos) const noexcept;

    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    iterator end() noexcept;
    const_iterator end() const noexcept;

    size_type capacity() const noexcept;
    size_type size() const noexcept;

private:
    static constexpr size_type block_size = 32;

    using block_type = block<T>;
    using block_container = std::vector<block_type>;

    size_type offset(block_type &block) const noexcept;
    size_type block_pos(size_type offset) const noexcept;
    block_type &get_free_block();

    size_type size_ = 0;
    block_container blocks_{};
    boost::dynamic_bitset<> used_{};
};


template <class Colony>
class colony_iterator {
    using size_type = Colony::size_type;
    using value_type = Colony::value_type;

public:
    ~colony_iterator() = default;

    colony_iterator(Colony *colony, size_type pos)
        : colony_(colony)
        , pos_(pos)
    {
    }

    colony_iterator &operator++() noexcept
    {
        pos_ = colony_->next(pos_);
        return *this;
    }

    colony_iterator operator++(int) noexcept
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    value_type &operator*()
    {
        return colony_->at(pos_);
    }

    value_type &operator*() const
    {
        return colony_->at(pos_);
    }

    bool operator==(const colony_iterator &other) const noexcept
    {
        return colony_ == other.colony_ && pos_ == other.pos_;
    }

    size_type pos() const noexcept { return pos_; }

private:
    Colony *colony_;
    size_type pos_;
};


template <class T>
template <class U>
colony<T>::size_type colony<T>::push_back(const U &value)
{
    auto &block = get_free_block();
    auto pos = block.push_back(value);
    pos += offset(block);
    used_.set(pos);
    ++size_;
    return pos;
}

template <class T>
template <class U>
colony<T>::size_type colony<T>::push_back(U &&value)
{
    auto &block = get_free_block();
    auto pos = block.push_back(std::move(value));
    pos += offset(block);
    used_.set(pos);
    ++size_;
    return pos;
}

template <class T>
template <class... Args>
colony<T>::size_type colony<T>::emplace_back(Args &&...args)
{
    auto &block = get_free_block();
    auto pos = block.emplace_back(std::forward<Args>(args)...);
    pos += offset(block);
    used_.set(pos);
    ++size_;
    return pos;
}

template <class T>
void colony<T>::clear()
{
    for (auto &block : blocks_)
        block.clear();
    size_ = 0;
}

template <class T>
void colony<T>::erase(size_type pos)
{
    if (used_.at(pos) == false) {
        return;
    }

    blocks_.at(block_pos(pos)).erase(pos % block_size);
    used_.flip(pos);
    --size_;
}

template <class T>
colony<T>::iterator colony<T>::erase(iterator it)
{
    erase(it.pos());
    return ++it;
}

template <class T>
colony<T>::size_type colony<T>::offset(block_type &block) const noexcept
{
    return (&block - blocks_.data()) * block_size;
}

template <class T>
colony<T>::size_type colony<T>::block_pos(size_type offset) const noexcept
{
    return offset / block_size;
}

template <class T>
colony<T>::reference colony<T>::at(size_type pos)
{
    if (used_.at(pos) == false)
        throw std::out_of_range("invalid index");

    const auto n = block_pos(pos);
    return blocks_.at(n).at(pos % block_size);
}

template <class T>
colony<T>::const_reference colony<T>::at(size_type pos) const
{
    if (used_.at(pos) == false)
        throw std::out_of_range("invalid index");

    const auto n = block_pos(pos);
    return blocks_.at(n).at(pos - n);
}

template <class T>
colony<T>::size_type colony<T>::next(size_type pos) const noexcept
{
    return used_.find_next(pos);
}

template <class T>
colony<T>::block_type &colony<T>::get_free_block()
{
    if (size_ == capacity()) {
        blocks_.emplace_back(block_size);
        used_.resize(size_ + block_size);
        return blocks_.back();
    }

    auto block = std::ranges::find_if(blocks_,
            [](const auto &block) { return block.has_space(); });

    assert(block != std::end(blocks_));

    return *block;
}

template <class T>
colony<T>::iterator colony<T>::begin() noexcept
{
    return iterator(this, used_.find_first());
}

template <class T>
colony<T>::const_iterator colony<T>::begin() const noexcept
{
    return const_iterator(this, used_.find_first());
}

template <class T>
colony<T>::iterator colony<T>::end() noexcept
{
    return iterator(this, boost::dynamic_bitset<>::npos);
}

template <class T>
colony<T>::const_iterator colony<T>::end() const noexcept
{
    return const_iterator(this, boost::dynamic_bitset<>::npos);
}

template <class T>
colony<T>::size_type colony<T>::capacity() const noexcept
{
    return block_size * blocks_.size();
}

template <class T>
colony<T>::size_type colony<T>::size() const noexcept
{
    return size_;
}


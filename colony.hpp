#pragma once

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "block.hpp"

template <class T>
class colony {
    class colony_iterator;
public:
    using size_type = size_t;
    using reference = T &;
    using const_reference = const T &;
    using iterator = colony_iterator;
    using const_iterator = const iterator;

    colony() = default;

    template <class U>
    void push_back(const U &value);

    template <class U>
    void push_back(U &&value);

    template <class... Args>
    void emplace_back(Args &&...args);

    void clear();

    reference at(size_type pos);
    const_reference at(size_type pos) const;

    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    iterator end() noexcept;
    const_iterator end() const noexcept;

    size_type capacity() const noexcept { return std::pow(2uz, blocks_.size()) - 1uz; }
    size_type size() const noexcept { return size_; }

private:
    static constexpr size_type block_size = 32;

    using block_container = std::vector<block<T>>;
    using block_iterator = block_container::iterator;

    class colony_iterator {
    public:
        colony_iterator(colony *colony, T *pos)
            : colony_(colony)
            , pos_(pos)
        {
        }

        iterator &operator++()
        {

            return *this;
        }

        iterator operator++(int)
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        bool operator==(const iterator &other)
        {
            return colony_ == other.colony_ && pos_ == other.pos_;
        }

    private:
        colony *colony_;
        T *pos_;
    };

    size_type offset(block_iterator block) const noexcept;
    size_type block_pos(size_type offset) const noexcept;
    block_iterator get_free_block();

    size_type size_ = 0;
    block_container blocks_{};
    boost::dynamic_bitset<> used_{};
};

template <class T>
template <class U>
void colony<T>::push_back(const U &value)
{
    auto block = get_free_block();
    const auto pos = block->push_back(value);
    used_.set(offset(block) + pos);
    ++size_;
}

template <class T>
template <class U>
void colony<T>::push_back(U &&value)
{
    auto block = get_free_block();
    const auto pos = block->push_back(std::move(value));
    used_.set(offset(block) + pos);
    ++size_;
}

template <class T>
template <class... Args>
void colony<T>::emplace_back(Args &&...args)
{
    auto block = get_free_block();
    const auto pos = block->push_back(std::forward<Args>(args)...);
    used_.set(offset(block) + pos);
    ++size_;
}

template <class T>
void colony<T>::clear()
{
    for (auto &block : blocks_)
        block.clear();
}

template <class T>
colony<T>::size_type colony<T>::offset(block_iterator block) const noexcept
{
    return std::pow(2uz, block - std::begin(blocks_)) - 1uz;
}

template <class T>
colony<T>::size_type colony<T>::block_pos(size_type offset) const noexcept
{
    return std::log2(offset + 1uz);
}

template <class T>
colony<T>::reference colony<T>::at(size_type pos)
{
    if (pos >= capacity())
        throw std::out_of_range(std::format("{} >= {}", pos, capacity()));

    const auto n = block_pos(pos);
    return blocks_.at(n).at(pos - n);
}

template <class T>
colony<T>::const_reference colony<T>::at(size_type pos) const
{
    if (pos >= capacity())
        throw std::out_of_range(std::format("{} >= {}", pos, capacity()));

    const auto n = block_pos(pos);
    return blocks_.at(n).at(pos - n);
}

template <class T>
colony<T>::block_iterator colony<T>::get_free_block()
{
    if (size_ == capacity()) {
        blocks_.emplace_back(size_ + 1uz);
        used_.resize(2uz * size_  + 1uz);
        return blocks_.end() - 1uz;
    }

    auto block = std::ranges::find_if(blocks_,
            [](const auto &block) { return block.has_space(); });

    assert(block != std::end(blocks_));

    return block;
}


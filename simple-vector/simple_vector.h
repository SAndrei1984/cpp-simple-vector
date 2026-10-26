#pragma once

#include <cassert>
#include <initializer_list>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>
#include "array_ptr.h"
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>
#include <type_traits>


using namespace std;

struct ReserveProxyObj {
    explicit ReserveProxyObj(size_t capacity_to_reserve)
        : capacity(capacity_to_reserve) {
    }
    size_t capacity;
};

template <typename Type>
class SimpleVector {

public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    SimpleVector(SimpleVector&& other) {
        simple_vector_ = std::move(other.simple_vector_);
        sv_size_ = std::exchange(other.sv_size_, 0);
        sv_capacity_ = std::exchange(other.sv_capacity_, 0);
    }

    SimpleVector(const SimpleVector& other)
        : SimpleVector(other.GetCapacity(), Type{}) {
        std::copy(other.begin(), other.end(), simple_vector_.Get());
        Resize(other.GetSize());
    }

    explicit SimpleVector(size_t size)
        : SimpleVector(size, Type{}) {
    }

    SimpleVector(size_t size, const Type& value)
        : simple_vector_(size)
        , sv_size_(size)
        , sv_capacity_(size) {
        std::fill(simple_vector_.Get(), simple_vector_.Get() + sv_size_, value);
    }

    SimpleVector(size_t size, Type&& value)
        : simple_vector_(size)
        , sv_size_(size)
        , sv_capacity_(size) {
        std::generate(simple_vector_.Get(), simple_vector_.Get() + size, [&value]() mutable { 
                            return std::move(value); 
                        });
    }

    SimpleVector(std::initializer_list<Type> init)
        : simple_vector_(init.size())
        , sv_size_(init.size())
        , sv_capacity_(init.size()) {
        std::copy(init.begin(), init.end(), simple_vector_.Get());
    }

    SimpleVector (ReserveProxyObj size)
        : simple_vector_(size.capacity)
        , sv_size_(0)
        , sv_capacity_(size.capacity) {
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (rhs.IsEmpty()) {
            Clear();
        } else {
            if(*this != rhs) {
                SimpleVector buffer(rhs);
                swap(buffer);
            }
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if(*this != rhs) {
            simple_vector_= std::move(rhs.simple_vector_);
            sv_size_ = std::exchange(rhs.sv_size_, 0);
            sv_capacity_ = std::exchange(rhs.sv_capacity_, 0);
        }
        return *this;
    }

    size_t GetSize() const noexcept {
        return sv_size_;
    }

    size_t GetCapacity() const noexcept {
        return sv_capacity_;
    }

    bool IsEmpty() const noexcept {
        return sv_size_ == 0 ? true : false;
    }

    Type& operator[](size_t index) noexcept {
        return simple_vector_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        return simple_vector_[index];
    }

    Type& At(size_t index) {
        if(index >= sv_size_) {
            throw std::out_of_range("Requested vector item index is out of vector size range");
        }
        return simple_vector_[index];
    }

    const Type& At(size_t index) const {
        if(index >= sv_size_) {
            throw std::out_of_range("Requested vector item index is out of vector size range");
        }
        return simple_vector_[index];
    }

    void Clear() noexcept {
        sv_size_ = 0;
    }

    void Resize(size_t new_size) {
        if(new_size > sv_capacity_) {
            size_t new_capacity = std::max(sv_capacity_ * 2, new_size);
            ArrayPtr<Type> arr_with_new_capacity(new_capacity);
            std::move(simple_vector_.Get(), simple_vector_.Get() + sv_size_, arr_with_new_capacity.Get());
            for (size_t i = sv_size_; i < new_size; ++i) {
                arr_with_new_capacity[i] = Type();
            }
            simple_vector_.swap(arr_with_new_capacity);
            sv_capacity_ = new_capacity;
        } else if(new_size > sv_size_) {
            if constexpr (std::is_move_constructible_v<Type>)  {
                for (size_t i = sv_size_; i < new_size; ++i) {
                    simple_vector_[i] = Type();
                }
            }  else if(std::is_copy_constructible_v<Type>) {
                std::fill(simple_vector_.Get() + sv_size_, simple_vector_.Get() + new_size, Type{});
            }
        }
        sv_size_ = new_size;
    }

    Iterator begin() noexcept {
        return simple_vector_.Get();
    }

    Iterator end() noexcept {
        return simple_vector_.Get() + sv_size_;
    }

    ConstIterator begin() const noexcept {
        return simple_vector_.Get();
    }

    ConstIterator end() const noexcept {
        return simple_vector_.Get() + sv_size_;
    }

    ConstIterator cbegin() const noexcept {
        return simple_vector_.Get();
    }

    ConstIterator cend() const noexcept {
        return simple_vector_.Get() + sv_size_;
    }

    void PushBack(const Type& item) {
        if(sv_size_ >= sv_capacity_) {
            size_t new_capacity = sv_size_ == 0 ? 1 : 2 * sv_capacity_;
            Reserve(new_capacity);
        }
        *end() = item;
        ++sv_size_;
    }

    void PushBack(Type&& item) {
        if(sv_size_ >= sv_capacity_) {
            size_t new_capacity = sv_size_ == 0 ? 1 : 2 * sv_capacity_;
            Reserve(new_capacity);
        }
        *end() = std::move(item);
        ++sv_size_;
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        Iterator iter = const_cast<Iterator>(pos);
        assert(iter >= begin() && iter <= end());
        if(sv_size_ >= sv_capacity_){
            size_t new_capacity = sv_size_ == 0 ? 1 : 2 * sv_capacity_;
            Reserve(new_capacity);
        }
        std::copy_backward(iter, end(), end() + 1);
        *iter = value;
        ++sv_size_;
        return iter;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        Iterator iter = const_cast<Iterator>(pos);
        assert(iter >= begin() && iter <= end());
        size_t index = pos - begin();
        if(sv_size_ >= sv_capacity_){
            size_t new_capacity = sv_size_ == 0 ? 1 : 2 * sv_capacity_;
            Reserve(new_capacity);
        }
        for(auto it = sv_size_; it != index; --it) {
            simple_vector_[it + 1] = std::move(simple_vector_[it]);
        }
        simple_vector_[index] = std::move(value);
        ++sv_size_;
        return &simple_vector_[index];
    }

    Iterator Erase(ConstIterator pos) {
        auto iter = const_cast<Iterator>(pos);
        size_t index = iter - simple_vector_.Get();
        assert(iter >= begin() && iter <= end());
        if constexpr (std::is_move_constructible_v<Type>) {
            for (size_t i = index; i < sv_size_ - 1; ++i) {
                simple_vector_[i] = std::move(simple_vector_[i + 1]);
            }
        } else if(std::is_copy_constructible_v<Type>) {
                std::copy(iter + 1, end(), iter); 
        }
        --sv_size_;
        return iter;
    }

    void PopBack() noexcept {
        assert(IsEmpty() != true);
        --sv_size_;
    }

    void swap(SimpleVector& other) noexcept {
        simple_vector_.swap(other.simple_vector_);
        size_t buf_sv_size_= other.sv_size_;
        size_t buf_sv_capacity_ = other.sv_capacity_;
        other.sv_size_ = sv_size_;
        other.sv_capacity_ = sv_capacity_;
        sv_size_ = buf_sv_size_;
        sv_capacity_ = buf_sv_capacity_;
    }

    void Reserve(size_t new_capacity){
        if (new_capacity > sv_capacity_) {
            ArrayPtr<Type> arr_with_new_capacity(new_capacity);
            std::move(simple_vector_.Get(), simple_vector_.Get() + sv_size_, arr_with_new_capacity.Get());
            for (size_t i = sv_size_; i < new_capacity; ++i) {
                arr_with_new_capacity[i] = Type();
            }
            simple_vector_.swap(arr_with_new_capacity);
            sv_capacity_ = new_capacity;
        }
    }

private:
    ArrayPtr<Type> simple_vector_;
    size_t sv_size_= 0;
    size_t sv_capacity_ = 0;

};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs.GetSize() == rhs.GetSize() && std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

    return !(lhs < rhs);
}

inline ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}
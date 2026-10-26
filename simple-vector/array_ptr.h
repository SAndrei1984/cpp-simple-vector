#pragma once
#include <cassert>
#include <cstdlib>
#include <iterator>
#include <utility>

template <typename Type>
class ArrayPtr {
public:

    ArrayPtr() = default;

    explicit ArrayPtr(size_t size) {
        raw_ptr_ = (size == 0 ? nullptr : new Type[size]);
    }

    explicit ArrayPtr(Type* raw_ptr) noexcept {
        raw_ptr_ = raw_ptr;
    }

    ArrayPtr(const ArrayPtr&) = delete;

    ArrayPtr(ArrayPtr&& other) {
        raw_ptr_ = std::exchange(other.raw_ptr_, nullptr);
    }

    ~ArrayPtr() {
       delete[]  Release();
    }

    ArrayPtr& operator=(const ArrayPtr&) = delete;

    ArrayPtr& operator=(ArrayPtr&& other) {
        std::swap(raw_ptr_, other.raw_ptr_);
    return *this;
}

    [[nodiscard]] Type* Release() noexcept {
        Type* temp = raw_ptr_;
        raw_ptr_ = nullptr;
        return temp;
    }

    Type& operator[](size_t index) noexcept {
        return *(raw_ptr_ + index);
    }

    const Type& operator[](size_t index) const noexcept {
        return *(raw_ptr_ + index);
    }

    explicit operator bool() const {
        if(raw_ptr_ != nullptr) {
            return true;
        } 
        return false;
    }

    Type* Get() const noexcept {
        return raw_ptr_;
    }
    
    void swap(ArrayPtr& other) noexcept {
        Type* buff = other.raw_ptr_;
        other.raw_ptr_ = raw_ptr_;
        raw_ptr_ = buff;
    }

private:
    Type* raw_ptr_ = nullptr;
};
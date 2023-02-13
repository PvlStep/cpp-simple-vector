#pragma once

#include "array_ptr.h"

#include <cassert>
#include <algorithm>
#include <initializer_list>
#include <utility>
#include <cstddef>

//using namespace std::literals;

class ReserveProxyObject {
public:
    explicit ReserveProxyObject(size_t capacity) : reserve_(capacity) {}

    size_t Return_Capasity() {
        return reserve_;
    }

private:
    size_t reserve_;
};

ReserveProxyObject Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObject(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : size_(size), capacity_(size), elements_(size)
    {
        std::fill(elements_.Get(), elements_.Get() + size, Type{});
    }


    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) : size_(size), capacity_(size), elements_(size)
    {
        std::fill(elements_.Get(), elements_.Get() + size, value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) {
        Assign(init.begin(), init.end());
    }

    SimpleVector(const SimpleVector& other) {
        Assign(other.begin(), other.end());
    }

    SimpleVector(SimpleVector&& other) noexcept{
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
        elements_ = std::move(other.elements_);
    }

    explicit SimpleVector(ReserveProxyObject reserve_capacity)  {
        Reserve(reserve_capacity.Return_Capasity());
    }

    SimpleVector& operator=(const SimpleVector& other) {
        if (this != &other) {
            SimpleVector temp(other);
            swap(temp);
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& other) noexcept {
        if (this != &other) {
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        elements_.swap(other.elements_);
        }
        return *this;
    }

    void PushBack(const Type& value) {
        size_t new_size = size_ + 1u;
        if (new_size > capacity_) {
            size_t new_capacity = (capacity_ != 0) ? std::max(new_size, 2u * capacity_) : 1;
            MoveElements(new_capacity);
        }
        elements_[size_] = value;
        size_ = new_size;
    }

    void PushBack(Type&& value) {
        size_t new_size = size_ + 1u;
        if (new_size > capacity_) {
            size_t new_capacity = (capacity_ != 0) ? std::max(new_size, 2u * capacity_) : 1;
            ArrayPtr<Type> new_elements(new_capacity);
            size_t elements_count = std::min(size_, new_capacity);
            std::move(elements_.Get(), elements_.Get() + elements_count, new_elements.Get());
            elements_.swap(new_elements);
            capacity_ = new_capacity;
        }
        elements_[size_] = std::move(value);
        size_ = new_size;
    }

    void PopBack() noexcept {
        assert(size_ != 0u);
        --size_;
    }

    Iterator Insert(ConstIterator position, const Type& value) {
        assert(position >= begin() && position <= end());
        size_t distance_ = position - elements_.Get();
        //assert(distance_ <= size_);
        Iterator element_position = elements_.Get() + distance_;

        size_t new_size = size_ + 1;
        if (new_size <= capacity_) {
            std::copy_backward(element_position, elements_.Get() + size_, elements_.Get() + size_ + 1u);
            elements_[distance_] = value;
        }
        else {
            size_t new_capacity = (capacity_ != 0) ? std::max(new_size, 2u * capacity_) : 1;

            ArrayPtr<Type> new_elem(new_capacity);
            std::copy(elements_.Get(), element_position, new_elem.Get());


            new_elem[distance_] = value;
            std::copy(element_position, elements_.Get() + size_, new_elem.Get() + distance_ + 1u);

            elements_.swap(new_elem);

            capacity_ = new_capacity;
        }
        size_ = new_size;

        return elements_.Get() + distance_;
    }

    Iterator Insert(ConstIterator position, Type&& value) {
        assert(position >= begin() && position <= end());
        size_t distance_ = position - elements_.Get();
        //assert(distance_ <= size_);
        Iterator element_position = elements_.Get() + distance_;

        size_t new_size = size_ + 1;
        if (new_size <= capacity_) {
            std::move_backward(element_position, elements_.Get() + size_, elements_.Get() + size_ + 1u);
            elements_[distance_] = std::move(value);
        }
        else {
            size_t new_capacity = (capacity_ != 0) ? std::max(new_size, 2u * capacity_) : 1;

            ArrayPtr<Type> new_elem(new_capacity);
            std::move(elements_.Get(), element_position, new_elem.Get());

            new_elem[distance_] = std::move(value);
            std::move(element_position, elements_.Get() + size_, new_elem.Get() + distance_ + 1u);

            elements_.swap(new_elem);
            capacity_ = new_capacity;
        }
        size_ = new_size;

        return elements_.Get() + distance_;
    }

    Iterator Erase(ConstIterator position) {
        assert(position >= begin() && position <= end());
        std::move(std::next((Iterator)position), end(), Iterator(position));
        --size_;

        return Iterator(position);
    }

    void swap(SimpleVector<Type>& other) noexcept {
        elements_.swap(other.elements_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_)
            MoveElements(new_capacity);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return  elements_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return  elements_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Out of range");
        }
        return  elements_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Out of range");
        }
        return elements_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (capacity_ < new_size) {
            MoveElements(new_size);
        }
        else {
            if (new_size >= size_) {
                std::fill(elements_.Get() + size_, elements_.Get() + new_size, Type{});
            }
        }
        size_ = new_size;
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return elements_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return  elements_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return  elements_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return  elements_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return  elements_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return  elements_.Get() + size_;
    }

private:
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> elements_;


    template <typename Iter>
    void Assign(Iter first, Iter second) {
        size_ = capacity_ = std::distance(first, second);
        ArrayPtr<Type> temp_(size_);

        std::copy(first, second, temp_.Get());
        elements_.swap(temp_);
    }

    void MoveElements(size_t new_capacity) {
        ArrayPtr<Type> new_elements(new_capacity);
        size_t elements_count = std::min(size_, new_capacity);
        std::move(elements_.Get(), elements_.Get() + elements_count, new_elements.Get());
        elements_.swap(new_elements);

        capacity_ = new_capacity;
    }

};

template <typename Type>
bool operator==(const SimpleVector<Type>& left, const SimpleVector<Type>& right) {
    if (&left == &right)
        return true;
    return left.GetSize() == right.GetSize() && std::equal(left.begin(), left.end(), right.begin());
}

template <typename Type>
bool operator!=(const SimpleVector<Type>& left, const SimpleVector<Type>& right) {
    return !(left == right);
}

template <typename Type>
bool operator<(const SimpleVector<Type>& left, const SimpleVector<Type>& right) {
    return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
}

template <typename Type>
bool operator<=(const SimpleVector<Type>& left, const SimpleVector<Type>& right) {
    return (left < right) || (left == right);
}

template <typename Type>
bool operator>(const SimpleVector<Type>& left, const SimpleVector<Type>& right) {
    return right < left;
}

template <typename Type>
bool operator>=(const SimpleVector<Type>& left, const SimpleVector<Type>& right) {
    return (right < left) || (right == left);
}


// containers.h - F3-22: three small containers that need only operator new and delete,
// so they work in the kernel and on the host alike.
#pragma once
#include <cstddef>
#include <cstdint>
#include <new>
#include <utility>

// A growable array (the kernel's std::vector stand-in).
template <typename T>
class KVector {
public:
    KVector() = default;
    KVector(const KVector&) = delete;
    KVector& operator=(const KVector&) = delete;
    ~KVector()
    {
        clear();
        ::operator delete(data_, std::align_val_t{alignof(T)});
    }
    bool push_back(T value)
    {
        if (size_ == cap_ && !grow()) {
            return false;
        }
        new (&data_[size_]) T(std::move(value));
        ++size_;
        return true;
    }
    void clear()
    {
        for (size_t i = 0; i < size_; ++i) {
            data_[i].~T();
        }
        size_ = 0;
    }
    T& operator[](size_t i) { return data_[i]; }
    const T& operator[](size_t i) const { return data_[i]; }
    size_t size() const { return size_; }

private:
    bool grow()
    {
        size_t cap = cap_ == 0 ? 8 : cap_ * 2;
        auto* d = static_cast<T*>(::operator new(cap * sizeof(T), std::align_val_t{alignof(T)}, std::nothrow));
        if (d == nullptr) {
            return false;
        }
        for (size_t i = 0; i < size_; ++i) {
            new (&d[i]) T(std::move(data_[i]));
            data_[i].~T();
        }
        ::operator delete(data_, std::align_val_t{alignof(T)});
        data_ = d;
        cap_ = cap;
        return true;
    }
    T* data_ = nullptr;
    size_t size_ = 0;
    size_t cap_ = 0;
};

// An intrusive doubly linked list: the links live inside the objects, so adding an object
// never allocates (useful in code that must not allocate, such as an interrupt handler).
struct ListLink {
    ListLink* prev = nullptr;
    ListLink* next = nullptr;
    void* owner = nullptr;             // the object that contains this link
};

template <typename T, ListLink T::*Link>
class IntrusiveList {
public:
    void push_back(T& obj)
    {
        ListLink* l = &(obj.*Link);
        l->owner = &obj;
        l->prev = tail_;
        l->next = nullptr;
        (tail_ != nullptr ? tail_->next : head_) = l;
        tail_ = l;
        ++size_;
    }
    T* pop_front()
    {
        if (head_ == nullptr) {
            return nullptr;
        }
        ListLink* l = head_;
        head_ = l->next;
        (head_ != nullptr ? head_->prev : tail_) = nullptr;
        --size_;
        return static_cast<T*>(l->owner);
    }
    size_t size() const { return size_; }

private:
    ListLink* head_ = nullptr;
    ListLink* tail_ = nullptr;
    size_t size_ = 0;
};

// A hash map from 64-bit keys to values: open addressing with linear probing.
template <typename V>
class KHashMap {
public:
    ~KHashMap() { delete[] slots_; }
    bool put(uint64_t key, V value)
    {
        if ((count_ + 1) * 4 > cap_ * 3 && !rehash(cap_ == 0 ? 16 : cap_ * 2)) {
            return false;
        }
        Slot& s = find(key);
        if (!s.used) {
            s.used = true;
            s.key = key;
            ++count_;
        }
        s.value = std::move(value);
        return true;
    }
    V* get(uint64_t key)
    {
        if (cap_ == 0) {
            return nullptr;
        }
        Slot& s = find(key);
        return s.used ? &s.value : nullptr;
    }
    size_t size() const { return count_; }

private:
    struct Slot {
        bool used = false;
        uint64_t key = 0;
        V value{};
    };
    static uint64_t hash(uint64_t k)   // a 64-bit mixing function (multiply-xorshift)
    {
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccdull;
        k ^= k >> 33;
        return k;
    }
    Slot& find(uint64_t key)
    {
        size_t i = hash(key) & (cap_ - 1);
        while (slots_[i].used && slots_[i].key != key) {
            i = (i + 1) & (cap_ - 1);
        }
        return slots_[i];
    }
    bool rehash(size_t cap)
    {
        Slot* old = slots_;
        size_t old_cap = cap_;
        slots_ = new (std::nothrow) Slot[cap];
        if (slots_ == nullptr) {
            slots_ = old;
            return false;
        }
        cap_ = cap;
        count_ = 0;
        for (size_t i = 0; i < old_cap; ++i) {
            if (old[i].used) {
                Slot& s = find(old[i].key);
                s.used = true;
                s.key = old[i].key;
                s.value = std::move(old[i].value);
                ++count_;
            }
        }
        delete[] old;
        return true;
    }
    Slot* slots_ = nullptr;
    size_t cap_ = 0;
    size_t count_ = 0;
};

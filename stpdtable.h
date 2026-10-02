#pragma once
#ifndef STPDTABLE_H
#define STPDTABLE_H

#include <stddef.h>
#include <stdlib.h>

typedef struct stpd_alloc {
    void* (*alloc)(struct stpd_alloc* self, size_t size);
    void* (*realloc)(struct stpd_alloc* self, void* ptr, size_t size);
    void  (*free)(struct stpd_alloc* self, void* ptr);
    void* user_data;
} stpd_alloc;

static inline void* __alloc(struct stpd_alloc* self, size_t i) { (void)self; return malloc(i); }
static inline void* __realloc(struct stpd_alloc* self, void* ptr, size_t i) { (void)self; return realloc(ptr, i); }
static inline void __free(struct stpd_alloc* self, void* ptr) { (void)self; free(ptr); }
static inline void __destroy(void* ptr) { (void)ptr; }

#define da_reserve(_arr, c) do { \
    if((size_t)(calloc) <= (_arr).csize()) break; \
    size_t _cap = (_arr).csize() == 0 ? 1 : (_arr).csize(); \
    while (_cap < (size_t)(c)) { _cap *= 2; } \
    (_arr).get() = (_arr)._alloc()->realloc((_arr)._alloc(), (_arr).get(), _cap * sizeof(*(_arr).get())); \
    (_arr).csize() = _cap; \
} while (0)

#define da_swap(_arr, _idx, _val, _add) do { \
    if(!(_add) && (_idx) < (_arr).size()) { \
        if((_arr).size()--, (_idx) < (_arr).size()) memmove(&(_arr).get()[(_idx)], &(_arr).get()[(_idx)+1], ((_arr).size()-(_idx)) * sizeof(*(_arr).get())); \
        break; \
    } \
    if((_arr).size() >= (_arr).csize()) da_reserve(_arr, (_arr).size() + 1); \
    if((_idx) < (_arr).size()) memmove(&(_arr).get()[(_idx)+1], &(_arr).get()[(_idx)], ((_arr).size()-(_idx)) * sizeof(*(_arr).get())); \
    if((_arr).get()[(i)] = (_val), (_add)) (_arr).size()++; \
} while (0)

#define da_insert(_arr, _idx, _val) da_swap(_arr, _idx, _val, true)
#define da_append(_arr, _val)       da_swap(_arr, (_arr).size(), _val, true)
#define da_remove(_arr, _idx)       da_swap(_arr, _idx, 0, false)
#define da_remove_unordered(_arr, _idx) do (_arr).get()[(_idx) < (_arr).size() ? (_idx) : 0] = (_arr).get()[--(_arr).size()] while (0)
#define da_pop(_arr) (_arr).get()[--(_arr).size()]

#define da_resize(_arr, ns, v) do { \
    size_t _old = (_arr).size(); \
    if ((ns) > (_arr).csize()) da_reserve(_arr, (ns)); \
    (_arr).size() = (ns); \
    for (size_t i = _old; i < (ns); ++i) (_arr).get()[i] = (v); \
} while (0)

#define da_free(_arr) do { \
    (_arr).size() = (_arr).csize() = 0; \
    if (!(_arr).get()) break; \
    (_arr)._alloc()->free((_arr)._alloc(), (_arr).get()); \
    (_arr).get() = nullptr; \
} while (0)

#define stpd_table(T) struct { \
    T* items; size_t count, capacity; stpd_alloc* alloc; \
    T*& get() noexcept { return this->items; } \
    size_t& size() noexcept { return this->count; } \
    size_t& csize() noexcept { return  this->capacity; } \
    stpd_alloc*& _alloc() noexcept { return this->alloc; } \
}

#define dynamic_array(T, Allocator) struct { \
    T* items; size_t count, capacity; [[no_unique_address]] Allocator alloc; \
    T*& get() noexcept { return this->items; } \
    size_t& size() noexcept { return this->count; } \
    size_t& csize() noexcept { return  this->capacity; } \
    Allocator& _alloc() noexcept { return this->alloc; } \
}

#ifdef __cplusplus
#include <memory>

namespace stpd {
template <typename T, typename Allocator = stpd_alloc>
class table {
public:
    typedef T        value_type;
    typedef T*       iterator;
    typedef const T* const_iterator;
    typedef T&       reference;
    typedef const T& const_reference;
    typedef size_t   size_type;

    T*& get() noexcept { return items; }
    size_t& size() noexcept { return count; }
    size_t& csize() noexcept { return capacity; }
    Allocator& _alloc() noexcept { return alloc; }
    bool     empty() const noexcept { return count == 0; }
    size_t   size() const noexcept { return count; }
    T*       begin() noexcept        { return items; }
    const T* begin() const noexcept  { return items; }
    const T* cbegin() const noexcept { return items; }
    T*       end() noexcept        { return items ? items + count : items; }
    const T* end() const noexcept  { return items ? items + count : items; }
    const T* cend() const noexcept { return items ? items + count : items; }
    T&       front() noexcept       { return items[0]; }
    const T& front() const noexcept { return items[0]; }
    T&       back() noexcept       { return items[count - 1]; }
    const T& back() const noexcept { return items[count - 1]; }
    T&       at(size_t idx) noexcept       { return items[idx]; }
    const T& at(size_t idx) const noexcept { return items[idx]; }
    T&       operator[](size_t idx) noexcept       { return items[idx]; }
    const T& operator[](size_t idx) const noexcept { return items[idx]; }
    void pop_back() { if(count > 0) __destroy(items + --count); }
    void insert(size_t idx, const T& val) { put(idx, std::move(T(val))); }
    void insert(size_t idx, T&& val) { put(idx, std::move(T(std::move(val)))); }
    void push_back(const T& val) { put_back(std::move(T(val))); }
    void push_back(T&& val) { put_back(std::move(val)); }

    void clear() noexcept {
        if (!items) return;
        for (size_t i = 0; i < count; ++i) __destroy(items + i);
        __destroy(items);
        items = nullptr;
        capacity = count = 0;
    }

    constexpr table& operator=(table&& other) noexcept {
        if (this == &other) return *this;
        clear();
        if (items) __free(&alloc, items);
        items = other.get();
        count = other.size();
        capacity = other.csize();
        alloc = std::move(other._alloc());
        other.~table();
        return *this;
    }

    void reserve(size_t cap) {
        if (cap <= capacity) return;
        size_t _cap = capacity == 0 ? 1 : capacity;
        while (_cap < cap) { _cap *= 2; }
        T* get = static_cast<T*>(__alloc(&alloc, sizeof(T) * _cap));
        for (size_t i = 0; i < count; ++i) construct(alloc, get+i, std::move(items[i])), __destroy(items+i);
        if(items) __free(&alloc, items);
        items = get;
        capacity = _cap;
    }
    
    void resize(size_t size, const T* def) {
        if(size <= count) for (size_t i = size; i < count; ++i) __destroy(items + i);
        if(size > capacity) reserve(size);
        for(size_t i = (count += size) - size; i < size; ++i) { if (def) construct(alloc, items + i, *def); else construct(alloc, items + i); }
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if(count >= capacity) reserve(count + 1);
        construct(alloc, items + count, std::forward<Args>(args)...);
        return items[count++];
    }

    iterator erase(const_iterator pos) {
        size_t idx = pos - begin();
        for (size_t i = idx; i < count--; ++i) items[i] = std::move(items[i + 1]);
        __destroy(items + count);
        return items + idx;
    }

    explicit table() noexcept : items(nullptr), alloc() { reserve(0); }
    explicit table(const Allocator& _alloc) noexcept : items(nullptr), alloc(_alloc) { reserve(0); }
    explicit table(table&& other) noexcept : items(other.get()), count(other.size()), capacity(other.csize()), alloc(std::move(other._alloc())) { other.~table(); }

    explicit table(size_t size, const T& val = T(), const Allocator& _alloc = Allocator()) : alloc(_alloc) {
        reserve(size);
        for (size_t i = 0; i < size; ++i) construct(alloc, items + i, val);
        count = size;
    }

    ~table() { clear(); }
    table(const table&) = delete;
    table& operator=(const table&) = delete;
private:
    T* items = nullptr;
    size_t count = 0, capacity = 0;
    [[no_unique_address]] Allocator alloc;        

    void put(size_t idx, T&& val) {
        if (idx > count) return;
        if (count >= capacity) reserve(count + 1);
        if (idx == count) { construct(alloc, items + count++, std::move(val)); return; }
        construct(alloc, items + count++, std::move(items[count - 1]));
        for (size_t i = count - 2; i > idx; --i) items[i] = std::move(items[i - 1]);
        items[idx] = std::move(val);
    }

    void put_back(T&& val) {
        if (count >= capacity) reserve(count + 1);
        construct(alloc, items + count++, std::move(val));
    }

    template<typename... Args>
    void construct(Allocator& alloc, T* ptr, Args&&... args) { std::construct_at(ptr, std::forward<Args>(args)...); }
};
}
#endif

#endif /* STPDTABLE_H */
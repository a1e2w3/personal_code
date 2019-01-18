/**
 * @file instance_pool.h
 * @author wangcong(a1e2w3@baidu.com)
 * @date 2018-02-09 13:35:41
 * @brief 一个通用的无锁对象池
 *  
 **/
 
#pragma once
 
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <type_traits>
#include <utility>

#ifdef __DEBUG_INSTANCE_POOL__
#include <com_log.h>
#define PRINT_DEBUG(fmt, args...) \
    do { \
        com_writelog(COMLOG_DEBUG, "[%s:%d] "fmt, __FILE__, __LINE__, ##args); \
    } while (0)
#else
#define PRINT_DEBUG(fmt, args...)
#endif

namespace common {

inline uint8_t lowest_bit_8(uint8_t val) {
    uint8_t pos = 0;
    if ((val & 0x0f) == 0) {
        pos += 4;
        val >>= 4;
    }

    if ((val & 0x03) == 0) {
        pos += 2;
        val >>= 2;
    }

    if ((val & 0x01) == 0) {
        pos += 1;
        val >>= 1;
    }

    return pos;
}

inline uint8_t lowest_bit_8_table(uint8_t val) {
    static uint8_t _s_table[256];
    static std::once_flag _s_init_once_flag;

    std::call_once(_s_init_once_flag,
            [] (uint8_t* table, size_t size) {
                PRINT_DEBUG("init lowest bit table");
                table[0] = 0;
                for (size_t i = 1; i < size; ++i) {
                    table[i] = lowest_bit_8(static_cast<uint8_t>(i));
                }
            },
            _s_table, 256);
    return _s_table[val];
}

template<typename UIntType>
inline uint8_t lowest_bit_pos(UIntType data) {
    uint8_t pos = 0;
    for (size_t i = 0; i < sizeof(UIntType); ++i) {
        if ((data & 0xff) == 0) {
            pos += 8;
            data >>= 8;
        } else {
            return pos + lowest_bit_8_table(data);
        }
    }
    return 0;
}
// specialize for uint_8
template<>
inline uint8_t lowest_bit_pos<uint8_t>(uint8_t data) {
    return lowest_bit_8_table(data);
}
 
template<class T, class... Args>
class InstancePool {
public:
    using element_type = typename std::remove_reference<T>::type;
    using pointer = element_type*;
    using reference = element_type&;
    using const_pointer = const element_type*;
    using const_reference = const element_type&;

    // call while instance fetch and give_back
    using construct_type = std::function<pointer (pointer, Args&&...)>;
    using deconstruct_type = std::function<void (pointer)>;

    // call while instance pool create and destroy
    using initializer_type = std::function<void (pointer)>;
    using uninitializer_type = std::function<void (pointer)>;

private:
    using slot_type = uint8_t;
    using slot_index_type = uint32_t;

    // constants
    // capacity per slot which is number of bits of slot_type
    static const size_t SLOT_CAPACITY = sizeof(slot_type) << 3;
    // max slot count which is max number of slot_index_type + 1
    static const size_t MAX_SLOT_COUNT = 0x01UL <<  (sizeof(slot_index_type) << 3);
    // max instance count
    static const size_t MAX_CAPACITY = MAX_SLOT_COUNT * SLOT_CAPACITY;

    static const slot_type SLOT_INITIALIZER = ~static_cast<slot_type>(0);

    static inline size_t slots_capacity(size_t capacity) {
        return capacity == 0 ? 0 : ((capacity - 1) / SLOT_CAPACITY) + 1;
    }

    static inline std::pair<slot_index_type, slot_index_type> slot_of(size_t index) {
        return std::make_pair(index / SLOT_CAPACITY, index % SLOT_CAPACITY);
    }

    static inline uint8_t lowest_bit_of(slot_type data) {
        return lowest_bit_pos<slot_type>(data);
    }

    // default as placement new
    static pointer default_constructor(pointer p, Args&&... args) {
        if (p) {
            return new (p) element_type(std::forward<Args>(args)...);
        } else {
            return p;
        }
    }

    // default as deconstructor
    static void default_deconstructor(pointer p) {
        if (p) {
            p->~element_type();
        }
    }
    static void do_nothing(pointer /*p*/) { /* do nothing */ }

public:
    explicit InstancePool(size_t capacity)
        : InstancePool(capacity,
                 InstancePool<T, Args...>::do_nothing,
                 InstancePool<T, Args...>::do_nothing) {}
    InstancePool(size_t capacity,
            const initializer_type& initializer, const uninitializer_type& uninitializer)
        : _capacity(std::min(static_cast<size_t>(MAX_CAPACITY), capacity)),
          _slot_count(slots_capacity(_capacity)),
          _start(nullptr),
          _end(nullptr),
          _availablity(_capacity),
          _slot_rr(0),
          _slots(nullptr),
          _initializer(initializer),
          _uninitializer(uninitializer),
          _constructor(InstancePool<T, Args...>::default_constructor),
          _deconstructor(InstancePool<T, Args...>::default_deconstructor) {
        init();
    }
    InstancePool(size_t capacity, initializer_type&& initializer, uninitializer_type&& uninitializer)
            : _capacity(std::min(static_cast<size_t>(MAX_CAPACITY), capacity)),
              _slot_count(slots_capacity(_capacity)),
              _start(nullptr),
              _end(nullptr),
              _availablity(_capacity),
              _slot_rr(0),
              _slots(nullptr),
              _initializer(std::move(initializer)),
              _uninitializer(std::move(uninitializer)),
              _constructor(InstancePool<T, Args...>::default_constructor),
              _deconstructor(InstancePool<T, Args...>::default_deconstructor) {
            init();
        }

    ~InstancePool() {
        release();
    }

    void set_constructor(const construct_type& constructor) {
        _constructor = constructor;
    }
    void set_constructor(construct_type&& constructor) {
        _constructor = std::move(constructor);
    }
    void set_deconstructor(const deconstruct_type& deconstructor) {
        _deconstructor = deconstructor;
    }
    void set_deconstructor(deconstruct_type&& deconstructor) {
        _deconstructor = std::move(deconstructor);
    }

    // find all slot
    pointer fetch(Args&&... args) {
        if (!_start || availablity() == 0) {
            PRINT_DEBUG("availablity == 0, just new");
            return allocate_and_construct(std::forward<Args>(args)...);
        }
        slot_index_type start_index = (_slot_rr++) % _slot_count;
        for (slot_index_type i = start_index; i < _slot_count; ++i) {
            pointer p = fetch_in_slot(i);
            if (p != nullptr) {
                return _constructor(p, std::forward<Args>(args)...);
            }
        }
        for (slot_index_type i = 0; i < start_index; ++i) {
            pointer p = fetch_in_slot(i);
            if (p != nullptr) {
                return _constructor(p, std::forward<Args>(args)...);
            }
        }

        PRINT_DEBUG("availablity == 0, just new");
        return allocate_and_construct(std::forward<Args>(args)...);
    }

    // find one slot only
    pointer fetch_fast_fail(Args&&... args) {
        if (!_start || availablity() == 0) {
            PRINT_DEBUG("availablity == 0, just new");
            return allocate_and_construct(std::forward<Args>(args)...);
        }
        slot_index_type start_index = (_slot_rr++) % _slot_count;
        pointer p = fetch_in_slot(start_index);
        if (p != nullptr) {
            return _constructor(p, std::forward<Args>(args)...);
        }

        PRINT_DEBUG("availablity == 0, just new");
        return allocate_and_construct(std::forward<Args>(args)...);
    }

    void give_back(pointer p) {
        ssize_t index = index_of(p);
        if (index < 0) {
            // not in pool, just delete
            PRINT_DEBUG("%p not in [%p-%p], just delete", p, _start, _end);
            deconstruct_and_free(p);
        } else {
            auto slot_pair = slot_of(index);
            PRINT_DEBUG("give back %p [%p-%p] %ld to slot[%u-%u]",
                    p, _start, _end, index, slot_pair.first, slot_pair.second);

            // call deconstructor firstly
            _deconstructor(p);

            // set index available
            _slots[slot_pair.first] |= (0x01 << slot_pair.second);

            // incr availablity
            ++_availablity;
        }
    }

    size_t availablity() const {
        return _availablity.load();
    }

    size_t capacity() const {
        return _capacity;
    }

private:
    pointer allocate_and_construct(Args&&... args) {
        pointer p = reinterpret_cast<pointer>(malloc(sizeof(element_type)));
        _initializer(p);
        return _constructor(p, std::forward<Args>(args)...);
    }

    void deconstruct_and_free(pointer p) {
        _deconstructor(p);
        _uninitializer(p);
        free(p);
    }

    ssize_t index_of(pointer p) {
        ssize_t index = -1;
        if (_start && p >= _start && p < _end) {
            index = p - _start;
        }
        return index;
    }

    void init() {
        lowest_bit_8_table(1);
        if (_capacity > 0) {
            _start = reinterpret_cast<pointer>(malloc(_capacity * sizeof(element_type)));
            _end = _start + _capacity;

            // init instance
            for (pointer p = _start; p != _end; ++p) {
                _initializer(p);
            }

            // init slots
            _slots = new std::atomic<slot_type>[_slot_count];
            for (slot_index_type i = 0; i < _slot_count - 1; ++i) {
                _slots[i].store(SLOT_INITIALIZER);
            }
            size_t remain = _capacity - ((_slot_count - 1) * SLOT_CAPACITY);
            slot_type highest_slot_init = (0x01UL << remain) - 1;
            _slots[_slot_count - 1].store(highest_slot_init);
        }
    }

    void release() {
        if (_slots) {
            delete[] _slots;
            _slots = nullptr;
        }
        if (_start) {
            for (pointer p = _start; p != _end; ++p) {
                _uninitializer(p);
            }
            free(_start);
            _start = nullptr;
            _end = nullptr;
        }
    }

    pointer fetch_in_slot(slot_index_type slot) {
        slot_type slot_data = _slots[slot].load();
        PRINT_DEBUG("trying fetch in slot:%u-%x", slot, slot_data);
        while (slot_data != 0) {
            slot_index_type index_in_slot = lowest_bit_of(slot_data);
            slot_type mask = ~(0x01UL << index_in_slot);
            slot_type new_data = (slot_data & mask);
            PRINT_DEBUG("try set slot %u value %x - %x fetch index %u",
                    slot, slot_data, new_data, index_in_slot);
            if (_slots[slot].compare_exchange_strong(slot_data, new_data)) {
                // found available instance
                --_availablity;
                size_t index = index_in_slot + (slot * SLOT_CAPACITY);

                PRINT_DEBUG("fetch instance at %ld in slot[%u-%u] %lx",
                        index, slot, index_in_slot, new_data);
                return _start + index;
            } else {
                PRINT_DEBUG("slot %u value changed %x retry", slot, slot_data);
            }
        }
        return nullptr;
    }

private:
    const size_t _capacity;
    const size_t _slot_count;
    pointer _start;
    pointer _end;

    // round robin between slots
    std::atomic<size_t> _availablity;
    std::atomic<slot_index_type> _slot_rr;
    std::atomic<slot_type>* _slots;

    initializer_type _initializer;
    uninitializer_type _uninitializer;

    construct_type _constructor;
    deconstruct_type _deconstructor;
};
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */


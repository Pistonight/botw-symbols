/**
 * Raw memory access
 */
#pragma once

#include <cstdint>

namespace botw::mem {

/**
 * Check if the value looks like a pointer
 */
bool ptr_looks_safe(const void* ptr);

/**
 * Raw memory pointer that can be manipulated using array-like syntax
 *
 * for example:
 * x[0x2cA1A78] is *(x + 0x2cA1A78)
 *
 * Additionally, it checks if the pointer looks safe before dereferencing it.
 * If the pointer looks unsafe, it will set an error flag and prevents further
 * manipulation.
 *
 * Usually this is converted to a safe_ptr<T> before using
 */
class mem_ptr {
public:
    // Create raw pointer to address
    mem_ptr(void** ptr) { m_ptr = reinterpret_cast<char*>(ptr); }
    // Copy constructor
    mem_ptr(const mem_ptr& other) {
        m_ptr = other.m_ptr;
        m_error = other.m_error;
    }
    // Offset
    mem_ptr& add(int64_t offset) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
        m_ptr += offset;
#pragma GCC diagnostic pop
        return *this;
    }
    // p+x is the same as p.add(xxx)
    mem_ptr& operator+(int64_t offset) { return add(offset); }
    // Deferring and storing the value as pointer
    mem_ptr& deref() {
        if (m_error || !ptr_looks_safe(m_ptr)) {
            m_error = true;
            return *this;
        }
        char** pp = reinterpret_cast<char**>(m_ptr);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
        m_ptr = *pp;
#pragma GCC diagnostic pop
        return *this;
    }
    // p[x] is the same as p.add(xxx).deref()
    mem_ptr& operator[](int64_t offset) { return add(offset).deref(); }

public:
    char* m_ptr;
    bool m_error = false;
};

extern "C" void* __botw_main_memory;

/** 
 * Get the address of the main module
 *
 * Putting this in a mem_ptr will allow access raw memory from the main module
 * with the usual known offsets
 *
 * e.g. mem_ptr(main_module())[0x2cA1A78]
 */
inline void** main_module() {
    return &__botw_main_memory;
}

}  // namespace botw::savs

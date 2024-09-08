/**
 * Safer memory access
 */
#pragma once

#include "toolkit/mem/mem_ptr.hpp"
#include <cstddef>

namespace botw::mem {

/**
 * safe_ptr<T> can be used like T*, but it checks if the inner value
 * looks like a valid pointer before dereferencing it.
 */
template <typename T> class safe_ptr {
public:
    safe_ptr(T* ptr) : m_ptr(ptr) {}
    safe_ptr(const safe_ptr& other) : m_ptr(other.m_ptr) {}
    safe_ptr(const mem_ptr& p) {
        m_ptr = p.m_error ? nullptr : reinterpret_cast<T*>(p.m_ptr);
    }

    bool take_ptr(T** out) const {
        if (!looks_safe()) {
            return false;
        }
        *out = m_ptr;
        return true;
    }

    /** *out = *this */
    bool get(T* out) const {
        if (!looks_safe()) {
            return false;
        }
        *out = *m_ptr;
        return true;
    }

    /** *this = value */
    bool set(T value) const {
        if (!looks_safe()) {
            return false;
        }
        *m_ptr = value;
        return true;
    }

    /** *out = this[i] */
    bool get(T* out, size_t i) const {
        if (!looks_safe()) {
            return false;
        }
        *out = m_ptr[i];
        return true;
    }

    /** this[i] = value */
    bool set(T value, size_t i) const {
        if (!looks_safe()) {
            return false;
        }
        m_ptr[i] = value;
        return true;
    }

    bool get_array(T* out, size_t len) const {
        if (!looks_safe()) {
            return false;
        }
        for (size_t i = 0; i < len; i++) {
            out[i] = m_ptr[i];
        }
        return true;
    }

    bool set_array(const T* array, size_t len) const {
        if (!looks_safe()) {
            return false;
        }
        for (size_t i = 0; i < len; i++) {
            m_ptr[i] = array[i];
        }
        return true;
    }

    bool looks_safe() const { return ptr_looks_safe(m_ptr); }

private:
    T* m_ptr;
};
} // namespace botw::mem

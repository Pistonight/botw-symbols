/**
 * Unique pointer with malloc/free
 */
#pragma once

#include <cstdlib>
#include <memory>

namespace botw::mem {

struct raw_free {
    void operator()(void* ptr) const {
        free(ptr);
    }
};

template <typename T>
using unique_ptr = std::unique_ptr<T, raw_free>;

/* template <typename T> */
/* class unique_ptr { */
/* public: */
/*     unique_ptr() = default; */
/*     unique_ptr(T* ptr) : m_ptr(ptr) {} */
/*     ~unique_ptr() { */
/*         if (m_ptr) { */
/*             m_ptr->~T(); */
/*             free(m_ptr); */
/*         } */
/*     } */
/*  */
/*     // delete copy constructor and assignment operator */
/*     unique_ptr(const unique_ptr&) = delete; */
/*     unique_ptr& operator=(const unique_ptr&) = delete; */
/*  */
/*     // move constructor */
/*     unique_ptr(unique_ptr&& other) noexcept { */
/*         this->swap(other); */
/*     } */
/*  */
/*     // move assignment operator */
/*     unique_ptr<T>& operator=(unique_ptr<T>&& other) noexcept { */
/*         this->swap(other); */
/*         return *this; */
/*     } */
/*  */
/*     void swap(unique_ptr<T>& other) { */
/*         std::swap(m_ptr, other.m_ptr); */
/*     } */
/*  */
/*     T* operator->() const { */
/*         return m_ptr; */
/*     } */
/*  */
/*     T& operator*() const { */
/*         return *m_ptr; */
/*     } */
/*  */
/*     operator bool() const { */
/*         return m_ptr != nullptr; */
/*     } */
/*  */
/* private: */
/*     T* m_ptr = nullptr; */
/* }; */

template <typename T>
unique_ptr<T> make_unique() {
    T* ptr = static_cast<T*>(malloc(sizeof(T)));
    if (!ptr) {
        return {};
    }
    unique_ptr<T> uptr { ptr };
    new (ptr) T();
    return std::move(uptr);
}

}

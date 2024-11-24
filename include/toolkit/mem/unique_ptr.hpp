/**
 * Unique pointer with malloc/free
 */
#pragma once

#include <cstdlib>
#include <memory>

namespace botw::mem {

struct raw_free {
    void operator()(void* ptr) const { free(ptr); }
};

template <typename T> using unique_ptr = std::unique_ptr<T, raw_free>;

template <typename T, class... Args > unique_ptr<T> make_unique( Args&&... args ) {
    T* ptr = static_cast<T*>(malloc(sizeof(T)));
    if (!ptr) {
        return {};
    }
    unique_ptr<T> uptr{ptr};
    new (ptr) T(std::forward<Args>(args)...);
    return std::move(uptr);
}

} // namespace botw::mem

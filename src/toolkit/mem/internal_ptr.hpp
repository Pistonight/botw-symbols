/**
 * Utility for internal pointer pointing into a struct
 */

#include <cstdint>

namespace botw::mem {

template <typename T> class internal_ptr {
public:
    internal_ptr() = default;
    internal_ptr(void* base, T* ptr) {
        if (base == nullptr) {
            return;
        }
        if (ptr == nullptr) {
            return;
        }
        uintptr_t base_addr = reinterpret_cast<uintptr_t>(base);
        uintptr_t ptr_addr = reinterpret_cast<uintptr_t>(ptr);
        if (ptr_addr < base_addr) {
            return;
        }
        m_offset = ptr_addr - base_addr;
        m_nullptr = false;
    }
    internal_ptr(const internal_ptr<T>& other) {
        m_nullptr = other.m_nullptr;
        m_offset = other.m_offset;
    }
    T* hydrate(void* base) const {
        if (m_nullptr) {
            return nullptr;
        }
        return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(base) +
                                    m_offset);
    }

    bool is_nullptr() const { return m_nullptr; }

    uintptr_t offset() const { return m_offset; }

    void set(bool is_nullptr, uintptr_t offset) {
        m_nullptr = is_nullptr;
        m_offset = offset;
    }

private:
    bool m_nullptr = true;
    uintptr_t m_offset = 0;
};

} // namespace botw::mem

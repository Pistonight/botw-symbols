
#include <cstdint>

namespace botw::mem {

bool ptr_looks_safe(const void* ptr) {
    uintptr_t raw = reinterpret_cast<uintptr_t>(ptr);

    if (raw > 0xFFFFFFFFFF || (raw >> 32 == 0)) {
        return false;
    }
    if ((raw & 3) != 0) {
        return false;
    }

    return true;
}

} // namespace botw::mem

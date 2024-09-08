/**
 * Wrapper utility for sead::ListImpl
 */
#pragma once

#include <cstdint>

namespace sead {
class ListImpl;
}

namespace botw::toolkit::sead {

/** Get the real size of the list by traversing the list, not by mCount */
int32_t get_list_real_size(::sead::ListImpl* list);

}


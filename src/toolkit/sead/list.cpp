// Need private field access
#define private public
#define protected public
#include <container/seadListImpl.h>
#undef private
#undef protected

namespace botw::toolkit::sead {

int32_t get_list_real_size(::sead::ListImpl* list) {
    int32_t size = 0;
    ::sead::ListNode* ptr = list->mStartEnd.mNext;
    while(ptr && ptr != &list->mStartEnd){
        ptr = ptr->mNext;
        size++;
    }
    return size;
}

}

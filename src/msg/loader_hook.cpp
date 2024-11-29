#include <megaton/hook.h>
#include <prim/seadSafeString.h>

#include "toolkit/msg/info.hpp"
#include "toolkit/msg/loader_hook.hpp"
#if BOTW_VERSION == 160
#include "toolkit/msg/widget.hpp"
#endif

namespace botw::msg {

struct hook_trampoline_(ksys_ui_getMessage) {
#if BOTW_VERSION == 160
    target_offset_(0x0123DEA0)
#elif BOTW_VERSION == 150
    target_offset_(0x00AA248C)
#endif
    static int call(sead::SafeString * file, sead::SafeString* msg_id, WideString* out) {
        if (info::load_custom_mesasge(file, msg_id, out)) {
            return 0;
        }
#if BOTW_VERSION == 160
        if (widget::load_custom_mesasge(file, msg_id, out)) {
            return 0;
        }
#endif
        // original function for other cases
        return call_original(file, msg_id, out);
    }
};

void init_loader_hook() {
    ksys_ui_getMessage::install();
}

}

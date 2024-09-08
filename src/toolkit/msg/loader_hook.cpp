
#include <exl/lib.hpp>
#include <prim/seadSafeString.h>

#include "toolkit/msg/loader_hook.hpp"
#include "toolkit/msg/info.hpp"
#include "toolkit/msg/widget.hpp"

namespace botw::msg {

static bool s_installed = false;
// clang-format off
HOOK_DEFINE_TRAMPOLINE(ksys_ui_getMessage_hook){
    static int Callback(sead::SafeString * file, sead::SafeString* msg_id, WideString* out) {
        if (info::load_custom_mesasge(file, msg_id, out)) {
            return 0;
        }
        if (widget::load_custom_mesasge(file, msg_id, out)) {
            return 0;
        }
        // original function for other cases
        return Orig(file, msg_id, out);
    }
};
// clang-format on

void init_loader_hook() {
    if (s_installed) {
        return;
    }
    s_installed = true;
#if BOTW_VERSION == 160
    ksys_ui_getMessage_hook::InstallAtOffset(0x0123DEA0);
#elif BOTW_VERSION == 150
    ksys_ui_getMessage_hook::InstallAtOffset(0x00AA248C);
#endif

}

}

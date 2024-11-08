#if BOTW_VERSION == 160
#include <cstddef>
#endif

#include <exl_hook/prelude.h>
#include <nn/os.h>

#if BOTW_VERSION == 160
#include "toolkit/mem/string.hpp"
#include "toolkit/msg/loader_hook.hpp"
#include "toolkit/msg/widget.hpp"
#endif

extern "C" {
// 0x0119C750 (1.6.0)
void ScreenMessageTipsRuntime_doShowMessageTip(void* this_, u32 idx, bool);

// 0x00020950 (1.6.0)
// 0x00A261CC (1.5.0)
/* void ksys_ui_initMessageTipsRuntime(); */

// 0x02CBA3A8 (1.6.0)
// 0x025EFC08 (1.5.0)
/* extern u32 ksys_ui_sRuntimeTipsNum; */

// 0x02CBA3B0 (1.6.0)
// 0x025EFC10 (1.5.0)
extern botw::msg::widget::RuntimeTip* ksys_ui_sRuntimeTips;

// 0x02CC2490 (1.6.0)
// 0x025FCC68 (1.5.0)
extern botw::msg::widget::ScreenMgr* ksys_ui_ScreenMgr_sInstance;
}

#if BOTW_VERSION == 160
namespace botw::msg::widget {

constexpr size_t WIDGET_BUFFER_LEN = 280;
static mem::WStringBuffer<WIDGET_BUFFER_LEN> s_widget;
static uint32_t s_widget_idx = 0;
static bool s_enabled = false;

// clang-format off
// Hooking the initialization of message tip to override the "Wolf Link" text and flag
// this is because we need 2 messages and cycle between them to force update
hook_trampoline_(ksys_ui_sInitMessageTipsRuntime_hook){
    static void Callback(){
        Orig();
        ksys_ui_sRuntimeTips[0x0E].m_label = "0025";
        // note that this will not work for 1.5.0
        // 1.6.0 added a message for VR mode without a flag,
        // so they added a check for skipping the flag check when
        // the flag is empty
        ksys_ui_sRuntimeTips[0x0E].m_flag = "";
    }
};
// clang-format on

// clang-format off
// This will make new messages show up more consistently
// Although it's still not 100% consistent
hook_trampoline_(ScreenMessageTipsRuntime_doShowMessageTip_hook){
    static void Callback(void* this_, u32 idx, bool){
        if (idx == 0x17 || idx == 0x0E){u32 set_idx = idx == 0x17 ? 0x0E : 0x17;
            (reinterpret_cast<bool*>(this_))[0x365C] = false;
            (reinterpret_cast<int*>(this_))[0xD96] = set_idx;
            (reinterpret_cast<int*>(this_))[0xD98] = set_idx;
            nn::os::YieldThread();
            nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100 * 1000 * 1000));
        }
        return Orig(this_, idx, true);
    }
};

void init() {
    msg::init_loader_hook();
    if (s_enabled) {
        return;
    }
    s_enabled = true;
#if BOTW_VERSION == 160
    ksys_ui_sInitMessageTipsRuntime_hook::InstallAtOffset(0x00020950);
    ScreenMessageTipsRuntime_doShowMessageTip_hook::InstallAtOffset(0x0119C750);
#elif BOTW_VERSION == 150
    ksys_ui_sInitMessageTipsRuntime_hook::InstallAtOffset(0x00A261CC);
    // TODO: find the offset for 1.5.0
#endif
}

bool is_ready() {
    return ksys_ui_ScreenMgr_sInstance != nullptr;
}

bool load_custom_mesasge(sead::SafeString* file, sead::SafeString* msg_id, WideString* out) {
    if (!s_enabled) {
        return false;
    }
    // only override if we are loading from the file
    if (*file == "LayoutMsg/MessageTipsRunTime_00") {
        if (*msg_id == "0025") {
            out->content = s_widget.content();
            out->length = s_widget.len();
            return true;
        }
    }
    return false;
}

bool print(const char* message) {
    if (!is_ready()) {
        return false;
    }
    // idk what this is
    void** screens = ksys_ui_ScreenMgr_sInstance->screens;
    if (ksys_ui_ScreenMgr_sInstance->num_screens > 0x29) {
        screens += 0x29;
    }
    void* screen = *screens;
    // should be a sead::DynamicCast here
    if (!screen) {
        return false;
    }
    s_widget.copy_from(message);
    // cycle the message index so the game would clear the previous one
    s_widget_idx = s_widget_idx == 0x17 ? 0x0E : 0x17;
    ScreenMessageTipsRuntime_doShowMessageTip(screen, s_widget_idx, true);
    return true;
}

bool printf(const char* format, ...) {
    char result[WIDGET_BUFFER_LEN + 1];
    result[WIDGET_BUFFER_LEN - 1] = '\0';
    va_list args;
    va_start(args, format);
    int size = vsnprintf(result, WIDGET_BUFFER_LEN, format, args);
    va_end(args);
    if (size <= 0) {
        return false;
    }
    result[size] = '\0';
    return print(result);
}

}
#endif

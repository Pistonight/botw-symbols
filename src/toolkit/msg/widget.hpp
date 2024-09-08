/**
 * Widget message system
 *
 * This system hacks the runtime tips UI on the top-right corner of the screen
 * to programmatically show custom messages
 *
 * Currently only supported for 1.6.0 because it uses the VR message,
 * which does not have a flag check
 */
#pragma once

#if BOTW_VERSION == 160
#include <prim/seadSafeString.h>

#include "toolkit/msg/loader_hook.hpp"

namespace botw::msg::widget {

struct RuntimeTip {
    sead::SafeString m_label;
    sead::SafeString m_flag;
};
static_assert(sizeof(RuntimeTip) == 0x20);

struct ScreenMgr {
    void* vtable;
    char disposer[0x20];
    u32 num_screens;
    void** screens;
};
static_assert(offsetof(ScreenMgr, num_screens) == 0x28);
static_assert(offsetof(ScreenMgr, screens) == 0x30);

/** Initialize the system and install hooks */
void init();

/** Get if the ScreenMgr is initialized */
bool is_ready();

/**
 * Show a custom widget message on the screen.
 *
 * The max character limit is 280. Line breaks are not automatically added.
 * You need to break the lines yourself
 *
 * Return true if message is successfully scheduled (not necessarily shown or will show)
 */
bool print(const char* message);
bool printf(const char* format, ...);

/**
 * Function for the loader hook to load custom messages
 *
 * Return true if a custom message was loaded
 */
bool load_custom_mesasge(sead::SafeString* file, sead::SafeString* msg_id, WideString* out);


}
#endif

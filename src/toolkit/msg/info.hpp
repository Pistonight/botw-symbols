/**
 * Info message system
 *
 * This system hacks the info overlay (such as "Your Pot Lid is badly damaged")
 * to programmatically show custom messages
 */
#pragma once

#include <prim/seadSafeString.h>

#include "toolkit/msg/loader_hook.hpp"

namespace botw::msg::info {

/** Initialize the system and install hooks */
void init();

/**
 * Show a custom info message on the screen.
 *
 * Since long strings are truncated in the UI anyway,
 * the max character limit is 40.
 */
void print(const char* message);
void printf(const char* format, ...);

/**
 * Function for the loader hook to load custom messages
 *
 * Return true if a custom message was loaded
 */
bool load_custom_mesasge(sead::SafeString* file, sead::SafeString* msg_id, WideString* out);

}

/**
 * Shared loader hook for message system
 */
#pragma once

#include <cstdint>

namespace botw::msg {

struct WideString { // This is probably a eui::MessageString
    char16_t* content = nullptr;
    uint64_t length = 0;
};

void init_loader_hook();

} // namespace botw::msg

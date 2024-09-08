#include <cstddef>
#include <cstdint>

#include "toolkit/mem/string.hpp"
#include "toolkit/msg/info.hpp"

extern "C" {
// 0x01238680 (1.6.0)
// 0x00A95924 (1.5.0)
void* ksys_ui_showInfoOverlayWithString(u64 idx, const sead::SafeString* info);
}

namespace botw::msg::info {

constexpr size_t INFO_BUFFER_LEN = 40;
static mem::WStringBuffer<INFO_BUFFER_LEN> s_info;
static uint32_t s_info_idx = 0;
static bool s_enabled = false;

void init() {
    msg::init_loader_hook();
    s_enabled = true;
}

bool load_custom_mesasge(sead::SafeString* file, sead::SafeString* msg_id,
                         WideString* out) {
    if (!s_enabled) {
        return false;
    }
    // only override if we are loading from the file
    if (*file == "LayoutMsg/MainScreen_00") {
        // 0028 - The Master Sword has returned to the forest
        // 0061 - In this demo version, you can't advance any farther
        if (*msg_id == "0061" ||
            *msg_id == "0028") { // output the previously set message
            out->content = s_info.content();
            out->length = s_info.len();
            return true;
        }
    }
    return false;
}

void print(const char* message) {
    s_info.copy_from(message);
    // cycle the message index so the game would clear the previous one
    s_info_idx = s_info_idx == 0x21 ? 0x2A : 0x21;
    ksys_ui_showInfoOverlayWithString(
        s_info_idx, &sead::SafeStringBase<char>::cEmptyString);
}

void printf(const char* format, ...) {
    char result[INFO_BUFFER_LEN + 1];
    result[INFO_BUFFER_LEN - 1] = '\0';
    va_list args;
    va_start(args, format);
    int size = vsnprintf(result, INFO_BUFFER_LEN, format, args);
    va_end(args);
    if (size <= 0) {
        return;
    }
    result[size] = '\0';
    print(result);
}

} // namespace botw::msg::info

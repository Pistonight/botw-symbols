/**
 * Debug screen printing system
 *
 * This system enables game's debug printing system to print custom messages
 * on screen. 
 *
 * The debug system is completely removed in 1.6.0, so this system is only
 * available in 1.5.0.
 *
 * You also need a compatible System/Sead/primitive_drawer_nvm_shader.bin,
 * which is not present in the BOTW 1.5.0 rom
 */
#pragma once

namespace sead {
class TextWriter;
}

namespace botw::msg::screen {
#if BOTW_VERSION == 150
/**
 * 1.5.0 ONLY!
 *
 * Initialize the system and install hooks
 *
 * compute_fn is called before rendering the screen every frame
 * render_fn is called to print the messages and is called multiple times
 *   during rendering
 */
void init(void (*compute_fn)(), void (*render_fn)(sead::TextWriter* w));

#endif
}


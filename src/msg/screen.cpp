#if BOTW_VERSION == 150
#include <megaton/patch.h>
#endif

#include "toolkit/msg/screen.hpp"

namespace botw::msg::screen {
#if BOTW_VERSION == 150
namespace inst = exl::armv8::inst;
namespace reg = exl::armv8::reg;
namespace patch = megaton::patch;

static bool s_enabled = false;

void init(void (*compute_fn)(), void (*render_fn)(sead::TextWriter* w)) {
    static_assert(std::is_function_v<void()>);
    if (s_enabled) {
        return;
    }

    s_enabled = true;

    // clang-format off

    // Patch Screen Rendering (150 only)
    // (nnMain) Set debug heap to gfx heap
    patch::main_stream(0x007d6238) << inst::MovRegister(reg::X22, reg::X21);
    patch::main_stream(0x007d63d4) << inst::MovRegister(reg::X22, reg::X21);

    // sub_7100C65F5C(sead::TaskBase *a1, __int64 x1_0)
    // some kind of render function 
    // Patch it to call our screen compute and render
    patch::main_stream(0x00C661EC) << patch::bl(compute_fn);

    // Raw instructions that aren't supported by exlaunch yet
    constexpr exl::armv8::InstBitSet Inst_FMOV_S8_1_0 = 0x1E2E1008;
    constexpr u32 Inst_FMOV_S9_MINUS_1_0 = 0x1E3E1009;
    constexpr u32 Inst_FADD_S10_S0_S9 = 0x1E29280A;
    constexpr u32 Inst_FADD_S12_S10_S8 = 0x1E28294C;
    constexpr u32 Inst_STR_S12__SP_458_ =
        0xBD045BEC; // TODO: inst::StrRegisterImmediate(reg::S12, reg::SP,
                    // 0x458)
    constexpr u32 Inst_FMOV_S10_0 = 0x1E2703EA;
    constexpr u32 Inst_FMOV_S0_S11 = 0x1E204160;
    constexpr u32 Inst_FMOV_S12_S1 = 0x1E20402C;
    constexpr u32 Inst_FADD_S11_S0_S8 = 0x1E28280B;
    constexpr u32 Inst_FADD_S12_S1_S8 = 0x1E28282C;
    constexpr u32 Inst_FMOV_S11_S0 = 0x1E20400B;
    constexpr u32 Inst_FADD_S11_S11_S9 = 0x1E29296B;
    constexpr u32 Inst_STR_S12__SP_45C_ =
        0xBD045FEC; // TODO: inst::StrRegisterImmediate(reg::S12, reg::SP,
                    // 0x45C)
    constexpr u32 Inst_FMOV_S10_S11 = 0x1E20416A;
    constexpr u32 Inst_FADD_S11_S12_S9 = 0x1E29298B;
    constexpr u32 Inst_FADD_S0_S10_S8 = 0x1E282940;
    constexpr u32 Inst_FMOV_S1_S11 = 0x1E204161;

    // Draw Top Left
    patch::main_stream(0x00C662AC)
        << Inst_FMOV_S8_1_0           // FMOV S8,  #1.0
        << Inst_FMOV_S9_MINUS_1_0     // FMOV S9,  #-1.0
        << Inst_FADD_S10_S0_S9;       // FADD S10, S0, S9
    patch::main_stream(0x00C66300)
        << patch::bl(render_fn)
        << patch::skip(1)
        << Inst_FADD_S12_S10_S8 // FADD S12, S10, S8
        << patch::repeat(inst::Nop(), 3);

    patch::main_stream(0x00C6631C)
        << Inst_STR_S12__SP_458_ // STR  S12, [SP, #0x458]
        << patch::repeat(inst::Nop(), 4);

    // Draw Top
    patch::main_stream(0x00C66344) 
        << patch::bl(render_fn)
        << Inst_FMOV_S10_0        // FMOV S10, #0.0
        << Inst_FMOV_S0_S11;      // FMOV S0,  S11
    patch::main_stream(0x00C66374) << inst::Nop();

    // Draw Top Right
    patch::main_stream(0x00C66380)
        << Inst_FMOV_S12_S1       // FMOV S12, S1
        << Inst_FADD_S11_S0_S8;   // FADD S11, S0, S8
    patch::main_stream(0x00C663C8)
        << patch::bl(render_fn)
        << patch::skip(1)
        << inst::Nop()
        << patch::skip(1)
        << inst::Nop()
        << inst::Nop();
    patch::main_stream(0x00C663E8) << patch::repeat(inst::Nop(), 4);

    // Draw Right
    patch::main_stream(0x00C6640C) << patch::bl(render_fn);
    patch::main_stream(0x00C66440) << inst::Nop();

    // Draw Bottom Right
    patch::main_stream(0x00C6644C)
        << Inst_FADD_S12_S1_S8    // FADD S12, S1, S8
        << Inst_FMOV_S11_S0;      // FMOV S11, S0
    patch::main_stream(0x00C66498)
        << patch::bl(render_fn)
        << patch::skip(1)
        << inst::Nop();
    patch::main_stream(0x00C664AC) << patch::repeat(inst::Nop(), 10);

    // Draw Bottom
    patch::main_stream(0x00C664E8)
        << patch::bl(render_fn)
        << Inst_FADD_S11_S11_S9;  // FADD S11, S11, S9
    patch::main_stream(0x00C6650C) << Inst_STR_S12__SP_45C_; // STR  S12, [SP, #0x45C]

    // Draw Bottom Left
    patch::main_stream(0x00C6651C)
        << inst::AddImm(reg::X0, reg::SP, 0x430)
        << patch::bl(render_fn)
        << inst::Nop()
        << inst::Nop()

    // Draw Left
        << Inst_FMOV_S10_S11; // FMOV S10, S11
    patch::main_stream(0x00C66534) << Inst_FADD_S11_S12_S9; // FADD S11, S12, S9
    patch::main_stream(0x00C6656C) 
        << patch::bl(render_fn)

    // Draw Middle
        << Inst_FADD_S0_S10_S8 // FADD S0,  S10, S8
        << Inst_FMOV_S1_S11;    // FMOV S1,  S11
    patch::main_stream(0x00C665B8) << patch::bl(render_fn);

    // clang-format on
}
#endif
} // namespace botw::msg::screen

#include <exl/lib.hpp>

#include "toolkit/msg/screen.hpp"

namespace botw::msg::screen {
namespace inst = exl::armv8::inst;
namespace reg = exl::armv8::reg;

static bool s_enabled = false;

#if BOTW_VERSION == 150
void init(void (*compute_fn)(), void (*render_fn)(sead::TextWriter* w)) {
    if (s_enabled) {
        return;
    }

    s_enabled = true;

    // Patch Screen Rendering (150 only)
    // (nnMain) Set debug heap to gfx heap
    exl::patch::CodePatcher patcher { 0x007d6238 };
    patcher.WriteInst(inst::MovRegister(reg::X22, reg::X21));
    patcher.Seek(0x007d63d4);
    patcher.WriteInst(inst::MovRegister(reg::X22, reg::X21));

    // sub_7100C65F5C(sead::TaskBase *a1, __int64 x1_0) some kind of render function
    // Patch it to call our screen compute and render
    patcher.Seek(0x00C661EC);
    patcher.BranchLinkInst(reinterpret_cast<void*>(compute_fn));

    // Raw instructions that aren't supported by exlaunch yet
    constexpr u32 Inst_FMOV_S8_1_0 = 0x1E2E1008;
    constexpr u32 Inst_FMOV_S9_MINUS_1_0 = 0x1E3E1009;
    constexpr u32 Inst_FADD_S10_S0_S9 = 0x1E29280A;
    constexpr u32 Inst_FADD_S12_S10_S8 = 0x1E28294C;
    constexpr u32 Inst_STR_S12__SP_458_ = 0xBD045BEC; // TODO: inst::StrRegisterImmediate(reg::S12, reg::SP, 0x458)
    constexpr u32 Inst_FMOV_S10_0 = 0x1E2703EA;
    constexpr u32 Inst_FMOV_S0_S11 = 0x1E204160;
    constexpr u32 Inst_FMOV_S12_S1 = 0x1E20402C;
    constexpr u32 Inst_FADD_S11_S0_S8 = 0x1E28280B;
    constexpr u32 Inst_FADD_S12_S1_S8 = 0x1E28282C;
    constexpr u32 Inst_FMOV_S11_S0 = 0x1E20400B;
    constexpr u32 Inst_FADD_S11_S11_S9 = 0x1E29296B;
    constexpr u32 Inst_STR_S12__SP_45C_ = 0xBD045FEC; // TODO: inst::StrRegisterImmediate(reg::S12, reg::SP, 0x45C)
    constexpr u32 Inst_FMOV_S10_S11 = 0x1E20416A;
    constexpr u32 Inst_FADD_S11_S12_S9 = 0x1E29298B;
    constexpr u32 Inst_FADD_S0_S10_S8 = 0x1E282940;
    constexpr u32 Inst_FMOV_S1_S11 = 0x1E204161;

    // Draw Top Left
    patcher.Seek(0x00C662AC);
    patcher.WriteInst(Inst_FMOV_S8_1_0);       // FMOV S8,  #1.0
    patcher.WriteInst(Inst_FMOV_S9_MINUS_1_0); // FMOV S9,  #-1.0
    patcher.WriteInst(Inst_FADD_S10_S0_S9); // FADD S10, S0, S9

    patcher.Seek(0x00C66300);
    patcher.BranchLinkInst(reinterpret_cast<void*>(render_fn));
    patcher.Seek(0x00C66308);
    patcher.WriteInst(Inst_FADD_S12_S10_S8); // FADD S12, S10, S8 
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.Seek(0x00C6631C);
    patcher.WriteInst(Inst_STR_S12__SP_458_); //STR  S12, [SP, #0x458]
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());

    // Draw Top
    patcher.Seek(0x00C66344);
    patcher.BranchLinkInst(reinterpret_cast<void*>(render_fn));
    patcher.WriteInst(Inst_FMOV_S10_0);  // FMOV S10, #0.0
    patcher.WriteInst(Inst_FMOV_S0_S11); // FMOV S0,  S11
    patcher.Seek(0x00C66374);
    patcher.WriteInst(inst::Nop());

    // Draw Top Right
    patcher.Seek(0x00C66380);
    patcher.WriteInst(Inst_FMOV_S12_S1);    // FMOV S12, S1
    patcher.WriteInst(Inst_FADD_S11_S0_S8); // FADD S11, S0, S8
    patcher.Seek(0x00C663C8);
    patcher.BranchLinkInst(reinterpret_cast<void*>(render_fn));
    patcher.Seek(0x00C663D0);
    patcher.WriteInst(inst::Nop());
    patcher.Seek(0x00C663D8);
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.Seek(0x00C663E8);
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());

    // Draw Right
    patcher.Seek(0x00C6640C);
    patcher.BranchLinkInst(reinterpret_cast<void*>(render_fn));
    patcher.Seek(0x00C66440);
    patcher.WriteInst(inst::Nop());

    // Draw Bottom Right
    patcher.Seek(0x00C6644C);
    patcher.WriteInst(Inst_FADD_S12_S1_S8); // FADD S12, S1, S8
    patcher.WriteInst(Inst_FMOV_S11_S0);    // FMOV S11, S0
    patcher.Seek(0x00C66498);
    patcher.BranchLinkInst(reinterpret_cast<void*>(render_fn));
    patcher.Seek(0x00C664A0);
    patcher.WriteInst(inst::Nop());
    patcher.Seek(0x00C664AC);
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());

    // Draw Bottom
    patcher.Seek(0x00C664E8);
    patcher.BranchLinkInst(reinterpret_cast<void*>(render_fn));
    patcher.WriteInst(Inst_FADD_S11_S11_S9);   // FADD S11, S11, S9
    patcher.Seek(0x00C6650C);
    patcher.WriteInst(Inst_STR_S12__SP_45C_);  // STR  S12, [SP, #0x45C]
    
    // Draw Bottom Left
    patcher.Seek(0x00C6651C);
    patcher.WriteInst(inst::AddImmediate(reg::X0, reg::SP, 0x430)); 
    patcher.BranchLinkInst(reinterpret_cast<void*>(render_fn));
    patcher.WriteInst(inst::Nop());
    patcher.WriteInst(inst::Nop());
    
    // Draw Left
    patcher.WriteInst(Inst_FMOV_S10_S11);     // FMOV S10, S11
    patcher.Seek(0x00C66534);
    patcher.WriteInst(Inst_FADD_S11_S12_S9);  // FADD S11, S12, S9
    patcher.Seek(0x00C6656C);
    patcher.BranchLinkInst(reinterpret_cast<void*>(render_fn));

    // Draw Middle
    patcher.WriteInst(Inst_FADD_S0_S10_S8); // FADD S0,  S10, S8 
    patcher.WriteInst(Inst_FMOV_S1_S11);    // FMOV S1,  S11 
    patcher.Seek(0x00C665B8);
    patcher.BranchLinkInst(reinterpret_cast<void*>(render_fn));

}
#endif
}

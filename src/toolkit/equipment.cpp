#include <exl/lib.hpp>

#include <KingSystem/ActorSystem/actActorSystem.h>
#include <KingSystem/ActorSystem/actPlayerInfo.h>
#include <nn/os.h>
#include <nn/time.h>

#include "toolkit/pmdm.hpp"
#include "toolkit/scoped_lock.hpp"
#include "toolkit/tcp.hpp"

namespace botw::toolkit::equipment {

static bool s_initialized = false;
static nn::os::MutexType s_mutex;
static void* s_uimanager = nullptr;
static bool s_paused = false;
static bool s_need_sync = false;

// clang-format off
// This enables the actors and put them on the player
HOOK_DEFINE_TRAMPOLINE(player_m362_hook) {
    static void Callback(void* player) {
        ScopedLock lock(&s_mutex);
        Orig(player);
    }
};
// This is called when unpausing. Returns 1 when equipments are ready
HOOK_DEFINE_TRAMPOLINE(uiman_auto12_hook) {
    static bool Callback(void* x) {
        // unpause
        tcp::sendf("uiauto2 called\n");
        ScopedLock lock(&s_mutex);
        s_uimanager = x;
        s_paused = false;
        bool r = Orig(x);
        tcp::sendf("uiauto2 returned %d\n", r);
        return r;
    }
};
// This is called when pressing dpad. Returns 1 if quick menu is brought up
HOOK_DEFINE_TRAMPOLINE(uiman_unk_hook) {
    static bool Callback(void* x) {
        // pause
        tcp::sendf("uiunk called\n");
        ScopedLock lock(&s_mutex);
        bool r = Orig(x);
        tcp::sendf("uiunk returned %d\n", r);
        if (r) {
            s_paused = true;
            if (s_need_sync) {
                PmdmAccess pmdm;
                if (!pmdm.is_nullptr()) {
                    tcp::sendf("-- creating equipment\n");
                    pmdm->createPlayerEquipment();
                    s_need_sync = false;
                }
            }
        }
        return r;
    }
};

// clang-format on

void init() {
    if (s_initialized) {
        return;
    }
    s_initialized = true;
    nn::os::InitializeMutex(&s_mutex, true /*recursive*/, 0);
    player_m362_hook::InstallAtOffset(0x00F4D1A0);
    uiman_auto12_hook::InstallAtOffset(0x01203730);
    uiman_unk_hook::InstallAtOffset(0x0121B960);
}

int sync_with_pmdm() {
    tcp::sendf("sync_with_pmdm\n");
    if (!s_uimanager) {
        tcp::sendf("-- uimanager is null, cannot sync!\n");
        return -1;
    }
    if (!s_initialized) {
        tcp::sendf("-- not initialized!\n");
        return -1;
    }
    PmdmAccess pmdm;
    if (pmdm.is_nullptr()) {
        tcp::sendf("-- pmdm is null, cannot sync!\n");
        return -1;
    }
    ScopedLock lock(&s_mutex);
    if (!s_paused) {
        tcp::sendf("-- not paused, scheduling update later\n");
        s_need_sync = true;
        return 1;
    }
    tcp::sendf("-- paused, creating equipment\n");
    pmdm->createPlayerEquipment();
    return 0;
}
}

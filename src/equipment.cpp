#include <megaton/hook.h>

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

// This enables the actors and put them on the player
struct hook_trampoline_(player_m362) {
    target_offset_(0x00F4D1A0);
    static void call(void* player) {
        ScopedLock lock(&s_mutex);
        call_original(player);
    }
};
// This is called when unpausing. Returns 1 when equipments are ready
struct hook_trampoline_(uiman_auto12) {
    target_offset_(0x01203730)
    static bool call(void* x) {
        // unpause
        tcp::sendf("uiauto2 called\n");
        ScopedLock lock(&s_mutex);
        s_uimanager = x;
        s_paused = false;
        bool r = call_original(x);
        tcp::sendf("uiauto2 returned %d\n", r);
        return r;
    }
};
// This is called when pressing dpad. Returns 1 if quick menu is brought up
struct hook_trampoline_(uiman_unk) {
    target_offset_(0x0121B960)
    static bool call(void* x) {
        // pause
        tcp::sendf("uiunk called\n");
        ScopedLock lock(&s_mutex);
        bool r = call_original(x);
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

void init() {
    if (s_initialized) {
        return;
    }
    s_initialized = true;
    nn::os::InitializeMutex(&s_mutex, true /*recursive*/, 0);
    player_m362::install();
    uiman_auto12::install();
    uiman_unk::install();
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

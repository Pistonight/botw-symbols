/**
 * Own scoped lock implementation because CS stuff is inlined in 1.6
 */
#pragma once
#include <nn/os.h>

namespace botw::toolkit {
class ScopedLock {
public:
    ScopedLock(nn::os::MutexType* mutex) : m_mutex(mutex), m_locked(false) {
        if (m_mutex) {
            nn::os::LockMutex(m_mutex);
            m_locked = true;
        }
    }
    ~ScopedLock() {
        if (m_locked) {
            nn::os::UnlockMutex(m_mutex);
        }
    }

private:
    nn::os::MutexType* m_mutex;
    bool m_locked;
};

}; // namespace botw::toolkit

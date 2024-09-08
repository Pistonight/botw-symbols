/**
 * A name-value pair
 */
#pragma once
#include <prim/seadSafeString.h>

#define _named(x) #x, x

namespace botw::mem {

template <typename T, u32 L>
class NamedValue {
public:
    NamedValue(T initial) : m_value(initial) { m_name[0] = '\0'; }

    void set_name(const char* string) {
        strncpy(m_name, string, L);
        m_name[L - 1] = '\0';
    }

    bool name_matches(const char* string) const {
        return strncmp(m_name, string, L) == 0;
    }

    void clear_name() { m_name[0] = '\0'; }
    bool is_name_empty() const { return m_name[0] == '\0'; }
    const char* name() const { return m_name; }
    char* name() { return m_name; }
    T value() const { return m_value; }
    T* value_ptr() { return &m_value; }
    void set_value(T value) { m_value = value; }

    void set(const char* string, T value) {
        set_name(string);
        set_value(value);
    }

private:
    char m_name[L];
    T m_value;
};
}  // namespace botw::savs

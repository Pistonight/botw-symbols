/**
 * Fix-sized string buffers
 *
 * Often there are times where using sead::Strings do not work.
 * This implementation has no alloc and no external dependencies.
 */
#pragma once

#include <cstdint>
#include <cstring>
#include <stdarg.h>
#include <stdio.h>

namespace botw::mem {

template <typename T, uint32_t L> class StringBufferBase {
public:
    StringBufferBase() { clear(); }
    void clear() {
        m_len = 0;
        ensure_termination();
    }
    const T* content() const { return m_content; }
    T* content() { return m_content; }
    T* last() { return m_content + m_len; }
    uint32_t len() const {
        return m_len;
    } // may include null byte in the middle
    uint32_t capacity() const { return L; }

    void increase_length(uint32_t size) {
        if (size > 0) {
            m_len += size;
            if (m_len > L) {
                m_len = L;
            }
        }
        ensure_termination();
    }

    void append(const T* text) {
        strncpy(last(), text, L - m_len);
        increase_length(strlen(text));
    }

    void copy(const T* text) {
        clear();
        append(text);
    }

    void ensure_termination() { m_content[m_len] = '\0'; }

    bool index_of(T search, uint32_t from, uint32_t* outIndex) {
        for (uint32_t i = from; i < m_len; i++) {
            if (m_content[i] == search) {
                *outIndex = i;
                return true;
            }
        }
        return false;
    }

    void set(uint32_t i, T c) {
        if (i < m_len) {
            m_content[i] = c;
        }
    }

    void delete_front(uint32_t size) {
        if (size > m_len) {
            clear();
            return;
        }
        for (uint32_t i = 0; i < m_len - size; i++) {
            m_content[i] = m_content[i + size];
        }
        m_len -= size;
        ensure_termination();
    }

    void delete_end(uint32_t size) {
        if (m_len <= size) {
            m_len = 0;
        } else {
            m_len -= size;
        }
        ensure_termination();
    }

protected:
    T m_content[L + 1];
    uint32_t m_len;
};

template <uint32_t L> class StringBuffer : public StringBufferBase<char, L> {
public:
    void appendf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        int size = vsnprintf(this->last(), L - this->m_len, format, args);
        va_end(args);
        this->increase_length(size);
    }
};

template <uint32_t L>
class WStringBuffer : public StringBufferBase<char16_t, L> {
public:
    void copy_from(const char* text) {
        for (uint32_t i = 0; i < L; i++) {
            this->m_content[i] = text[i];
            if (text[i] == '\0') {
                this->m_len = i;
                return;
            }
        }
        this->m_len = L;
        this->ensure_termination();
    }
};

} // namespace botw::mem

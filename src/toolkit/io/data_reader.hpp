/**
 * Read data from a file that was wrtten with DataWriter
 */
#pragma once

#include "toolkit/io/file.hpp"
#include "toolkit/mem/named_value.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

namespace botw::io {
class DataReader {
public:
    DataReader(const char* path);
    ~DataReader() = default;

    bool is_successful() const { return m_success; }

    void read_bool(bool* out) {
        u64 i_value;
        read_integer(&i_value);
        if (!m_success) {
            return;
        }
        *out = i_value != 0;
    }

    void read_integer(u64* out) {
        if (m_success) {
            m_success = do_read_integer(out);
        }
    }

    void read_integer(u32* out) {
        u64 i_value;
        if (!do_read_integer(&i_value)) {
            m_success = false;
            return;
        }

        u32 truncated = (u32)(i_value);
        *out = truncated;
    }

    void read_integer(s32* out) { read_integer(reinterpret_cast<u32*>(out)); }

    void read_string(char* out_string, const u32 max_length) {
        if (m_success) {
            m_success = do_read_string(out_string, max_length);
        }
    }
    void read_float(f32* value) {
        u32 i_value;
        read_integer(&i_value);
        if (!m_success) {
            return;
        }
        *value = reinterpret_cast<f32&>(i_value);
    }
    template <typename T> void read_integer_array(T* out_array, u32 size) {
        read_integer(out_array);
        if (!m_success) {
            return;
        }
        for (u32 i = 1; i < size; i++) {
            read_integer(out_array + i);
            if (!m_success) {
                return;
            }
        }
    }
    void read_float_array(f32* out_array, u32 size) {
        read_float(out_array);
        if (!m_success) {
            return;
        }
        for (u32 i = 1; i < size; i++) {
            read_float(out_array + i);
            if (!m_success) {
                return;
            }
        }
    }
    template <typename T, u32 L>
    void read_named_integer(mem::NamedValue<T, L>& value) {
        read_string(value.name(), L);
        read_integer(value.value_ptr());
    }
    template <typename T, u32 L>
    void read_named_float(const mem::NamedValue<T, L>& value) {
        read_string(value.name(), L);
        read_float(value.value_ptr());
    }

private:
    bool m_success = true;
    io::File m_file;
    io::FileBuffer m_buffer;

    bool do_read_line(u32* out_line_length);
    bool do_read_integer(u64* out_value);
    bool do_read_string(char* out_string, const u32 max_length);
};
} // namespace botw::io
#pragma GCC diagnostic pop

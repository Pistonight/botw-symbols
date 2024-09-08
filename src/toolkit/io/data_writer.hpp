#pragma once

#include "toolkit/io/file.hpp"
#include "toolkit/mem/named_value.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

namespace botw::io {
class DataWriter {
public:
    DataWriter(const char* path);
    ~DataWriter() = default;

    bool is_successful() const { return m_success; }

    void write_bool(const char* field_name, const bool value) {
        if (m_success) {
            m_success = do_write_number(field_name, value ? 1 : 0);
        }
    }

    void write_integer(const char* field_name, const u64 value) {
        if (m_success) {
            m_success = do_write_number(field_name, value);
        }
    }

    void write_string(const char* field_name, const char* string) {
        if (m_success) {
            m_success = do_write_string(field_name, string);
        }
    }

    void write_float(const char* fieldName, const f32 value) {
        write_integer(fieldName, reinterpret_cast<const u32&>(value));
    }

    template <typename T>
    void write_integer_array(const char* field_name, const T* array, u32 size) {
        write_integer(field_name, array[0]);
        if (!m_success) {
            return;
        }
        for (u32 i = 1; i < size; i++) {
            write_integer(" ---", array[i]);
            if (!m_success) {
                return;
            }
        }
    }
    void write_float_array(const char* field_name, const f32* array, u32 size) {
        write_float(field_name, array[0]);
        if (!m_success) {
            return;
        }
        for (u32 i = 1; i < size; i++) {
            write_float(" ---", array[i]);
            if (!m_success) {
                return;
            }
        }
    }

    template <typename T, u32 L>
    void write_named_integer(const char* field_name, const mem::NamedValue<T, L>& value) {
        write_string("Name of value below", value.name());
        write_integer(field_name, value.value());
    }

    template <typename T, u32 L>
    void write_named_float(const char* field_name, const mem::NamedValue<T, L>& value) {
        write_string("Name of value below", value.name());
        write_float(field_name, value.value());
    }

    void flush();

private:
    bool m_success = true;
    io::File m_file;
    bool m_is_using_first_buffer = true;
    io::FileBuffer m_buffer;
    io::FileBuffer m_buffer2;

    io::FileBuffer& active_buffer() {
        if (m_is_using_first_buffer) {
            return m_buffer;
        }
        return m_buffer2;
    }

    io::FileBuffer& inactive_buffer() {
        if (m_is_using_first_buffer) {
            return m_buffer2;
        }
        return m_buffer;
    }

    bool append_inactive_to_active();
    bool do_write_number(const char* field_name, const u64 value);
    bool do_write_string(const char* field_name, const char* string);
};

}  // namespace botw::savs
#pragma GCC diagnostic pop

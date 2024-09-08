#include <stdlib.h>

#include "toolkit/io/data_writer.hpp"

namespace botw::io {

DataWriter::DataWriter(const char* path) : m_file(path) {
    if (!m_file.exists()) {
        if (!m_file.create()) {
            m_success = false;
            return;
        }
    }
    if (!m_file.open()) {
        m_success = false;
    }
}
bool DataWriter::append_inactive_to_active() {
    auto& inactive = inactive_buffer();
    auto& active = active_buffer();
    if (active.len() + inactive.len() > active.capacity()) {
        bool result = m_file.write(active);
        active.clear();
        m_is_using_first_buffer = !m_is_using_first_buffer;
        if (!result) {
            return false;
        }
    } else {
        active.append(inactive.content());
        inactive.clear();
    }
    return true;
}

bool DataWriter::do_write_number(const char* field_name, u64 value) {
    auto& inactive = inactive_buffer();
    inactive.clear();
    inactive.appendf("0x%016x", value);
    inactive.appendf("# %s\n", field_name);

    return append_inactive_to_active();
}

bool DataWriter::do_write_string(const char* field_name, const char* string) {
    auto& inactive = inactive_buffer();
    inactive.clear();
    inactive.appendf("%s", string);
    inactive.appendf("# %s\n", field_name);

    return append_inactive_to_active();
}

void DataWriter::flush() {
    if (!m_success) {
        return;
    }
    auto& active = active_buffer();
    if (active.len() > 0) {
        m_success = m_file.write(active);
        active.clear();
    }
}

} // namespace botw::io

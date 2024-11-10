/**
 * Wrappers to access private members of PauseMenuDataMgr and PouchItem
 */
#pragma once

#include <Game/UI/uiPauseMenuDataMgr.h>
#include <cstdint>

#include "toolkit/io/data_reader.hpp"
#include "toolkit/io/data_writer.hpp"
#include "toolkit/mem/internal_ptr.hpp"
#include "toolkit/mem/string.hpp"

namespace sead {
template <typename T> class OffsetList;
class ListNode;
} // namespace sead

namespace uking::ui {
class PauseMenuDataMgr;
class PouchItem;
enum class PouchItemType;
} // namespace uking::ui

namespace botw::toolkit {

using Pmdm = uking::ui::PauseMenuDataMgr;
using PouchItem = uking::ui::PouchItem;
using PouchItemType = uking::ui::PouchItemType;
using ItemUse = uking::ui::ItemUse;

class PmdmAccess {

public:
    PmdmAccess() { bind(); }
    bool bind();

    /**
     * Calls updateEquippedItemArray. Uses decompiled version in 1.6.0
     */
    void update_equipped_item_array();
    PouchItem* get_equipped_item(PouchItemType t);
    sead::OffsetList<PouchItem>& get_items();
    void set_offset_slots(int32_t slots);
    int32_t get_offset_slots();

    /**
     * Calls saveToGameData with current items
     */
    void save_to_gdt();

    Pmdm* operator->() { return m_pmdm; }
    Pmdm* operator*() { return m_pmdm; }
    bool is_nullptr() { return m_pmdm == nullptr; }

private:
    Pmdm* m_pmdm;
};

class PouchItemAccess {
public:
    PouchItemAccess(PouchItem* item) : m_item(item) {}

    sead::ListNode* get_list_node();
    void set_value(uint32_t value);
    void set_equipped(bool equipped);

    PouchItem* operator->() { return m_item; }
    PouchItem* operator*() { return m_item; }

private:
    PouchItem* m_item;
};

class PouchItemSaveState {
public:
    void read_from(PmdmAccess pmdm, PouchItemAccess item);
    void write_to(PmdmAccess pmdm) const;
    void read_from_file(io::DataReader& reader);
    void write_to_file(io::DataWriter& writer, int32_t idx) const;

    mem::internal_ptr<PouchItem> m_target;
    mem::internal_ptr<sead::ListNode> m_list_node_prev;
    mem::internal_ptr<sead::ListNode> m_list_node_next;
    PouchItemType m_type;
    ItemUse m_item_use;
    int32_t m_value;
    bool m_equipped;
    bool m_in_inventory;
    mem::StringBuffer<64> m_name;
    int32_t m_data[3];
    float_t m_data2[2];
    mem::StringBuffer<64> m_ingr[5];
};

class PmdmSaveState {
public:
    bool read_from(PmdmAccess pmdm);
    void write_to(PmdmAccess pmdm, bool sync_gamedata) const;
    void read_from_file(io::DataReader& reader);
    void write_to_file(io::DataWriter& writer) const;

    // mLists.list1
    mem::internal_ptr<sead::ListNode> m_items_list1_startend_node_prev;
    mem::internal_ptr<sead::ListNode> m_items_list1_startend_node_next;
    int32_t m_items_list1_count;
    int32_t m_items_list1_offset;
    // mLists.list2
    mem::internal_ptr<sead::ListNode> m_items_list2_startend_node_prev;
    mem::internal_ptr<sead::ListNode> m_items_list2_startend_node_next;
    int32_t m_items_list2_count;
    int32_t m_items_list2_offset;
    // items
    PouchItemSaveState m_item_buffer[420];
    // list heads
    mem::internal_ptr<PouchItem*> m_list_heads[7];
    mem::internal_ptr<PouchItem> m_tabs[50];
    PouchItemType m_tabs_type[50];
    mem::internal_ptr<PouchItem> m_last_added_item;
    int32_t m_last_added_item_tab;
    int32_t m_last_added_item_slot;
    int32_t m_num_tabs;
    mem::internal_ptr<PouchItem> m_grabbed_info_item[5];
    bool m_grabbed_info_b1[5];
    bool m_grabbed_info_b2[5];
    mem::internal_ptr<PouchItem> m_item_444f0;
    int32_t m_444f8;
    int32_t m_444fc;
    int32_t m_44500;
    uint32_t m_44504;
    uint32_t m_44508;
    uint32_t m_4450c;
    uint32_t m_44510;
    uint32_t m_44514;
    mem::internal_ptr<PouchItem> m_rito_soul;
    mem::internal_ptr<PouchItem> m_goron_soul;
    mem::internal_ptr<PouchItem> m_zora_soul;
    mem::internal_ptr<PouchItem> m_gerudo_soul;
    bool m_can_see_health_bar;
    PouchItemSaveState m_newly_added_item;
    bool m_is_pouch_for_quest;
    mem::internal_ptr<PouchItem> m_equipped_weapons[4];
    uking::ui::PouchCategory m_category_to_sort;
};

} // namespace botw::toolkit

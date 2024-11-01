
// Need private field access
#define private public
#define protected public
#include <Game/UI/uiPauseMenuDataMgr.h>
#include <container/seadOffsetList.h>
#undef protected
#undef private

#include <cstdio>

#if BOTW_VERSION != 160
#include <prim/seadScopedLock.h>
#endif

#include "toolkit/mem/named_value.hpp"
#include "toolkit/pmdm.hpp"
#if BOTW_VERSION == 160
#include "toolkit/scoped_lock.hpp"
#endif
#include "toolkit/sead/list.hpp"
#include "toolkit/tcp.hpp"

namespace botw::toolkit {

bool PmdmAccess::bind() {
    m_pmdm = uking::ui::PauseMenuDataMgr::instance();
    return m_pmdm != nullptr;
}

void PmdmAccess::update_equipped_item_array() {
#if BOTW_VERSION == 160
    // inlined, provide our own impl

    // offset of array: 0x44E70
    m_pmdm->mEquippedWeapons.fill({});
    ScopedLock lock(&m_pmdm->mCritSection.mCriticalSectionInner);
    for (auto& item : m_pmdm->getItems()) {
        if (item.getType() > PouchItemType::Shield)
            break;
        if (item.isEquipped())
            m_pmdm->mEquippedWeapons[u32(item.getType())] = &item;
    }
#else
    m_pmdm->updateEquippedItemArray();
#endif
}

PouchItem* PmdmAccess::get_equipped_item(PouchItemType t) {
    return m_pmdm->mEquippedWeapons[(int)t];
}

::sead::OffsetList<PouchItem>& PmdmAccess::get_items() {
    return m_pmdm->mItemLists.list1;
}

int32_t PmdmAccess::get_offset_slots() {
    auto& items = get_items();
    s32 count = items.size();
    s32 real_count = toolkit::sead::get_list_real_size(&items);
    return real_count - count;
}

void PmdmAccess::set_offset_slots(int32_t slots) {
    auto& items = get_items();
    s32 real_size = toolkit::sead::get_list_real_size(&items);
    s32 size = real_size - slots;
    m_pmdm->mItemLists.list1.mCount = size;
}

void PmdmAccess::save_to_gdt() { m_pmdm->saveToGameData(m_pmdm->getItems()); }

::sead::ListNode* PouchItemAccess::get_list_node() {
    return &m_item->mListNode;
}

void PouchItemAccess::set_value(u32 value) { m_item->mValue = value; }

void PouchItemAccess::set_equipped(bool equipped) {
    m_item->mEquipped = equipped;
}

void PouchItemSaveState::read_from(PmdmAccess pmdm, PouchItemAccess item)

{
    m_target = mem::internal_ptr(*pmdm, *item);
    m_list_node_prev = mem::internal_ptr(*pmdm, item.get_list_node()->prev());
    m_list_node_next = mem::internal_ptr(*pmdm, item.get_list_node()->next());
    m_type = item->getType();
    m_item_use = item->getItemUse();
    m_value = item->getValue();
    m_equipped = item->isEquipped();
    m_in_inventory = item->isInInventory();
    m_name.copy(item->getName().cstr());
    auto& cook_data = item->getCookData();
    m_data[0] = cook_data.mHealthRecover;
    m_data[1] = cook_data.mEffectDuration;
    m_data[2] = cook_data.mSellPrice;
    m_data2[0] = cook_data.mEffect.x;
    m_data2[1] = cook_data.mEffect.y;
    for (int i = 0; i < 5; i++) {
        m_ingr[i].copy(item->mIngredients[i]->cstr());
    }
}

void PouchItemSaveState::write_to(PmdmAccess pmdm) const {
    auto* item = m_target.hydrate(*pmdm);
    item->mListNode.mPrev = m_list_node_prev.hydrate(*pmdm);
    item->mListNode.mNext = m_list_node_next.hydrate(*pmdm);
    item->mType = m_type;
    item->mItemUse = m_item_use;
    item->mValue = m_value;
    item->mEquipped = m_equipped;
    item->mInInventory = m_in_inventory;
    item->mName = m_name.content();
    tcp::sendf("-- item name: %s\n", item->mName.cstr());
    auto& cook_data = item->getCookData();
    cook_data.mHealthRecover = m_data[0];
    cook_data.mEffectDuration = m_data[1];
    cook_data.mSellPrice = m_data[2];
    cook_data.mEffect.x = m_data2[0];
    cook_data.mEffect.y = m_data2[1];
    for (int i = 0; i < 5; i++) {
        *item->mIngredients[i] = m_ingr[i].content();
    }
}

void PouchItemSaveState::read_from_file(io::DataReader& reader) {
    bool is_nullptr = false;
    uintptr_t offset = 0;
    reader.read_bool(&is_nullptr);
    reader.read_integer(&offset);
    m_target.set(is_nullptr, offset);
    reader.read_bool(&is_nullptr);
    reader.read_integer(&offset);
    m_list_node_prev.set(is_nullptr, offset);
    reader.read_bool(&is_nullptr);
    reader.read_integer(&offset);
    m_list_node_next.set(is_nullptr, offset);
    uint64_t enum_value = 0;
    reader.read_integer(&enum_value);
    m_type = static_cast<PouchItemType>(enum_value);
    reader.read_integer(&enum_value);
    m_item_use = static_cast<ItemUse>(enum_value);
    reader.read_integer(&m_value);
    reader.read_bool(&m_equipped);
    reader.read_bool(&m_in_inventory);
    m_name.clear();
    reader.read_string(m_name.content(), 64);
    reader.read_integer(&m_data[0]);
    reader.read_integer(&m_data[1]);
    reader.read_integer(&m_data[2]);
    reader.read_float(&m_data2[0]);
    reader.read_float(&m_data2[1]);
    for (int i = 0; i < 5; i++) {
        m_ingr[i].clear();
        reader.read_string(m_ingr[i].content(), 64);
    }
}

void PouchItemSaveState::write_to_file(io::DataWriter& writer,
                                       int32_t idx) const {
    if (idx < 0) {
        writer.write_bool("item - target.is_nullptr", m_target.is_nullptr());
    } else {
        char name_buffer[64];
        name_buffer[0] = '\0';
        snprintf(name_buffer, sizeof(name_buffer) - 1,
                 "item (%d) target.is_nullptr", idx);
        name_buffer[sizeof(name_buffer) - 1] = '\0';
        writer.write_bool(name_buffer, m_target.is_nullptr());
    }
    writer.write_integer("target.offset", m_target.offset());
    writer.write_bool(_named(m_list_node_prev.is_nullptr()));
    writer.write_integer(_named(m_list_node_prev.offset()));
    writer.write_bool(_named(m_list_node_next.is_nullptr()));
    writer.write_integer(_named(m_list_node_next.offset()));
    writer.write_integer(_named(static_cast<uint64_t>(m_type)));
    writer.write_integer(_named(static_cast<uint64_t>(m_item_use)));
    writer.write_integer(_named(m_value));
    writer.write_bool(_named(m_equipped));
    writer.write_bool(_named(m_in_inventory));
    writer.write_string(_named(m_name.content()));
    writer.write_integer("health_recover", m_data[0]);
    writer.write_integer("effect_duration", m_data[1]);
    writer.write_integer("sell_price", m_data[2]);
    writer.write_float("effect_x", m_data2[0]);
    writer.write_float("effect_y", m_data2[1]);
    writer.write_string(_named(m_ingr[0].content()));
    writer.write_string(_named(m_ingr[1].content()));
    writer.write_string(_named(m_ingr[2].content()));
    writer.write_string(_named(m_ingr[3].content()));
    writer.write_string(_named(m_ingr[4].content()));
}

bool PmdmSaveState::read_from(PmdmAccess pmdm) {
#if BOTW_VERSION == 160
    ScopedLock lock(&pmdm->mCritSection.mCriticalSectionInner);
#else
    ::sead::ScopedLock lock(&pmdm->mCritSection);
#endif
    // lists
    m_items_list1_startend_node_prev =
        mem::internal_ptr(*pmdm, pmdm->mItemLists.list1.mStartEnd.mPrev);
    m_items_list1_startend_node_next =
        mem::internal_ptr(*pmdm, pmdm->mItemLists.list1.mStartEnd.mNext);
    m_items_list1_count = pmdm->mItemLists.list1.mCount;
    m_items_list1_offset = pmdm->mItemLists.list1.mOffset;
    m_items_list2_startend_node_prev =
        mem::internal_ptr(*pmdm, pmdm->mItemLists.list2.mStartEnd.mPrev);
    m_items_list2_startend_node_next =
        mem::internal_ptr(*pmdm, pmdm->mItemLists.list2.mStartEnd.mNext);
    m_items_list2_count = pmdm->mItemLists.list2.mCount;
    m_items_list2_offset = pmdm->mItemLists.list2.mOffset;
    // items
    for (int i = 0; i < 420; i++) {
        m_item_buffer[i].read_from(pmdm, &pmdm->mItemLists.buffer[i]);
    }
    // list heads
    for (int i = 0; i < 7; i++) {
        PouchItem** x = pmdm->mListHeads[i];
        m_list_heads[i] = mem::internal_ptr(*pmdm, x);
    }
    for (int i = 0; i < 50; i++) {
        PouchItem* tab = pmdm->mTabs[i];
        m_tabs[i] = mem::internal_ptr(*pmdm, tab);
        m_tabs_type[i] = pmdm->mTabsType[i];
    }
    m_last_added_item = mem::internal_ptr(*pmdm, pmdm->mLastAddedItem);
    m_last_added_item_tab = pmdm->mLastAddedItemTab;
    m_last_added_item_slot = pmdm->mLastAddedItemSlot;
    m_num_tabs = pmdm->mNumTabs;
    for (int i = 0; i < 5; i++) {
        m_grabbed_info_item[i] =
            mem::internal_ptr(*pmdm, pmdm->mGrabbedItems[i].item);
        m_grabbed_info_b1[i] = pmdm->mGrabbedItems[i]._8;
        m_grabbed_info_b2[i] = pmdm->mGrabbedItems[i]._9;
    }
    m_item_444f0 = mem::internal_ptr(*pmdm, pmdm->mItem_444f0);
    m_444f8 = pmdm->_444f8;
    m_444fc = pmdm->_444fc;
    m_44500 = pmdm->_44500;
    m_44504 = pmdm->_44504;
    m_44508 = pmdm->_44508;
    m_4450c = pmdm->_4450c;
    m_44510 = pmdm->_44510;
    m_44514 = pmdm->_44514;
    m_rito_soul = mem::internal_ptr(*pmdm, pmdm->mRitoSoulItem);
    m_goron_soul = mem::internal_ptr(*pmdm, pmdm->mGoronSoulItem);
    m_zora_soul = mem::internal_ptr(*pmdm, pmdm->mZoraSoulItem);
    m_gerudo_soul = mem::internal_ptr(*pmdm, pmdm->mGerudoSoulItem);
    m_can_see_health_bar = pmdm->mCanSeeHealthBar;
    m_newly_added_item.read_from(pmdm, &pmdm->mNewlyAddedItem);
    m_is_pouch_for_quest = pmdm->mIsPouchForQuest;
    for (int i = 0; i < 4; i++) {
        m_equipped_weapons[i] =
            mem::internal_ptr(*pmdm, pmdm->mEquippedWeapons[i]);
    }
    m_category_to_sort = pmdm->mCategoryToSort;

    return true;
}

void PmdmSaveState::write_to(PmdmAccess pmdm, bool sync_gamedata) const {
#if BOTW_VERSION == 160
    ScopedLock lock(&pmdm->mCritSection.mCriticalSectionInner);
#else
    ::sead::ScopedLock lock(&pmdm->mCritSection);
#endif
    pmdm->mItemLists.list1.mStartEnd.mPrev =
        m_items_list1_startend_node_prev.hydrate(*pmdm);
    pmdm->mItemLists.list1.mStartEnd.mNext =
        m_items_list1_startend_node_next.hydrate(*pmdm);
    pmdm->mItemLists.list1.mCount = m_items_list1_count;
    pmdm->mItemLists.list1.mOffset = m_items_list1_offset;
    pmdm->mItemLists.list2.mStartEnd.mPrev =
        m_items_list2_startend_node_prev.hydrate(*pmdm);
    pmdm->mItemLists.list2.mStartEnd.mNext =
        m_items_list2_startend_node_next.hydrate(*pmdm);
    pmdm->mItemLists.list2.mCount = m_items_list2_count;
    pmdm->mItemLists.list2.mOffset = m_items_list2_offset;
    for (int i = 0; i < 420; i++) {
        m_item_buffer[i].write_to(pmdm);
    }
    for (int i = 0; i < 7; i++) {
        pmdm->mListHeads[i] = m_list_heads[i].hydrate(*pmdm);
    }
    for (int i = 0; i < 50; i++) {
        pmdm->mTabs[i] = m_tabs[i].hydrate(*pmdm);
        pmdm->mTabsType[i] = m_tabs_type[i];
    }
    pmdm->mLastAddedItem = m_last_added_item.hydrate(*pmdm);
    pmdm->mLastAddedItemTab = m_last_added_item_tab;
    pmdm->mLastAddedItemSlot = m_last_added_item_slot;
    pmdm->mNumTabs = m_num_tabs;
    for (int i = 0; i < 5; i++) {
        pmdm->mGrabbedItems[i].item = m_grabbed_info_item[i].hydrate(*pmdm);
        pmdm->mGrabbedItems[i]._8 = m_grabbed_info_b1[i];
        pmdm->mGrabbedItems[i]._9 = m_grabbed_info_b2[i];
    }
    pmdm->mItem_444f0 = m_item_444f0.hydrate(*pmdm);
    pmdm->_444f8 = m_444f8;
    pmdm->_444fc = m_444fc;
    pmdm->_44500 = m_44500;
    pmdm->_44504 = m_44504;
    pmdm->_44508 = m_44508;
    pmdm->_4450c = m_4450c;
    pmdm->_44510 = m_44510;
    pmdm->_44514 = m_44514;
    pmdm->mRitoSoulItem = m_rito_soul.hydrate(*pmdm);
    pmdm->mGoronSoulItem = m_goron_soul.hydrate(*pmdm);
    pmdm->mZoraSoulItem = m_zora_soul.hydrate(*pmdm);
    pmdm->mGerudoSoulItem = m_gerudo_soul.hydrate(*pmdm);
    pmdm->mCanSeeHealthBar = m_can_see_health_bar;
    m_newly_added_item.write_to(pmdm);
    pmdm->mIsPouchForQuest = m_is_pouch_for_quest;
    for (int i = 0; i < 4; i++) {
        pmdm->mEquippedWeapons[i] = m_equipped_weapons[i].hydrate(*pmdm);
    }
    pmdm->mCategoryToSort = m_category_to_sort;

    if (sync_gamedata) {
        pmdm.save_to_gdt();
    }
}

void PmdmSaveState::read_from_file(io::DataReader& r) {
    bool is_nullptr = false;
    uintptr_t offset = 0;
    // lists
    r.read_bool(&is_nullptr);
    r.read_integer(&offset);
    m_items_list1_startend_node_prev.set(is_nullptr, offset);
    r.read_bool(&is_nullptr);
    r.read_integer(&offset);
    m_items_list1_startend_node_next.set(is_nullptr, offset);
    r.read_integer(&m_items_list1_count);
    r.read_integer(&m_items_list1_offset);

    r.read_bool(&is_nullptr);
    r.read_integer(&offset);
    m_items_list2_startend_node_prev.set(is_nullptr, offset);
    r.read_bool(&is_nullptr);
    r.read_integer(&offset);
    m_items_list2_startend_node_next.set(is_nullptr, offset);
    r.read_integer(&m_items_list2_count);
    r.read_integer(&m_items_list2_offset);
    // items
    for (int i = 0; i < 420; i++) {
        m_item_buffer[i].read_from_file(r);
    }
    for (int i = 0; i < 7; i++) {
        r.read_bool(&is_nullptr);
        r.read_integer(&offset);
        m_list_heads[i].set(is_nullptr, offset);
    }
    for (int i = 0; i < 50; i++) {
        r.read_bool(&is_nullptr);
        r.read_integer(&offset);
        m_tabs[i].set(is_nullptr, offset);
        uint64_t enum_value = 0;
        r.read_integer(&enum_value);
        m_tabs_type[i] = static_cast<PouchItemType>(enum_value);
    }
    r.read_bool(&is_nullptr);
    r.read_integer(&offset);
    m_last_added_item.set(is_nullptr, offset);
    r.read_integer(&m_last_added_item_tab);
    r.read_integer(&m_last_added_item_slot);
    r.read_integer(&m_num_tabs);
    for (int i = 0; i < 5; i++) {
        r.read_bool(&is_nullptr);
        r.read_integer(&offset);
        m_grabbed_info_item[i].set(is_nullptr, offset);
        r.read_bool(&m_grabbed_info_b1[i]);
        r.read_bool(&m_grabbed_info_b2[i]);
    }
    r.read_bool(&is_nullptr);
    r.read_integer(&offset);
    m_item_444f0.set(is_nullptr, offset);
    r.read_integer(&m_444f8);
    r.read_integer(&m_444fc);
    r.read_integer(&m_44500);
    r.read_integer(&m_44504);
    r.read_integer(&m_44508);
    r.read_integer(&m_4450c);
    r.read_integer(&m_44510);
    r.read_integer(&m_44514);
    r.read_bool(&is_nullptr);
    r.read_integer(&offset);
    m_rito_soul.set(is_nullptr, offset);
    r.read_bool(&is_nullptr);
    r.read_integer(&offset);
    m_goron_soul.set(is_nullptr, offset);
    r.read_bool(&is_nullptr);
    r.read_integer(&offset);
    m_zora_soul.set(is_nullptr, offset);
    r.read_bool(&is_nullptr);
    r.read_integer(&offset);
    m_gerudo_soul.set(is_nullptr, offset);
    r.read_bool(&m_can_see_health_bar);
    m_newly_added_item.read_from_file(r);
    r.read_bool(&m_is_pouch_for_quest);
    for (int i = 0; i < 4; i++) {
        r.read_bool(&is_nullptr);
        r.read_integer(&offset);
        m_equipped_weapons[i].set(is_nullptr, offset);
    }
    uint64_t enum_value = 0;
    r.read_integer(&enum_value);
    m_category_to_sort = static_cast<uking::ui::PouchCategory>(enum_value);
}

void PmdmSaveState::write_to_file(io::DataWriter& w) const {
    // lists
    tcp::sendf("start writing pmdm to file\n");
    tcp::sendf("-- lists\n");
    w.write_bool(_named(m_items_list1_startend_node_prev.is_nullptr()));
    w.write_integer(_named(m_items_list1_startend_node_prev.offset()));
    w.write_bool(_named(m_items_list1_startend_node_next.is_nullptr()));
    w.write_integer(_named(m_items_list1_startend_node_next.offset()));
    w.write_integer(_named(m_items_list1_count));
    w.write_integer(_named(m_items_list1_offset));
    w.write_bool(_named(m_items_list2_startend_node_prev.is_nullptr()));
    w.write_integer(_named(m_items_list2_startend_node_prev.offset()));
    w.write_bool(_named(m_items_list2_startend_node_next.is_nullptr()));
    w.write_integer(_named(m_items_list2_startend_node_next.offset()));
    w.write_integer(_named(m_items_list2_count));
    w.write_integer(_named(m_items_list2_offset));
    // items
    tcp::sendf("-- items\n");
    for (int i = 0; i < 420; i++) {
        tcp::sendf("-- -- item %d\n", i);
        m_item_buffer[i].write_to_file(w, i);
    }
    tcp::sendf("-- list_head\n");
    char name_buffer[64];
    for (int i = 0; i < 7; i++) {
        name_buffer[0] = '\0';
        snprintf(name_buffer, sizeof(name_buffer) - 1,
                 "list_head (%d) is_nullptr", i);
        name_buffer[sizeof(name_buffer) - 1] = '\0';
        w.write_bool(name_buffer, m_list_heads[i].is_nullptr());
        w.write_integer(_named(m_list_heads[i].offset()));
    }
    tcp::sendf("-- tabs\n");
    for (int i = 0; i < 50; i++) {
        name_buffer[0] = '\0';
        snprintf(name_buffer, sizeof(name_buffer) - 1, "tab (%d) is_nullptr",
                 i);
        name_buffer[sizeof(name_buffer) - 1] = '\0';
        w.write_bool(name_buffer, m_tabs[i].is_nullptr());
        w.write_integer(_named(m_tabs[i].offset()));
        w.write_integer(_named(static_cast<uint64_t>(m_tabs_type[i])));
    }
    tcp::sendf("-- last_added_item\n");
    w.write_bool(_named(m_last_added_item.is_nullptr()));
    w.write_integer(_named(m_last_added_item.offset()));
    w.write_integer(_named(m_last_added_item_tab));
    w.write_integer(_named(m_last_added_item_slot));
    w.write_integer(_named(m_num_tabs));
    tcp::sendf("-- grabbed_item_info\n");
    for (int i = 0; i < 5; i++) {
        name_buffer[0] = '\0';
        snprintf(name_buffer, sizeof(name_buffer) - 1,
                 "grabbed_info_item (%d) is_nullptr", i);
        name_buffer[sizeof(name_buffer) - 1] = '\0';
        w.write_bool(name_buffer, m_grabbed_info_item[i].is_nullptr());
        w.write_integer(_named(m_grabbed_info_item[i].offset()));
        w.write_bool(_named(m_grabbed_info_b1[i]));
        w.write_bool(_named(m_grabbed_info_b2[i]));
    }
    tcp::sendf("-- unknown region\n");
    w.write_bool(_named(m_item_444f0.is_nullptr()));
    w.write_integer(_named(m_item_444f0.offset()));
    w.write_integer(_named(m_444f8));
    w.write_integer(_named(m_444fc));
    w.write_integer(_named(m_44500));
    w.write_integer(_named(m_44504));
    w.write_integer(_named(m_44508));
    w.write_integer(_named(m_4450c));
    w.write_integer(_named(m_44510));
    w.write_integer(_named(m_44514));
    tcp::sendf("-- champion abilities\n");
    w.write_bool(_named(m_rito_soul.is_nullptr()));
    w.write_integer(_named(m_rito_soul.offset()));
    w.write_bool(_named(m_goron_soul.is_nullptr()));
    w.write_integer(_named(m_goron_soul.offset()));
    w.write_bool(_named(m_zora_soul.is_nullptr()));
    w.write_integer(_named(m_zora_soul.offset()));
    w.write_bool(_named(m_gerudo_soul.is_nullptr()));
    w.write_integer(_named(m_gerudo_soul.offset()));
    w.write_bool(_named(m_can_see_health_bar));
    tcp::sendf("-- newly_added_item\n");
    m_newly_added_item.write_to_file(w, -1);
    tcp::sendf("-- is_pouch_for_quest\n");
    w.write_bool(_named(m_is_pouch_for_quest));
    tcp::sendf("-- equipped weapons\n");
    w.write_bool(_named(m_equipped_weapons[0].is_nullptr()));
    w.write_integer(_named(m_equipped_weapons[0].offset()));
    w.write_bool(_named(m_equipped_weapons[1].is_nullptr()));
    w.write_integer(_named(m_equipped_weapons[1].offset()));
    w.write_bool(_named(m_equipped_weapons[2].is_nullptr()));
    w.write_integer(_named(m_equipped_weapons[2].offset()));
    w.write_bool(_named(m_equipped_weapons[3].is_nullptr()));
    w.write_integer(_named(m_equipped_weapons[3].offset()));
    tcp::sendf("-- category_to_sort\n");
    w.write_integer(_named(static_cast<uint64_t>(m_category_to_sort)));
    tcp::sendf("finish writing pmdm to file\n");
}

} // namespace botw::toolkit

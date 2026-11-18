namespace equipment_transfer {
using namespace lantern::equipment;
using Operation=void*(__fastcall*)(void*,void*,uint64_t,uint8_t);
Operation originalEquip=nullptr,originalUnequip=nullptr;
// InventorySlotId is entity:uint32 + index:uint8 + three padding bytes.
constexpr uint64_t slotAddress(uint64_t value) {return value&0xffffffffffULL;}
Stack readStack(const void* data,size_t offset) {
    auto stack=read<Stack>(data,offset);
    // Native ea2e10 treats zero quantity as empty even if the old item remains.
    // Native e93430 reports one instance when persistentId is set.
    if(!stack.item||!stack.count)return {};
    if(stack.persistentId)stack.count=1;
    return stack;
}
void trace(const char* stage,uint64_t address,unsigned status) {
    char message[192]{};
    std::snprintf(message,sizeof(message),"Lantern: equipment %s entity=%u cell=%u status=%u",
        stage,static_cast<unsigned>(address),static_cast<unsigned>((address>>32)&255),status);
    logger(message);
}
bool collect(void* world,uint32_t actor,Snapshot& snapshot,unsigned depth=0) {
    if(depth>4)return false;
    const auto setup=component(world,actor,"keen::ecs::InventorySetup",84);if(!setup.data)return false;
    for(unsigned i=0;i<16;++i){
        const auto entity=read<uint32_t>(setup.data,i*4);if(!entity)break;
        const auto category=setup.data[64+i];
        if(category==5){if(!collect(world,entity,snapshot,depth+1))return false;continue;}
        if(category!=4)continue;
        const auto slots=component(world,entity,"keen::ecs::Inventory",96);if(!slots.data)return false;
        for(unsigned j=0;j<8;++j)snapshot.bag.push_back({uint64_t(entity)|(uint64_t(j)<<32),readStack(slots.data,j*12)});
    }
    return true;
}
bool snapshot(void* transaction,Snapshot& value) {
    uint32_t actor=0;ui<uint32_t*(__fastcall*)(void*,uint32_t*)>(0x9bf9e0)(read<void*>(transaction,0),&actor);
    if(!actor||actor!=equipment_storage::owner.load()||!characterLoaded)return false;
    const auto page=equipment_storage::page.load();const auto world=equipment_storage::world.load();
    const auto inventory=component(world,page,"keen::ecs::Inventory",96);if(!inventory.data)return false;
    value.character=activeCharacter;value.session=1;value.revision=1;
    value.equipped={page,readStack(inventory.data,0)};
    return collect(world,actor,value)&&valid(value);
}
bool move(void* transaction,uint64_t destination,uint64_t source) {
    alignas(16) unsigned char status[16]{};
    ui<void*(__fastcall*)(void*,void*,uint64_t,uint64_t,uint32_t)>(0x4149b0)(status,transaction,destination,source,2);
    // Native quantity selector: 0=all, 1=half, 2=one (419460).
    // A successful no-op is not a successful equipment transfer.
    if(!status[0]&&read<uint32_t>(status,8)!=1){trace("unexpected moved quantity",source,read<uint32_t>(status,8));return false;}
    if(status[0])trace("native move refused",source,status[0]);
    return status[0]==0;
}
void* apply(void* result,void* transaction,uint64_t address,bool remove) {
    using lantern::save_bridge::set;
    set<uint64_t>(result,0,0x11);
    address=slotAddress(address);
    Snapshot state{};if(!snapshot(transaction,state)){trace("invalid inventory",address,0x11);return result;}
    const auto cell=remove?state.equipped:Cell{address,{}};
    auto choice=select(state,cell);
    if(!remove){
        const auto found=std::find_if(state.bag.begin(),state.bag.end(),[&](const Cell& candidate){return candidate.address==address;});
        if(found==state.bag.end()){trace("source absent",address,0x11);return result;}choice=select(state,*found);
    }
    const auto plan=prepare(state,Catalog{{lantern::native::itemId(),lantern::native::goldItemId()}},choice);
    if(plan.result!=Result::ready){trace("plan refused",address,static_cast<unsigned>(plan.result));return result;}
    const auto privateInventory=component(equipment_storage::world.load(),equipment_storage::page.load(),"keen::ecs::Inventory",96);
    if(!privateInventory.data||!readStack(privateInventory.data,12).empty())return result;
    bool ok=false;
    if(remove)ok=move(transaction,plan.writes[1].before.address,state.equipped.address);
    else if(state.equipped.stack.empty())ok=move(transaction,state.equipped.address,address);
    else {
        const auto temporary=state.equipped.address|(uint64_t(1)<<32);
        ok=move(transaction,temporary,state.equipped.address)&&move(transaction,state.equipped.address,address)&&move(transaction,address,temporary);
    }
    // The native caller commits on success and rolls the ENTIRE transaction
    // back on any failure, including failure in the middle of a replacement.
    if(ok)set<uint64_t>(result,0,0);
    trace(remove?"unequip":"equip",address,ok?0:0x11);
    return result;
}
void* __fastcall equip(void* result,void* transaction,uint64_t source,uint8_t slot) {
    alignas(16) unsigned char found[16]{};
    ui<void*(__fastcall*)(void*,void*,uint64_t)>(0x416620)(found,transaction,source);
    const auto item=read<const void*>(found,8);
    if(found[0]||!item||!lantern::native::isLantern(read<uint32_t>(item,0)))return originalEquip(result,transaction,source,slot);
    try{return apply(result,transaction,source,static_cast<uint32_t>(source)==equipment_storage::page.load());}
    catch(...){lantern::save_bridge::set<uint64_t>(result,0,0x11);return result;}
}
void* __fastcall unequip(void* result,void* transaction,uint64_t source,uint8_t slot) {
    if(!equipment_storage::page.load()||static_cast<uint32_t>(source)!=equipment_storage::page.load())
        return originalUnequip(result,transaction,source,slot);
    try{return apply(result,transaction,source,true);}
    catch(...){lantern::save_bridge::set<uint64_t>(result,0,0x11);return result;}
}
}
void refreshEquipment(void* world,uint32_t actor) {
    bool worn=false;uint32_t selected=0;
    if(actor==equipment_storage::owner.load()&&world==equipment_storage::world.load()) {
        const auto slots=component(world,equipment_storage::page.load(),"keen::ecs::Inventory",96);
        worn=slots.data&&lantern::native::isLantern(read<uint32_t>(slots.data,0))&&read<uint32_t>(slots.data,4)==1;
        if(worn)selected=read<uint32_t>(slots.data,0);
    }
    equippedItem=selected;equipped=worn;owned=worn||inventoryHas(world,actor);
}

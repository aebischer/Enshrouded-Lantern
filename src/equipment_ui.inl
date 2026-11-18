namespace equipment_ui {
using lantern::save_bridge::set;
constexpr uint8_t category=21;
using Item=std::array<unsigned char,72>;
using Groups=void(__fastcall*)(void*);
using Page=void(__fastcall*)(void*,void*);
using Binding=void(__fastcall*)(void*);
using Browser=uint64_t(__fastcall*)(void*,const void*);
using Lookup=void*(__fastcall*)(uint32_t,void*);
using Compared=bool(__fastcall*)(void*,void*,uint32_t);
using Title=Text*(__fastcall*)(Text*,uint8_t);
using Icon=const void*(__fastcall*)(uint8_t);
using Actions=uint64_t(__fastcall*)(void*);
using Widget=void*(__fastcall*)(void*,void*,void*,void*);
using BuildActions=void(__fastcall*)(void*,void*,void*,void*);
Groups originalGroups=nullptr;
Page originalPage=nullptr;
Binding originalBinding=nullptr;
Browser originalBrowser=nullptr;
Lookup originalLookup=nullptr;
Compared originalCompared=nullptr;
Title originalTitle=nullptr;
Icon originalIcon=nullptr;
Actions originalActions=nullptr;
Widget originalWidget=nullptr;
BuildActions originalBuildActions=nullptr;
thread_local void* selectionState=nullptr;
thread_local bool browsing=false;
thread_local Item equippedRow{};
const unsigned char icon[]={0xa7,0xf3,0x69,0xa8,0x28,0x6a,0x4a,0xaf,0x91,0x5e,0x55,0x21,0x8e,0x7b,0xc9,0x10};
Text* __fastcall title(Text* output,uint8_t value) {
    if(value!=category)return originalTitle(output,value);
    *output=text(lantern::native::equipmentTitle());return output;
}
const void* __fastcall categoryIcon(uint8_t value) {
    return value==category?icon:originalIcon(value);
}
void* playerUi() {
    const auto context=*reinterpret_cast<void**>(base+0x283f4a8);
    if(!context)return nullptr;
    const auto services=read<void*>(context,0xd0);if(!services)return nullptr;
    const auto game=read<void*>(services,0);if(!game)return nullptr;
    const auto client=read<void*>(game,0x38);if(!client)return nullptr;
    const auto count=read<uint64_t>(client,0x3068);
    const auto rows=read<unsigned char*>(client,0x3060);
    if(count>16||!rows)return nullptr;
    for(uint64_t i=0;i<count;++i)if(read<uint32_t>(rows+i*0x8f70,0)==read<uint32_t>(client,0x18))return rows+i*0x8f70;
    return nullptr;
}
Item findEquipped(void* player) {
    Item result{};
    if(!player)return result;
    const auto data=read<const Item*>(player,0x2d8);const auto count=read<uint64_t>(player,0x2e0);
    if(count>1024||!data)return result;
    for(uint64_t i=0;i<count;++i)if(read<uint32_t>(data[i].data(),0)==equipment_storage::page.load()&&data[i][4]==0&&
        lantern::native::isLantern(read<uint32_t>(data[i].data(),8)))return data[i];
    return result;
}
void __fastcall groups(void* output) {
    originalGroups(output);
    // The native layout visits seven groups by fixed offsets. An eighth group
    // is never laid out. Add the cell to the existing accessories/rings group.
    const auto data=read<unsigned char*>(output,0);
    if(!data||read<uint64_t>(output,8)<5)return;
    auto group=data+4*312;
    const auto count=read<uint64_t>(group,16),capacity=read<uint64_t>(group,24);
    auto entries=read<unsigned char*>(group,8);
    if(!entries||count!=2||capacity<3)return;
    auto entry=entries+count*40;std::memset(entry,0,40);
    entry[3]=category;
    // Slot 24 selects the statistics/appearance renderer, not an item cell.
    // Slot 10 is only a visual template: binding() supplies our own instance.
    entry[4]=10;
    set<float>(entry,8,130);set<float>(entry,12,130);
    set<uint32_t>(entry,32,1);set<uint32_t>(entry,36,2);
    set<uint64_t>(group,16,count+1);
    static std::atomic<bool> reported{false};
    if(!reported.exchange(true))logger("Lantern: native accessory group extended with equipment cell");
}
void __fastcall page(void* state,void* groupsValue) {
    const auto previous=selectionState;selectionState=state;
    originalPage(state,groupsValue);selectionState=previous;
}
void __fastcall binding(void* value) {
    originalBinding(value);
    const auto entry=read<void*>(value,8);
    if(entry&&read<uint8_t>(entry,3)==category) {
        const auto row=findEquipped(playerUi());std::memcpy(static_cast<unsigned char*>(value)+16,row.data(),row.size());
    }
}
void* __fastcall lookup(uint32_t id,void* registry) {
    auto result=originalLookup(id,registry);
    if(!browsing||!lantern::native::isLantern(id)||!result)return result;
    // The view enables native equipment actions only while drawing this page.
    // The real descriptor and the server's item definition remain unchanged.
    const auto view=lantern::native::equipmentDescriptor(id);
    return view?const_cast<void*>(view):result;
}
bool __fastcall compared(void* player,void* output,uint32_t index) {
    if(!browsing)return originalCompared(player,output,index);
    if(!read<uint32_t>(equippedRow.data(),8))return false;
    std::memcpy(output,equippedRow.data(),equippedRow.size());return true;
}
uint64_t __fastcall actions(void* context) {
    const auto item=context?read<const void*>(context,8):nullptr;
    if(!item||!lantern::native::isLantern(read<uint32_t>(item,8)))return originalActions(context);
    const auto previous=browsing;const auto previousRow=equippedRow;
    browsing=true;equippedRow=findEquipped(playerUi());
    const auto result=originalActions(context);
    equippedRow=previousRow;browsing=previous;return result;
}
void* __fastcall widget(void* result,void* config,void* actionsValue,void* extra) {
    const auto item=config?read<const void*>(config,0):nullptr;
    if(!item||!lantern::native::isLantern(read<uint32_t>(item,8)))
        return originalWidget(result,config,actionsValue,extra);
    // Scope the equipment view used by the cell and its tooltip. Context
    // actions are built separately, before this widget, in buildActions().
    const auto previous=browsing;const auto previousRow=equippedRow;
    browsing=true;
    static std::atomic<bool> reported{false};
    if(!reported.exchange(true))logger("Lantern: native inventory widget equipment actions enabled");
    const auto returned=originalWidget(result,config,actionsValue,extra);
    equippedRow=previousRow;browsing=previous;return returned;
}
void __fastcall buildActions(void* player,void* config,void* state,void* output) {
    const auto item=config?read<const void*>(config,0):nullptr;
    if(!item||!lantern::native::isLantern(read<uint32_t>(item,8))) {
        originalBuildActions(player,config,state,output);return;
    }
    // d1ab40 receives the same cell config as c046c0, and writes up to
    // sixteen 72-byte actions followed by their byte count at +0x480.
    const auto previous=browsing;browsing=true;
    originalBuildActions(player,config,state,output);
    browsing=previous;
    if(!output)return;
    const auto count=read<uint8_t>(output,0x480);
    if(count>16)return;
    auto rows=static_cast<unsigned char*>(output);
    uint8_t kept=0;
    for(uint8_t i=0;i<count;++i) {
        auto entry=rows+i*72;
        if(entry[0]==0x45)continue; // Second native ring action.
        if(entry[0]==0x44)entry[0x1b]=0; // Native translated Equip, without "(1)" override.
        if(kept!=i)std::memmove(rows+kept*72,entry,72);
        ++kept;
    }
    set<uint8_t>(output,0x480,kept);
    // d11dd0 dispatches action 0x44 through d6f470 using this item's packed
    // source reference. The existing server equip adapter handles the lantern
    // before native slot selection, including replacement when already worn.
}
uint64_t __fastcall browser(void* config,const void* input) {
    if(!selectionState||read<uint8_t>(selectionState,8)!=category)return originalBrowser(config,input);
    auto player=read<void*>(config,0x38);if(!player)return 0;
    const auto data=read<const Item*>(player,0x2d8);const auto count=read<uint64_t>(player,0x2e0);
    if(count>1024||(count&&!data))return 0;
    std::vector<Item> available;
    equippedRow=findEquipped(player);
    for(uint64_t i=0;i<count;++i)if(lantern::native::isLantern(read<uint32_t>(data[i].data(),8))&&
        read<uint32_t>(data[i].data(),0)!=equipment_storage::page.load())available.push_back(data[i]);
    alignas(16) unsigned char copy[120]{};std::memcpy(copy,config,sizeof(copy));
    // Explicit instance reference, not either native ring slot. In particular,
    // an unavailable private page must never fall back to displaying ring 1.
    copy[0]=24;
    set<uint64_t>(copy,4,equipment_storage::page.load());
    set<const void*>(copy,0x18,icon);copy[0x5b]=1;copy[0x5e]=1;
    const auto title=text(lantern::native::equipmentTitle());std::memcpy(copy+0x40,&title,sizeof(title));
    struct Span {const Item* data;uint64_t count;} rows{available.data(),available.size()};
    const auto previous=browsing;browsing=true;
    const auto result=originalBrowser(copy,&rows);
    browsing=previous;return result;
}
}

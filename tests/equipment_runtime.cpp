#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <bcrypt.h>
#include <array>
#include <atomic>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include "../src/equipment_selection.h"
#include "../src/equipment_save_bridge.h"
using lantern::save_bridge::set;
using lantern::equipment::Stack;
namespace lantern::native {
unsigned itemId(){return 42;}
unsigned goldItemId(){return 43;}
bool isLantern(unsigned id){return id==42||id==43;}
const char* equipmentTitle(){return "Lanterne";}
std::array<unsigned char,2088> descriptor{};
const void* equipmentDescriptor(unsigned =0){return descriptor.data();}
}
template<class T>T read(const void* p,size_t n){return lantern::save_bridge::get<T>(p,n);}
struct View{unsigned char* data;size_t size;};
std::map<std::pair<uint32_t,std::string>,std::vector<unsigned char>> components;
View component(void* world,uint32_t actor,const char* name,size_t size){
 assert(!world||world==reinterpret_cast<void*>(0x10e179c0));
 auto it=components.find({actor,name});if(it==components.end()||it->second.size()!=size)return {};
 return {it->second.data(),size};
}
void log(const char*){}
auto logger=&log;
std::filesystem::path characterDirectory=std::filesystem::current_path()/"equipment-test"/"characters";
bool characterLoaded=true;
std::array<unsigned char,16> activeCharacter{{1}};
std::atomic<bool> equipped{false},owned{false};
std::atomic<uint32_t> equippedItem{0};
bool inventoryHas(void*,uint32_t){return false;}
std::map<uintptr_t,void*> functions;
template<class T>T ui(uintptr_t rva){return reinterpret_cast<T>(functions.at(rva));}
#include "../src/equipment_storage.inl"
#include "../src/equipment_transfer.inl"
uintptr_t base=0;
struct Text{const char* bytes;uint64_t size,hash;};
Text text(const char* s){return {s,strlen(s),0};}
#include "../src/equipment_ui.inl"
unsigned moveCalls=0,failMove=0,passthrough=0;
Stack* stack(uint64_t address){auto v=component(nullptr,uint32_t(address),"keen::ecs::Inventory",96);return v.data?reinterpret_cast<Stack*>(v.data)+unsigned((address>>32)&255):nullptr;}
uint32_t* actor(void*,uint32_t* output){*output=10;return output;}
void* lookup(void* result,void*,uint64_t address){set<uint64_t>(result,0,0);set<Stack*>(result,8,stack(address));return result;}
void* move(void* result,void*,uint64_t to,uint64_t from,uint32_t mode){
 assert(mode<=2);++moveCalls;set<uint64_t>(result,0,0x11);
 if(moveCalls==failMove)return result;
 auto src=stack(from),dst=stack(to);assert(src&&dst&&src->item&&src->count&&(!dst->item||!dst->count));
 const auto count=mode==1?src->count/2:mode==2?1:src->count;
 if(!count){set<uint64_t>(result,0,0);set<uint32_t>(result,8,0);return result;}
 assert(count==1);*dst=*src;*src={};set<uint64_t>(result,0,0);set<uint32_t>(result,8,1);return result;
}
void* pass(void* result,void*,uint64_t,uint8_t){++passthrough;return result;}
void reset(){
 components.clear();auto& setup=components[{10,"keen::ecs::InventorySetup"}];setup.resize(84);
 set<uint32_t>(setup.data(),0,20);setup[64]=4;set<uint32_t>(setup.data(),4,30);setup[65]=1;setup[83]=1;
 components[{20,"keen::ecs::Inventory"}].resize(96);components[{30,"keen::ecs::Inventory"}].resize(96);
 equipment_storage::owner=10;equipment_storage::page=30;equipment_storage::world=reinterpret_cast<void*>(0x10e179c0);
 moveCalls=failMove=0;
}
void transfers(){
 functions[0x9bf9e0]=reinterpret_cast<void*>(&actor);functions[0x416620]=reinterpret_cast<void*>(&lookup);functions[0x4149b0]=reinterpret_cast<void*>(&move);
 equipment_transfer::originalEquip=&pass;equipment_transfer::originalUnequip=&pass;
 std::array<unsigned char,16> result{},transaction{};
 reset();*stack(20)={42,1,123};equipment_transfer::equip(result.data(),transaction.data(),20,10);
 assert(!result[0]&&stack(20)->empty()&&stack(30)->persistentId==123&&moveCalls==1);
 equipment_transfer::unequip(result.data(),transaction.data(),30,10);
 assert(!result[0]&&stack(30)->empty()&&stack(20)->persistentId==123);
 // Native references carry padding; native vacant slots may retain item IDs.
 reset();*stack(20)={42,1,123};*stack(20|(uint64_t(1)<<32))={77,0,0};
 *stack(30)={42,0,0};*stack(30|(uint64_t(1)<<32))={42,0,0};
 equipment_transfer::equip(result.data(),transaction.data(),0xa5b6c70000000014ULL,10);
 assert(!result[0]&&stack(30)->persistentId==123);
 equipment_transfer::unequip(result.data(),transaction.data(),0xfedcba000000001eULL,10);
 assert(!result[0]&&stack(20)->persistentId==123);
 for(unsigned failure=0;failure<=3;++failure){
  reset();*stack(30)={42,1,99};*stack(20)={42,1,123};
  for(unsigned i=1;i<8;++i)*stack(20|(uint64_t(i)<<32))={100+i,1,200+i};
  const auto before=components;failMove=failure;
  equipment_transfer::equip(result.data(),transaction.data(),20,10);
  if(failure){assert(result[0]);components=before; /* native outer caller rollback */}
  else{assert(!result[0]&&stack(30)->persistentId==123&&stack(20)->persistentId==99);}
  assert(stack(30|(uint64_t(1)<<32))->empty());
 }
 reset();*stack(30)={42,1,99};for(unsigned i=0;i<8;++i)*stack(20|(uint64_t(i)<<32))={100+i,1,200+i};
 equipment_transfer::unequip(result.data(),transaction.data(),30,10);assert(result[0]&&!moveCalls&&stack(30)->persistentId==99);
 equipment_transfer::equip(result.data(),transaction.data(),20,10);assert(passthrough==1);
 equipment_transfer::unequip(result.data(),transaction.data(),20,10);assert(passthrough==2);
 refreshEquipment(reinterpret_cast<void*>(0x10e179c0),10);assert(equipped&&owned);
 refreshEquipment(reinterpret_cast<void*>(2),10);assert(!equipped);
}
unsigned exports=0;std::array<unsigned char,84> exportedSetup{};
void* nativeExport(void* result,void*,void*,void*,const void* setup,void*,void*,void*,void*,void*){
 ++exports;memcpy(exportedSetup.data(),setup,84);set<uint64_t>(result,0,0);return result;
}
void* nativeDecode(void* result,void*,void*,const void*){return result;}
unsigned imports=0;
void* nativeImport(void* result,void*,uint32_t actorId,uint8_t category,const void* descriptor,const void* source,void*){
 ++imports;set<uint64_t>(result,0,0);
 if(category==1){assert(actorId==10&&read<uint8_t>(descriptor,0)==1&&read<uint32_t>(descriptor,8)==1);
  assert(read<uint64_t>(source,8)==1);auto row=read<const void*>(source,0);
  auto setup=component(nullptr,10,"keen::ecs::InventorySetup",84);set<uint32_t>(setup.data,4,30);setup.data[65]=1;
  *stack(30)={read<uint32_t>(row,8),read<uint16_t>(row,0),read<uint32_t>(row,36)};
 }return result;
}
void storage(unsigned kind=42){
 reset();std::array<unsigned char,16> result{},entity{},context{};set<uint32_t>(entity.data(),0,10);
 equipment_storage::originalExport=&nativeExport;
 auto setup=component(nullptr,10,"keen::ecs::InventorySetup",84);
 equipment_storage::projected=false;
 equipment_storage::exportItems(result.data(),nullptr,nullptr,nullptr,setup.data,nullptr,nullptr,nullptr,nullptr,entity.data());
 assert(!equipment_storage::projected&&exportedSetup[65]==1);
 *stack(30)={42,1,123};equipment_storage::exportItems(result.data(),nullptr,nullptr,nullptr,setup.data,nullptr,nullptr,nullptr,nullptr,entity.data());
 assert(equipment_storage::projected&&exportedSetup[65]==4&&setup.data[65]==1);
 std::array<unsigned char,0x2000> snapshot{};
 std::vector<lantern::save_bridge::Row> rows(2);
 set<uint16_t>(rows[0].data(),0,1);set<uint32_t>(rows[0].data(),8,77);
 set<uint16_t>(rows[1].data(),0,1);set<uint32_t>(rows[1].data(),8,kind);rows[1][4]=8;
 set<uint32_t>(rows[1].data(),36,123);rows[1][40]=1;
 snapshot[0x1e38]=2;set<uint32_t>(snapshot.data(),0x1e40,2);
 set<const void*>(snapshot.data(),0x1e50,rows.data());set<uint64_t>(snapshot.data(),0x1e58,2);set<uint64_t>(snapshot.data(),0x1e60,2);
 assert(equipment_storage::record(snapshot.data()));const auto marker=equipment_storage::marker(snapshot.data(),rows);
 equipment_storage::originalDecode=&nativeDecode;equipment_storage::originalImport=&nativeImport;
 equipment_storage::Loading pending{};equipment_storage::loading=&pending;
 equipment_storage::decode(result.data(),snapshot.data(),nullptr,nullptr);
 assert(pending.parsed&&pending.restore.valid&&snapshot[0x1e38]==1&&read<uint32_t>(snapshot.data(),0x1e40)==1);
 set<uint32_t>(setup.data,4,0);setup.data[65]=0;
 // Native import context contains the scene, not its ECS storage.
 set<void*>(context.data(),0,reinterpret_cast<void*>(0x10000000));
 equipment_storage::import(result.data(),nullptr,10,2,nullptr,nullptr,context.data());
 equipment_storage::import(result.data(),nullptr,10,4,nullptr,nullptr,context.data());
 assert(!result[0]&&pending.created&&equipment_storage::page==30&&stack(30)->persistentId==123&&stack(30)->item==kind&&imports==3);
 equipment_storage::loading=nullptr;std::filesystem::remove(marker);
 snapshot[0x1e38]=2;set<uint32_t>(snapshot.data(),0x1e40,2);pending={};equipment_storage::loading=&pending;
 equipment_storage::decode(result.data(),snapshot.data(),nullptr,nullptr);
 assert(!pending.restore.valid&&snapshot[0x1e38]==2);equipment_storage::loading=nullptr;
}
unsigned browsed=0;
uint64_t nativeBrowser(void* config,const void* span){
 ++browsed;assert(equipment_ui::browsing);
 assert(read<uint64_t>(config,4)==30&&read<uint8_t>(config,0x5b)==1);
 assert(read<uint64_t>(span,8)==2);auto rows=read<const equipment_ui::Item*>(span,0);
 assert(read<uint32_t>(rows[0].data(),0)==20&&read<uint32_t>(rows[1].data(),0)==21);
 assert(read<uint32_t>(equipment_ui::equippedRow.data(),0)==30);return 456;
}
void interface(){
 std::array<unsigned char,0x300> player{};std::array<unsigned char,120> config{};std::array<unsigned char,16> state{};
 std::array<equipment_ui::Item,4> rows{};
 for(unsigned i=0;i<4;++i){set<uint32_t>(rows[i].data(),0,i==2?30:20+i);set<uint32_t>(rows[i].data(),8,i==3?99:42);}
 set<uint32_t>(rows[1].data(),8,43);
 set<const void*>(player.data(),0x2d8,rows.data());set<uint64_t>(player.data(),0x2e0,4);
 set<void*>(config.data(),0x38,player.data());state[8]=21;equipment_ui::selectionState=state.data();
 equipment_ui::originalBrowser=&nativeBrowser;const auto before=config;
 assert(equipment_ui::browser(config.data(),nullptr)==456&&browsed==1&&config==before&&!equipment_ui::browsing);
 Text caption{};assert(equipment_ui::title(&caption,21)==&caption&&std::string(caption.bytes)=="Lanterne");
 equipment_ui::selectionState=nullptr;
}

std::array<unsigned char,312*7> nativeGroups{};
std::array<unsigned char,40*6> nativeRingCells{};
void nativeGroupsBuilder(void* output){
 nativeGroups.fill(0);nativeRingCells.fill(0);
 auto group=nativeGroups.data()+4*312;
 nativeRingCells[3]=11;nativeRingCells[4]=10;
 nativeRingCells[43]=12;nativeRingCells[44]=11;
 set<void*>(group,8,nativeRingCells.data());set<uint64_t>(group,16,2);set<uint64_t>(group,24,6);
 set<void*>(output,0,nativeGroups.data());set<uint64_t>(output,8,7);
}
void* nativeWidget(void* output,void* config,void* actionsValue,void* extra){
 assert(actionsValue==reinterpret_cast<void*>(123)&&extra==reinterpret_cast<void*>(456));
 auto item=read<const void*>(config,0);
 assert(equipment_ui::browsing==(read<uint32_t>(item,8)==42));
 return output;
}
void* nativeItemLookup(uint32_t,void*){return reinterpret_cast<void*>(789);}
void uiRegression(){
 equipment_ui::originalGroups=&nativeGroupsBuilder;
 std::array<unsigned char,40> output{};
 equipment_ui::groups(output.data());
 assert(read<uint64_t>(output.data(),8)==7); // Native layout visits only these seven.
 assert(read<uint64_t>(nativeGroups.data()+4*312,16)==3);
 assert(nativeRingCells[3]==11&&nativeRingCells[4]==10&&nativeRingCells[43]==12&&nativeRingCells[44]==11);
 assert(nativeRingCells[83]==21&&nativeRingCells[84]!=24); // Must take the item-cell rendering branch.
 assert(read<float>(nativeRingCells.data()+80,8)==130);
 equipment_ui::originalWidget=&nativeWidget;equipment_ui::originalLookup=&nativeItemLookup;
 equipment_ui::Item item{};std::array<unsigned char,16> config{};
 set<void*>(config.data(),0,item.data());set<uint32_t>(item.data(),8,42);
 assert(equipment_ui::widget(output.data(),config.data(),reinterpret_cast<void*>(123),reinterpret_cast<void*>(456))==output.data());
 assert(!equipment_ui::browsing);
 set<uint32_t>(item.data(),8,99);
 equipment_ui::widget(output.data(),config.data(),reinterpret_cast<void*>(123),reinterpret_cast<void*>(456));
 assert(equipment_ui::lookup(42,nullptr)==reinterpret_cast<void*>(789));
 equipment_ui::browsing=true;
 assert(equipment_ui::lookup(42,nullptr)==lantern::native::equipmentDescriptor());
 assert(equipment_ui::lookup(99,nullptr)==reinterpret_cast<void*>(789));
 equipment_ui::browsing=false;
 std::cout<<"PASS: fixed native group layout, item-cell renderer, existing rings preserved, context widget scope and unrelated item passthrough\n";
}
unsigned cleanupCalls=0;
bool contextEnabled=true,contextBlocked=false;
void nativeContextBuilder(void* player,void* config,void* state,void* output){
 assert(player==reinterpret_cast<void*>(123)&&state==reinterpret_cast<void*>(456));
 auto rows=static_cast<unsigned char*>(output);
 std::memset(rows,0,0x481);
 const auto item=read<const void*>(config,0);
 const auto id=read<uint32_t>(item,8);
 // The native builder runs before the cell widget: a plain lantern gets no
 // equipment actions; the ring view gets actions 0x44 and 0x45.
 const bool ring=id==99||equipment_ui::lookup(id,nullptr)==lantern::native::equipmentDescriptor();
 if(contextBlocked){rows[0]=0x27;rows[0x480]=1;return;}
 rows[0]=0x24;rows[1]=1;rows[0x480]=1;
 if(ring){
  for(unsigned i=1;i<=2;++i){rows[i*72]=static_cast<uint8_t>(0x43+i);rows[i*72+1]=contextEnabled;
   rows[i*72+0x1b]=1;rows[i*72+0x1a]=2;}
  rows[0x480]=3;
 }
 const auto next=rows[0x480]++;rows[next*72]=0x48;rows[next*72+1]=1;
}
void contextMenuRegression(){
 equipment_ui::originalBuildActions=&nativeContextBuilder;
 equipment_ui::originalLookup=&nativeItemLookup;
 equipment_ui::Item item{};std::array<unsigned char,16> config{};
 std::array<unsigned char,0x481> output{};
 set<void*>(config.data(),0,item.data());set<uint32_t>(item.data(),8,42);
 const auto before=config;const auto itemBefore=item;
 nativeContextBuilder(reinterpret_cast<void*>(123),config.data(),reinterpret_cast<void*>(456),output.data());
 assert(output[0x480]==2&&output[72]==0x48); // Reproduce missing Equip.
 for(bool enabled:{true,false}){
  contextEnabled=enabled;
  equipment_ui::buildActions(reinterpret_cast<void*>(123),config.data(),reinterpret_cast<void*>(456),output.data());
  assert(output[0x480]==3&&output[0]==0x24&&output[72]==0x44&&output[144]==0x48);
  assert((output[73]!=0)==enabled&&output[72+0x1b]==0&&output[72+0x1a]==2);
  assert(!equipment_ui::browsing&&config==before&&item==itemBefore);
 }
 contextBlocked=true;
 equipment_ui::buildActions(reinterpret_cast<void*>(123),config.data(),reinterpret_cast<void*>(456),output.data());
 assert(output[0x480]==1&&output[0]==0x27); // Native drag/block state wins.
 contextBlocked=false;contextEnabled=true;
 set<uint32_t>(item.data(),8,99);
 equipment_ui::buildActions(reinterpret_cast<void*>(123),config.data(),reinterpret_cast<void*>(456),output.data());
 assert(output[0x480]==4&&output[72]==0x44&&output[144]==0x45&&output[72+0x1b]==1);
 set<uint32_t>(item.data(),8,42);equipment_ui::browsing=true;
 equipment_ui::buildActions(reinterpret_cast<void*>(123),config.data(),reinterpret_cast<void*>(456),output.data());
 assert(equipment_ui::browsing);equipment_ui::browsing=false;
 // Native context dispatch uses slot 24 for the actual non-ring definition.
 reset();*stack(20)={42,1,123};std::array<unsigned char,16> result{},transaction{};
 equipment_transfer::equip(result.data(),transaction.data(),20,24);
 assert(!result[0]&&stack(30)->persistentId==123&&!stack(20)->count);
 *stack(20)={43,1,456};equipment_transfer::equip(result.data(),transaction.data(),20,24);
 assert(!result[0]&&stack(30)->persistentId==456&&stack(30)->item==43&&stack(20)->persistentId==123);
 std::cout<<"PASS: context builder before widget, single native Equip caption, enabled/blocked states, untouched ring menu, scope restoration, context slot equip/replacement\n";
}
void nativeCleanup(void* context){
 ++cleanupCalls;auto setup=read<unsigned char*>(context,0x30);
 // Reproduce 401200: delete category 1 pages, then compact the links.
 unsigned out=0;
 for(unsigned i=0;i<16;++i){
  auto entity=read<uint32_t>(setup,i*4);if(!entity)continue;
  if(setup[64+i]==1){components.erase({entity,"keen::ecs::Inventory"});continue;}
  set<uint32_t>(setup,out*4,entity);setup[64+out]=setup[64+i];++out;
 }
 for(;out<16;++out){set<uint32_t>(setup,out*4,0);setup[64+out]=0;}
}
void cleanupRegression(){
 // Baseline: the unprotected native purge reproduces the missing live entity.
 reset();std::array<unsigned char,0x40> baseline{};
 set<void*>(baseline.data(),0x30,component(nullptr,10,"keen::ecs::InventorySetup",84).data);
 nativeCleanup(baseline.data());assert(!component(nullptr,30,"keen::ecs::Inventory",96).data);
 cleanupCalls=0;
 reset();equipment_storage::originalCleanup=&nativeCleanup;
 std::array<unsigned char,0x40> context{};
 auto setup=component(nullptr,10,"keen::ecs::InventorySetup",84).data;
 set<void*>(context.data(),0x30,setup);
 // A real cosmetic precedes our page: native compaction must remain effective.
 set<uint32_t>(setup,4,40);setup[65]=1;
 set<uint32_t>(setup,8,30);setup[66]=1;
 components[{40,"keen::ecs::Inventory"}].resize(96);
 for(unsigned i=0;i<3;++i){
  equipment_storage::cleanup(context.data());
  assert(component(nullptr,30,"keen::ecs::Inventory",96).data);
  assert(!component(nullptr,40,"keen::ecs::Inventory",96).data);
  assert(read<uint32_t>(setup,4)==30&&setup[65]==1);
 }
 // The page survives cleanup both empty and occupied, then still transfers.
 *stack(20)={42,1,123};std::array<unsigned char,16> result{},transaction{};
 equipment_transfer::equip(result.data(),transaction.data(),20,10);assert(!result[0]);
 equipment_storage::cleanup(context.data());assert(stack(30)->persistentId==123&&setup[65]==1);
 equipment_transfer::unequip(result.data(),transaction.data(),30,10);assert(!result[0]&&stack(20)->persistentId==123);
 // Do not shield another character/world even with the same page number.
 reset();auto foreign=components[{10,"keen::ecs::InventorySetup"}];set<void*>(context.data(),0x30,foreign.data());
 equipment_storage::cleanup(context.data());assert(!component(nullptr,30,"keen::ecs::Inventory",96).data);
 assert(cleanupCalls==5);
 std::cout<<"PASS: native customization purge, compaction, repeated empty/occupied cleanup, subsequent equip/unequip, foreign context passthrough\n";
}
int main(){transfers();storage();imports=0;storage(43);interface();uiRegression();contextMenuRegression();cleanupRegression();std::cout<<"PASS: production adapters: inventory transfers, full-bag replacement, failure statuses, native passthrough, save projection/import, absent marker fallback, multiple selector instances, input immutability\n";}


#include <Windows.h>
#include <bcrypt.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <array>
#include <atomic>
#include <mutex>
#include <map>
#include <filesystem>
#include <fstream>
#include <string>
#include <cmath>
#include "native_content.h"
#include "equipment_selection.h"
#include "equipment_save_bridge.h"
#include "enshrouded_mod.h"
#include "MinHook.h"

namespace {
uintptr_t base=0;
EnshroudedModLogFn logger=nullptr;
template<class T>T read(const void* p,size_t off){T v;std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
bool compatible(){
 wchar_t path[32768];const auto n=GetModuleFileNameW(nullptr,path,32768);if(!n||n>=32768)return false;
 auto file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);if(file==INVALID_HANDLE_VALUE)return false;
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
 bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
 std::array<unsigned char,65536> block{};DWORD count=0;
 while(ok){if(!ReadFile(file,block.data(),static_cast<DWORD>(block.size()),&count,nullptr)){ok=false;break;}if(!count)break;ok=BCryptHashData(hash,block.data(),count,0)>=0;}
 unsigned char digest[32]{};if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(file);
 constexpr unsigned char expected[]={0x5d,0x9e,0xf2,0xf1,0x40,0xb8,0xed,0x49,0xe3,0x0d,0xbe,0xe6,0x55,0x84,0x1b,0x89,0xd0,0x8b,0xae,0xc2,0x0f,0x79,0x9a,0x18,0x17,0xe1,0x71,0xa1,0x8a,0x85,0x0c,0x77};
 return ok&&!std::memcmp(digest,expected,32);
}
using Visual=void(__fastcall*)(void*,const void*,uint32_t,void*,void*);
Visual originalVisual=nullptr;
std::atomic<bool> equipped{false};
std::atomic<uint32_t> equippedItem{0};
template<class T>T ui(uintptr_t offset){return reinterpret_cast<T>(base+offset);}
void refreshEquipment(void*,uint32_t);
#include "equipment_state.inl"
#include "equipment_storage.inl"
#include "equipment_transfer.inl"
#include "equipment_save_diagnostic.inl"
std::mutex accessoryMutex;
struct Accessory {std::array<unsigned char,24> visual{};ULONGLONG last=0;};
std::map<std::pair<void*,uint32_t>,Accessory> accessories;

void __fastcall visual(void* existing,const void* wanted,uint32_t mask,void* query,void* registry){
 originalVisual(existing,wanted,mask,query,registry);
 const auto descriptor=read<void*>(query,0);
 if(!descriptor||read<unsigned char>(descriptor,0xa1)==1)return;
 uint32_t actor=0;
 reinterpret_cast<uint32_t*(__fastcall*)(void*,uint32_t*)>(base+0x9bf9e0)(query,&actor);
 if(actor&&actor==playerEntity.load()&&descriptor&&lantern::native::itemDescriptor()) {
  std::lock_guard<std::mutex> lock(accessoryMutex);
  const auto world=read<void*>(descriptor,0x30);
  auto& accessory=accessories[{world,actor}];const auto now=GetTickCount64();
  if(now-accessory.last>=50){
   accessory.last=now;
   alignas(16) unsigned char target[48]{};
   if(equipped.load()&&owned.load()&&mask!=0xffffffffu) {
    const uint32_t item=equippedItem.load();std::memcpy(target+8,&item,4);
    const auto info=lantern::native::itemDescriptor(item);std::memcpy(target+24,&info,8);
   }
   const auto before=read<uint32_t>(accessory.visual.data(),20);
   originalVisual(accessory.visual.data(),target,0,query,registry);
   const auto after=read<uint32_t>(accessory.visual.data(),20);
   if(before!=after){char line[192];sprintf_s(line,"Lantern prototype: actor=%u accessory=%u equipped=%d",actor,after,int(equipped.load()));logger(line);}
  }
 }

}
using DestroyWorld=void(__fastcall*)(void*);
DestroyWorld originalDestroyWorld=nullptr;
void __fastcall destroyWorld(void* world){
 {
  std::lock_guard<std::mutex> lock(accessoryMutex);
  for(auto it=accessories.begin();it!=accessories.end();) {
   if(it->first.first==world)it=accessories.erase(it);else ++it;
  }
 }
 if(equipment_storage::world.load()==world){
  equipment_storage::page=0;equipment_storage::owner=0;equipment_storage::world=nullptr;
  equipped=false;owned=false;playerEntity=0;characterLoaded=false;
 }
 originalDestroyWorld(world);
}
template<class T>void put(void* p,size_t at,T value){std::memcpy(static_cast<unsigned char*>(p)+at,&value,sizeof(value));}
struct Text {const char* bytes;uint64_t size,hash;};
Text text(const char* s){uint64_t h=0xcbf29ce484222325ull;for(auto p=s;*p;++p)h=(h^uint64_t(int64_t(int8_t(*p))))*0x100000001b3ull;return {s,std::strlen(s),h};}
using CharacterPage=bool(__fastcall*)();
CharacterPage originalCharacterPage=nullptr;
#include "equipment_preview.inl"
#include "equipment_ui.inl"
bool __fastcall characterPage(){
 drawingEquipmentPage=true;
 const bool result=originalCharacterPage();
 drawingEquipmentPage=false;
 return result;
}
using UiTick=void(__fastcall*)(void*);
UiTick originalUiTick=nullptr;
std::atomic<unsigned> lastScreen{255};
void __fastcall uiTick(void* context){
 originalUiTick(context);
 const auto services=read<const void*>(context,0xd0);
 const auto game=services?read<const void*>(services,0):nullptr;
 const auto client=game?read<const void*>(game,0x38):nullptr;
 if(!client)return;
 unsigned screen=read<unsigned char>(client,0x1cd8);
 if(lastScreen.exchange(screen)!=screen){char line[192];sprintf_s(line,"Lantern diagnostic: screen=%u context=%p services=%p client=%p",screen,context,services,client);logger(line);}
}
}
ENSHROUDED_MOD_EXPORT int ENSHROUDED_MOD_CALL EnshroudedModInitialize(const EnshroudedModContext* context){
 try{
  if(!context||context->size<sizeof(*context)||context->abi_version!=1||!context->world_id||!context->write_log)return 0;
  logger=context->write_log;if(!compatible()){logger("Lantern: unsupported executable");return 0;}
  base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
  HMODULE self=nullptr;wchar_t modulePath[32768]{};
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&EnshroudedModInitialize),&self)||!GetModuleFileNameW(self,modulePath,32768))return 0;
  characterDirectory=std::filesystem::path(modulePath).parent_path()/L"characters";
  if(MH_Initialize()!=MH_OK)return 0;
  void* targets[]={reinterpret_cast<void*>(base+0x386d70),reinterpret_cast<void*>(base+0xd5bb50),reinterpret_cast<void*>(base+0xb70e60),reinterpret_cast<void*>(base+0xe66060),reinterpret_cast<void*>(base+0x95d9c0),reinterpret_cast<void*>(base+0xd00860),reinterpret_cast<void*>(base+0x243ed0),reinterpret_cast<void*>(base+0xe19e60),reinterpret_cast<void*>(base+0xe4d850),reinterpret_cast<void*>(base+0xe0f470),reinterpret_cast<void*>(base+0xe560f0),reinterpret_cast<void*>(base+0xe1afc0),reinterpret_cast<void*>(base+0x4152d0),reinterpret_cast<void*>(base+0x419740),reinterpret_cast<void*>(base+0xd60a10),reinterpret_cast<void*>(base+0xba9e80),reinterpret_cast<void*>(base+0xda1f60),reinterpret_cast<void*>(base+0xbbcd40),reinterpret_cast<void*>(base+0xe94470),reinterpret_cast<void*>(base+0xd1ba80),reinterpret_cast<void*>(base+0xd45b20),reinterpret_cast<void*>(base+0xd33430),reinterpret_cast<void*>(base+0xbab260),reinterpret_cast<void*>(base+0xc046c0),reinterpret_cast<void*>(base+0x401200),reinterpret_cast<void*>(base+0xd1ab40),reinterpret_cast<void*>(base+0xe37100)};
  void* detours[]={reinterpret_cast<void*>(&visual),reinterpret_cast<void*>(&uiTick),reinterpret_cast<void*>(&characterPage),reinterpret_cast<void*>(&serverStep),reinterpret_cast<void*>(&destroyWorld),reinterpret_cast<void*>(&drawCharacter),reinterpret_cast<void*>(&updateCharacterScene),reinterpret_cast<void*>(&characterSnapshot),reinterpret_cast<void*>(&equipment_storage::load),reinterpret_cast<void*>(&equipment_storage::decode),reinterpret_cast<void*>(&equipment_storage::import),reinterpret_cast<void*>(&equipment_storage::exportItems),reinterpret_cast<void*>(&equipment_transfer::equip),reinterpret_cast<void*>(&equipment_transfer::unequip),reinterpret_cast<void*>(&equipment_ui::groups),reinterpret_cast<void*>(&equipment_ui::page),reinterpret_cast<void*>(&equipment_ui::binding),reinterpret_cast<void*>(&equipment_ui::browser),reinterpret_cast<void*>(&equipment_ui::lookup),reinterpret_cast<void*>(&equipment_ui::compared),reinterpret_cast<void*>(&equipment_ui::title),reinterpret_cast<void*>(&equipment_ui::categoryIcon),reinterpret_cast<void*>(&equipment_ui::actions),reinterpret_cast<void*>(&equipment_ui::widget),reinterpret_cast<void*>(&equipment_storage::cleanup),reinterpret_cast<void*>(&equipment_ui::buildActions),reinterpret_cast<void*>(&equipment_storage::encode)};
  void** originals[]={reinterpret_cast<void**>(&originalVisual),reinterpret_cast<void**>(&originalUiTick),reinterpret_cast<void**>(&originalCharacterPage),reinterpret_cast<void**>(&originalServerStep),reinterpret_cast<void**>(&originalDestroyWorld),reinterpret_cast<void**>(&originalDrawCharacter),reinterpret_cast<void**>(&originalUpdateCharacterScene),reinterpret_cast<void**>(&originalCharacterSnapshot),reinterpret_cast<void**>(&equipment_storage::originalLoad),reinterpret_cast<void**>(&equipment_storage::originalDecode),reinterpret_cast<void**>(&equipment_storage::originalImport),reinterpret_cast<void**>(&equipment_storage::originalExport),reinterpret_cast<void**>(&equipment_transfer::originalEquip),reinterpret_cast<void**>(&equipment_transfer::originalUnequip),reinterpret_cast<void**>(&equipment_ui::originalGroups),reinterpret_cast<void**>(&equipment_ui::originalPage),reinterpret_cast<void**>(&equipment_ui::originalBinding),reinterpret_cast<void**>(&equipment_ui::originalBrowser),reinterpret_cast<void**>(&equipment_ui::originalLookup),reinterpret_cast<void**>(&equipment_ui::originalCompared),reinterpret_cast<void**>(&equipment_ui::originalTitle),reinterpret_cast<void**>(&equipment_ui::originalIcon),reinterpret_cast<void**>(&equipment_ui::originalActions),reinterpret_cast<void**>(&equipment_ui::originalWidget),reinterpret_cast<void**>(&equipment_storage::originalCleanup),reinterpret_cast<void**>(&equipment_ui::originalBuildActions),reinterpret_cast<void**>(&equipment_storage::originalEncode)};
  size_t created=0;
  for(;created<std::size(targets);++created)if(MH_CreateHook(targets[created],detours[created],originals[created])!=MH_OK)break;
  if(created!=std::size(targets)){for(size_t i=0;i<created;++i)MH_RemoveHook(targets[i]);return 0;}
  HMODULE owner=nullptr;
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&EnshroudedModInitialize),&owner))return 0;
  bool ok=true;for(auto target:targets)ok=MH_QueueEnableHook(target)==MH_OK&&ok;
  ok=ok&&MH_ApplyQueued()==MH_OK;
  if(!ok){for(auto target:targets)MH_DisableHook(target);return 0;}
  if(!lantern::native::initialize(logger)){for(auto target:targets)MH_DisableHook(target);return 0;}
  logger("Lantern 0.1.29: native equipment selector and inventory bridge initialized");return 1;
 }catch(...){return 0;}
}





// Executed after the native local-server step, on the inventory owning thread.
using Guid=std::array<unsigned char,16>;
std::atomic<uint32_t> playerEntity{0};
std::atomic<bool> owned{false};
std::filesystem::path characterDirectory;
Guid activeCharacter{};
bool characterLoaded=false;
uint32_t nameHash(const char* s){uint32_t h=2166136261u;for(;*s;++s)h=(h^uint8_t(*s))*16777619u;return h;}
struct View {unsigned char* data;size_t size;};
View component(void* world,uint32_t entity,const char* name,size_t size){
 if(!world||!entity)return {};
 auto registry=read<unsigned char*>(world,0);if(!registry)return {};
 const auto hash=nameHash(name);
 auto indexOf=ui<const uint16_t*(__fastcall*)(void*,const uint32_t*)>(0x9b1db0);
 auto index=indexOf(registry+0x30,&hash);if(!index)index=indexOf(registry+0x88,&hash);
 if(!index||*index>=640)return {};
 View result{};ui<View*(__fastcall*)(View*,void*,const uint16_t*,uint32_t)>(0x9b2b50)(&result,world,index,entity);
 return result.data&&result.size==size?result:View{};
}
bool inventoryHas(void* world,uint32_t entity,unsigned depth=0){
 if(depth>4)return false;
 const auto setup=component(world,entity,"keen::ecs::InventorySetup",84);
 if(!setup.data||!setup.data[83])return false;
 for(unsigned i=0;i<16;++i){
  const auto link=read<uint32_t>(setup.data,i*4);if(!link)break;
  if(setup.data[64+i]==5){if(inventoryHas(world,link,depth+1))return true;continue;}
  const auto slots=component(world,link,"keen::ecs::Inventory",96);if(!slots.data)continue;
  for(unsigned j=0;j<8;++j)if(lantern::native::isLantern(read<uint32_t>(slots.data,j*12))&&read<uint32_t>(slots.data,j*12+4))return true;
 }
 return false;
}
using ServerStep=void(__fastcall*)(void*);
ServerStep originalServerStep=nullptr;
void __fastcall serverStep(void* server){
 originalServerStep(server);
 try {
  const auto scene=read<uintptr_t>(server,0x1d0);if(!scene){owned=false;equipped=false;playerEntity=0;return;}
  uint32_t entity=0;Guid character{};unsigned players=0;
  for(unsigned i=0;i<16;++i){const auto slot=static_cast<unsigned char*>(server)+0x1e0+i*0x148;
   if(!read<uint32_t>(slot,0)||!read<uint32_t>(slot,0x18))continue;
   ++players;entity=read<uint32_t>(slot,0x18);character=read<Guid>(slot,0x50);
  }
  if(players!=1||character==Guid{}){owned=false;equipped=false;playerEntity=0;return;}
  auto world=reinterpret_cast<void*>(scene+0xe179c0);
  const auto setup=component(world,entity,"keen::ecs::InventorySetup",84);
  if(!setup.data||!setup.data[83]){owned=false;equipped=false;return;}
  if(!characterLoaded||character!=activeCharacter){
   activeCharacter=character;characterLoaded=true;equipped=false;
   char message[160];sprintf_s(message,"Lantern: active player=%u world=%p inventory initialized",entity,world);logger(message);
  }
  playerEntity=entity;refreshEquipment(world,entity);
 }catch(...){owned=false;equipped=false;logger("Lantern: equipment state update refused");}
}

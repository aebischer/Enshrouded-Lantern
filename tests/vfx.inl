namespace vfx_test {
using namespace lantern::content;
using Byte=unsigned char;
template<class T>T read(const void* p,size_t at){T value;std::memcpy(&value,static_cast<const Byte*>(p)+at,sizeof(value));return value;}
template<class T>void write(void* p,size_t at,T value){std::memcpy(static_cast<Byte*>(p)+at,&value,sizeof(value));}
enum class ResourceKind {Light};
struct Entry {struct Key {Guid guid;} key;ResourceKind kind=ResourceKind::Light;};
Entry entries[]={{{guid("b4a8ef162fc645efb178dea679d20513")}},{{GoldLight}}};
bool available=true;
size_t lastBuilt=99;
const Bytes* build(size_t entry,void*){lastBuilt=entry;static const Bytes bytes{1};return available?&bytes:nullptr;}
void log(const char*){}
#include "../src/native_lantern_vfx.inl"
unsigned registrations=0,creations=0;
void __fastcall insert(void* manager,const Guid* key){
 ++registrations;
 auto bits=read<uint64_t*>(manager,0x1be0);
 auto keys=read<Guid*>(manager,0x1c00);
 auto values=read<uint32_t*>(manager,0x1c18);
 const auto count=read<uint32_t>(manager,0x1c2c);
 auto at=read<uint32_t>(key->data(),0)&7u;
 while(bits[0]&(uint64_t(1)<<at))at=(at+1)&7u;
 keys[at]=*key;values[at]=count;bits[0]|=uint64_t(1)<<at;
 write(manager,0x1c2c,count+1);
 auto records=read<Byte*>(manager,0x1c38);
 write<const void*>(records+count*0xc0,0x10,records);
}
uint32_t* __fastcall create(uint32_t* out,void*,const Guid*,const void*,const void*){++creations;*out=42;return out;}
unsigned starts=0;
void* expectedManager=nullptr;
void __fastcall guardedStart(void*,Byte* entry,int64_t now,void* timing){
 ++starts;assert(now==123&&timing==nullptr);
 if(entry&&entry[0]==1){
  const auto key=read<Guid>(entry,0x160);uint32_t index=0;
  // Model Kiln's earlier guard: unregistered GUIDs are erased before CreateInstance.
  if(!lanternVfxIndex(expectedManager,key,index))std::memset(entry+0x160,0,16);
 }
}
void run(){
 Bytes manager(0x2200),records(8*0xc0),context(16);
 uint64_t bits=0,pending=0;Guid keys[8]{};uint32_t values[8]{};
 write(manager.data(),0x1be0,&bits);write<uint64_t>(manager.data(),0x1be8,8);
 write(manager.data(),0x1c00,keys);write(manager.data(),0x1c18,values);
 write<uint32_t>(manager.data(),0x1c28,6);write(manager.data(),0x1c38,records.data());
 write<uint64_t>(manager.data(),0x1c40,8);write<uint64_t>(manager.data(),0x2100,8);
 write(manager.data(),0x20f8,&pending);write(manager.data(),0x148,manager.data());
 write(context.data(),8,manager.data());registerVfx=insert;originalVfxCreate=create;
 uint32_t out=99;const auto key=entries[0].key.guid;
 // Missing key, as in the crash: insert before forwarding to native creation.
 lanternVfxCreate(&out,context.data(),&key,nullptr,nullptr);
 assert(out==42&&registrations==1&&creations==1);
 lanternVfxCreate(&out,context.data(),&key,nullptr,nullptr);
 assert(registrations==1&&creations==2);
 // A new native catalogue at the same address must be registered again.
 bits=0;write<uint32_t>(manager.data(),0x1c2c,0);
 available=false;lanternVfxCreate(&out,context.data(),&key,nullptr,nullptr);
 assert(out==0&&registrations==1&&creations==2&&bits==0);
 available=true;write<uint32_t>(manager.data(),0x1c28,0);
 lanternVfxCreate(&out,context.data(),&key,nullptr,nullptr);
 assert(out==0&&registrations==1&&creations==2);
 write<uint32_t>(manager.data(),0x1c28,6);
 // Collision probing with a different native GUID.
 auto other=key;other[15]^=1;insert(manager.data(),&other);
 lanternVfxCreate(&out,context.data(),&key,nullptr,nullptr);
 assert(out==42&&registrations==3&&creations==3);
 lanternVfxCreate(&out,context.data(),&other,nullptr,nullptr);
 assert(out==42&&registrations==3&&creations==4);
 lanternVfxCreate(&out,nullptr,&key,nullptr,nullptr);assert(out==0&&creations==4);
 const auto gold=entries[1].key.guid;
 lanternVfxCreate(&out,context.data(),&gold,nullptr,nullptr);
 assert(out==42&&registrations==4&&creations==5&&lastBuilt==1);
 uint32_t normalIndex=99,goldIndex=99;
 assert(lanternVfxIndex(manager.data(),key,normalIndex)&&lanternVfxIndex(manager.data(),gold,goldIndex)&&normalIndex!=goldIndex);
 lanternVfxCreate(&out,context.data(),&gold,nullptr,nullptr);
 lanternVfxCreate(&out,context.data(),&key,nullptr,nullptr);
 assert(registrations==4&&creations==7);
 // Reproduce the real Kiln failure on an empty catalogue, for both variants.
 Bytes controller(16),renderer(0xa0),holder(0x958),fx(0x180);
 write(controller.data(),8,renderer.data());write(renderer.data(),0x98,holder.data());
 write(holder.data(),0x950,context.data());originalFxStart=guardedStart;expectedManager=manager.data();
 for(const auto& light:{key,gold}){
  bits=0;write<uint32_t>(manager.data(),0x1c2c,0);fx[0]=1;write(fx.data(),0x160,light);
  guardedStart(controller.data(),fx.data(),123,nullptr);assert(read<Guid>(fx.data(),0x160)==Guid{});
  write(fx.data(),0x160,light);
  lanternFxStart(controller.data(),fx.data(),123,nullptr);
  assert(read<Guid>(fx.data(),0x160)==light);
  const auto count=registrations;
  lanternFxStart(controller.data(),fx.data(),123,nullptr);assert(registrations==count);
 }
 // Unrelated effects keep the guard, and resource failure never bypasses it.
 write(fx.data(),0x160,other);lanternFxStart(controller.data(),fx.data(),123,nullptr);
 assert(read<Guid>(fx.data(),0x160)==Guid{});
 bits=0;write<uint32_t>(manager.data(),0x1c2c,0);available=false;
 write(fx.data(),0x160,key);lanternFxStart(controller.data(),fx.data(),123,nullptr);
 assert(read<Guid>(fx.data(),0x160)==Guid{});available=true;
 fx[0]=0;write(fx.data(),0x160,key);lanternFxStart(controller.data(),fx.data(),123,nullptr);
 assert(read<Guid>(fx.data(),0x160)==key);
 lanternFxStart(nullptr,nullptr,123,nullptr);assert(starts==10);
 std::cout<<"PASS: VFX registration before creation, repeated calls, catalogue renewal, collisions, missing resource/capacity, native passthrough, null context\n";
}
}

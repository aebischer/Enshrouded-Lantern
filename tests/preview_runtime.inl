namespace lantern::native {inline unsigned goldItemId(){return 43;}}
namespace preview_runtime_test {
std::atomic<uint32_t> equippedItem{42};
std::atomic<bool> equipped{true},owned{true};
void (*logger)(const char*)=nullptr;
template<class T>T read(const void* p,size_t at){T v;std::memcpy(&v,static_cast<const unsigned char*>(p)+at,sizeof(v));return v;}
void* resolve(uintptr_t offset);
template<class T>T ui(uintptr_t offset){return reinterpret_cast<T>(resolve(offset));}
#include "../src/equipment_preview.inl"
unsigned creates=0,updates=0,removes=0,nativeUpdates=0,transforms=0;
bool conversion=true;const void* drawn=nullptr;
std::array<unsigned char,lantern_preview::descriptorSize+1> queued{};
void __fastcall draw(void*,const void* data,const void*){
 drawn=data;std::memcpy(queued.data(),data,lantern_preview::descriptorSize);queued.back()=1;
}
void __fastcall nativeUpdate(void*,const void*,const void*,uint64_t){++nativeUpdates;}
uint32_t __fastcall create(void*,const void* guid,const void*){assert(lantern_preview::ours(guid));++creates;return 123;}
void __fastcall flag(void*,uint32_t id,unsigned char value){assert(id==123&&value==1);}
void __fastcall remove(void*,const PreviewModels* ids){assert(ids->count==1&&*ids->data==123);++removes;}
void __fastcall update(void*,const PreviewModels* ids,const PreviewTransforms* poses){assert(ids->count==1&&*ids->data==123&&poses->count==1);++updates;}
void __fastcall rootMatrix(const void* root,void* out,const void* anchor){
 assert(root==anchor);const auto& pose=*static_cast<const PreviewWorldTransform*>(root);
 assert(std::abs(pose.rotation[1]-std::sin(-.25f))<1e-6f);
 std::memset(out,0,64);++transforms;
}
void* __fastcall combine(void* out,const void* pelvis,const void*){assert(read<float>(pelvis,0)==42.f);std::memset(out,0,64);return out;}
bool __fastcall convert(void*,const void*,const void*){return conversion;}
void __fastcall offset(void*,const void* input){
 const auto& value=*static_cast<const PreviewOffset*>(input);
 assert(value.local==lantern_fit::offset()&&value.rotation==lantern_fit::rotation);
}
void* resolve(uintptr_t value){
 switch(value){
 case 0xf38e0:return reinterpret_cast<void*>(&create);
 case 0xf6eb0:return reinterpret_cast<void*>(&flag);
 case 0xf67f0:return reinterpret_cast<void*>(&remove);
 case 0xf6060:return reinterpret_cast<void*>(&update);
 case 0xf7e670:return reinterpret_cast<void*>(&rootMatrix);
 case 0x19fc90:return reinterpret_cast<void*>(&combine);
 case 0xf7ff20:return reinterpret_cast<void*>(&convert);
 case 0x965bc0:return reinterpret_cast<void*>(&offset);
 default:assert(false);return nullptr;
 }
}
template<class T>void write(void* p,size_t at,const T& value){std::memcpy(static_cast<unsigned char*>(p)+at,&value,sizeof(value));}
void run(){
 originalDrawCharacter=draw;originalUpdateCharacterScene=nativeUpdate;
 std::array<unsigned char,lantern_preview::descriptorSize+1> source{};
 write(source.data(),0x8bc,.5f);
 drawCharacter(nullptr,source.data(),nullptr);
 assert(drawn==source.data()&&!lantern_preview::requested(queued.data())); // Startup/other page.
 drawingEquipmentPage=true;owned=false;
 drawCharacter(nullptr,source.data(),nullptr);assert(drawn==source.data());
 owned=true;equipped=false;
 drawCharacter(nullptr,source.data(),nullptr);assert(drawn==source.data());
 equipped=true;
 drawCharacter(nullptr,source.data(),nullptr);
 assert(drawn!=source.data()&&lantern_preview::requested(queued.data()));
 assert(!lantern_preview::requested(source.data()));
 std::array<unsigned char,0x510> scene{};
 std::array<unsigned char,0x640+0x910> skeleton{};
 write(scene.data(),0x20,static_cast<void*>(scene.data()));
 write(scene.data(),0xd8,static_cast<void*>(skeleton.data()));
 write(scene.data(),0xf8,uint32_t(1));
 write(skeleton.data(),0x640+0x900,uint64_t(8));
 write(skeleton.data(),0x640+3*64,42.f);
 updateCharacterScene(scene.data(),queued.data(),nullptr,0);
 assert(nativeUpdates==1&&creates==1&&updates==1&&transforms==1);
 updateCharacterScene(scene.data(),queued.data(),nullptr,0);
 assert(creates==1&&updates==2);
 conversion=false;
 updateCharacterScene(scene.data(),queued.data(),nullptr,0);
 assert(removes==1); // Bad attachment transform removes the previous image.
 conversion=true;
 updateCharacterScene(scene.data(),queued.data(),nullptr,0);
 assert(creates==2);
 equipped=false;
 updateCharacterScene(scene.data(),queued.data(),nullptr,0);
 assert(removes==2);
 equipped=true;write(skeleton.data(),0x640+0x900,uint64_t(0));
 const auto before=transforms;
 updateCharacterScene(scene.data(),queued.data(),nullptr,0);
 assert(creates==2&&transforms==before); // Missing pelvis safely skips rendering.
 write(skeleton.data(),0x640+0x900,uint64_t(8));queued.back()=0;
 updateCharacterScene(scene.data(),queued.data(),nullptr,0);
 assert(creates==2&&transforms==before); // Scene teardown/closed preview.
 drawingEquipmentPage=false;
 std::cout<<"PASS: N-only preview routing, source immutability, native calls, shared fit, missing pose, unequip and scene teardown\n";
}
}

// Le catalogue initial du moteur précède l'activation des mods par monde.
// Ajouter les textures privées sur le parcours de chargement graphique, avant
// la recherche native (qui déclenche une assertion si la clé est absente).
using TextureLookup=uint32_t*(__fastcall*)(uint32_t*,void*,const Byte*,uint32_t);
using TextureRegister=void(__fastcall*)(void*,const Byte*);
using TextureRequirements=void*(__fastcall*)(void*,void*,void*);
using TextureAllocate=void*(__fastcall*)(void*,const void*);
using TextureBind=void(__fastcall*)(void*,const void*);
TextureLookup originalTextureLookup=nullptr;
TextureRegister registerTexture=nullptr;
TextureRequirements textureRequirements=nullptr;
TextureAllocate allocateTextureMemory=nullptr;
TextureBind bindTextureMemory=nullptr;
std::mutex lanternTextureMutex;
enum class TexturePhase {Registered,Ready};
struct LanternTextureMemory {
 void* manager;Guid hash;void* memory;Byte* records;void* image;void* device;
 uint16_t index;TexturePhase phase;
};
std::vector<LanternTextureMemory> lanternTextureMemory;

const Byte* privateTexture(const Guid& hash) {
 for(const auto id:{LanternMaterialId,GoldMaterialId}) {
 const auto material=lanternAsset(id,0xaee1894b);
 if(!material||material->size<24)return nullptr;
 const auto relative=read<int32_t>(material->data,16);
 const auto count=read<uint32_t>(material->data,20);
 const int64_t start=int64_t(16)+relative;
 if(start<24||count!=4||uint64_t(start)>material->size||count>(material->size-size_t(start))/96)return nullptr;
 for(uint32_t i=0;i<count;++i) {
  auto texture=material->data+size_t(start)+i*96+8;
  if(read<Guid>(texture,8)==hash)return texture;
 }
 }
 return nullptr;
}

bool textureIndex(void* manager,const Guid& hash,uint16_t& index) {
 const auto capacity=read<uint64_t>(manager,0xf98);
 const auto bits=read<const uint64_t*>(manager,0xf90);
 const auto keys=read<const Guid*>(manager,0xfb0);
 const auto values=read<const uint16_t*>(manager,0xfc8);
 if(!capacity||(capacity&(capacity-1))||capacity>0x20000||!bits||!keys||!values)return false;
 auto at=uint64_t(read<uint32_t>(hash.data(),4))&(capacity-1);
 for(uint64_t n=0;n<capacity;++n,at=(at+1)&(capacity-1)) {
  if(!(bits[at/64]&(uint64_t(1)<<(at%64))))return false;
  if(keys[at]==hash){index=values[at];return index<read<uint32_t>(manager,0xfdc);}
 }
 return false;
}

bool prepareLanternTexture(void* manager,const Byte* texture) {
 const auto hash=read<Guid>(texture,8);
 auto existingState=std::find_if(lanternTextureMemory.begin(),lanternTextureMemory.end(),
  [&](const auto& state){return state.manager==manager&&state.hash==hash;});
 if(existingState!=lanternTextureMemory.end()) {
  uint16_t liveIndex=0;
  const bool present=textureIndex(manager,hash,liveIndex);
  const auto records=read<Byte*>(manager,0xfe8);
  const auto image=present&&records?read<void*>(records+size_t(liveIndex)*0x48,0x30):nullptr;
  const auto device=read<void*>(manager,0x10);
  if(!present) {
   // Native teardown owns old GPU allocations. Discard only our bookkeeping.
   lanternTextureMemory.erase(existingState);existingState=lanternTextureMemory.end();
  } else if(existingState->index!=liveIndex||existingState->records!=records||
            existingState->image!=image||existingState->device!=device) {
   *existingState={manager,hash,nullptr,records,image,device,liveIndex,TexturePhase::Registered};
  }
 }
 if(existingState==lanternTextureMemory.end()) {
  uint16_t existing=0;
  if(textureIndex(manager,hash,existing))return true;
  const auto count=read<uint32_t>(manager,0xfdc);
  // Native arrays must not move while their streaming jobs retain pointers.
  if(!read<uint8_t>(manager,0x1020)||!count||count>=0xffff||
     count>=read<uint64_t>(manager,0xff0)||count>=read<uint64_t>(manager,0x1000)||
     count>=read<uint32_t>(manager,0xfd8)||count+1>=read<uint64_t>(manager,0xf98)||
     read<uint64_t>(manager,0x1370)>=read<uint64_t>(manager,0x1378)||
     !read<void*>(manager,0xfe8)||!read<void*>(manager,0xff8)||
     !registerTexture||!textureRequirements||!allocateTextureMemory||!bindTextureMemory)return false;
  // Allocate bookkeeping BEFORE native mutation. An allocation failure leaves
  // the native catalogue unchanged. Subsequent GPU failures retain a resumable
  // registration, not a permanently poisoned entry or a second insertion.
  lanternTextureMemory.reserve(lanternTextureMemory.size()+1);
  registerTexture(manager,texture);
  uint16_t index=0;
  if(!textureIndex(manager,hash,index)||index!=count)return false;
  auto records=read<Byte*>(manager,0xfe8);
  auto image=read<void*>(records+size_t(index)*0x48,0x30);
  lanternTextureMemory.push_back({manager,hash,nullptr,records,image,read<void*>(manager,0x10),index,TexturePhase::Registered});
  existingState=lanternTextureMemory.end()-1;
 }
 auto& state=*existingState;
 uint16_t index=0;
 // A manager address alone is not a generation. Refuse stale GPU objects.
 if(!state.records||!textureIndex(manager,hash,index)||index!=state.index||read<Byte*>(manager,0xfe8)!=state.records||
    read<void*>(state.records+size_t(index)*0x48,0x30)!=state.image)return false;
 if(state.phase==TexturePhase::Ready)return true;
 auto image=state.image,device=read<void*>(manager,0x10);
 if(!image||!device)return false;
 alignas(16) uint64_t requirements[3]{};
 textureRequirements(requirements,device,image);
 if(!requirements[0]||requirements[0]>64*1024*1024||!requirements[1])return false;
 // Même description et liaison que l'initialisation native des mip tails,
 // avec une allocation distincte : ne jamais remplacer celle du jeu entier.
 alignas(16) Byte descriptor[64]{};
 descriptor[0]=read<Byte>(manager,0x178);
 write<uint64_t>(descriptor,8,requirements[0]);
 write<uint16_t>(descriptor,0x38,1);
 if(!state.memory)state.memory=allocateTextureMemory(device,descriptor);
 if(!state.memory)return false;
 struct Binding {void* image;void* memory;uint64_t offset,size;};
 Binding binding{image,state.memory,0,requirements[0]};
 struct Span {const Binding* data;uint64_t count;};
 Span bindings{&binding,1};bindTextureMemory(device,&bindings);
 auto pending=read<uint64_t*>(manager,0xff8);
 const auto mask=uint64_t(1)<<(index%64);
 if(!(pending[index/64]&mask)) {
  pending[index/64]|=mask;
  write<uint32_t>(manager,0x1010,read<uint32_t>(manager,0x1010)+1);
 }
 state.phase=TexturePhase::Ready;
 log("Lantern: private lantern texture registered and GPU memory bound");
 return true;
}

uint32_t* __fastcall lanternTextureLookup(uint32_t* out,void* manager,const Byte* texture,uint32_t flags) {
 const auto own=texture?privateTexture(read<Guid>(texture,8)):nullptr;
 if(!own)return originalTextureLookup(out,manager,texture,flags);
 try {
  bool ready=false;
  {std::lock_guard<std::mutex> lock(lanternTextureMutex);
   ready=manager&&prepareLanternTexture(manager,own);}
  // Never hold our lock across the downstream native lookup.
  if(ready)return originalTextureLookup(out,manager,texture,flags);
 }catch(const std::exception&){}
 // Une défaillance ne doit pas appeler la recherche native avec une clé absente.
 log("Lantern: lantern texture unavailable; native missing-texture assertion avoided");
 *out=0;return out;
}

void bindLanternTextures() {
 registerTexture=reinterpret_cast<TextureRegister>(base+0xb2fb0);
 textureRequirements=reinterpret_cast<TextureRequirements>(base+0x10045b0);
 allocateTextureMemory=reinterpret_cast<TextureAllocate>(base+0xfda3a0);
 bindTextureMemory=reinterpret_cast<TextureBind>(base+0xfcab30);
}

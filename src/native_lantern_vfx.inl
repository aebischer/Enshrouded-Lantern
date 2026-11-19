// Build 1287369: VFX catalogue creation precedes per-world mod activation.
// Register on the native instance-creation path, before it acquires a resource
// index. Returning a null instance here is supported by the native caller.
using VfxCreate=uint32_t*(__fastcall*)(uint32_t*,void*,const Guid*,const void*,const void*);
using VfxRegister=void(__fastcall*)(void*,const Guid*);
VfxCreate originalVfxCreate=nullptr;
VfxRegister registerVfx=nullptr;
std::mutex lanternVfxMutex;

bool lanternVfxIndex(void* manager,const Guid& key,uint32_t& index) {
 const auto capacity=read<uint64_t>(manager,0x1be8);
 const auto bits=read<const uint64_t*>(manager,0x1be0);
 const auto keys=read<const Guid*>(manager,0x1c00);
 const auto values=read<const uint32_t*>(manager,0x1c18);
 if(!capacity||(capacity&(capacity-1))||capacity>0x20000||!bits||!keys||!values)return false;
 auto at=uint64_t(read<uint32_t>(key.data(),0))&(capacity-1);
 for(uint64_t n=0;n<capacity;++n,at=(at+1)&(capacity-1)) {
  if(!(bits[at/64]&(uint64_t(1)<<(at%64))))return false;
  if(keys[at]==key){index=values[at];return index<read<uint64_t>(manager,0x1c40);}
 }
 return false;
}

bool prepareLanternVfx(void* manager,size_t entry) {
 if(!manager)return false;
 const auto& key=entries[entry].key.guid;
 uint32_t index=0;
 if(!lanternVfxIndex(manager,key,index)) {
  const auto count=read<uint32_t>(manager,0x1c2c);
  const auto capacity=read<uint64_t>(manager,0x1be8);
  // Do not grow or relocate arrays while native streaming jobs use them.
  if(!capacity||(capacity&(capacity-1))||capacity>0x20000||
     count>=read<uint32_t>(manager,0x1c28)||count>=capacity||
     count>=read<uint64_t>(manager,0x1c40)||count>=read<uint64_t>(manager,0x2100)||
     !read<void*>(manager,0x1be0)||!read<void*>(manager,0x1c00)||
     !read<void*>(manager,0x1c18)||!read<void*>(manager,0x1c38)||
     !read<void*>(manager,0x20f8)||!registerVfx)return false;
  auto db=read<void*>(manager,0x148);
  // Finish all fallible resource construction BEFORE inserting a native entry.
  if(!db||!build(entry,db))return false;
  registerVfx(manager,&key);
  if(!lanternVfxIndex(manager,key,index)||index!=count)return false;
  log("Lantern: private point light registered in native VFX catalogue");
 }
 const auto records=read<const Byte*>(manager,0x1c38);
 return records&&read<const void*>(records+size_t(index)*0xc0,0x10)!=nullptr;
}

uint32_t* __fastcall lanternVfxCreate(uint32_t* out,void* context,const Guid* key,
                                    const void* transform,const void* parameters) {
 size_t entry=std::size(entries);
 if(key)for(size_t i=0;i<std::size(entries);++i)if(entries[i].kind==ResourceKind::Light&&entries[i].key.guid==*key){entry=i;break;}
 if(entry==std::size(entries))return originalVfxCreate(out,context,key,transform,parameters);
 bool ready=false;
 try {
  std::lock_guard<std::mutex> lock(lanternVfxMutex);
  ready=context&&prepareLanternVfx(read<void*>(context,8),entry);
 }catch(const std::exception&){}
 if(ready){
  auto result=originalVfxCreate(out,context,key,transform,parameters);
  // Native creation copies an explicit 64-byte block when byte +64 is set.
  // Log the actual input, not just the values in the asset we constructed.
  if(parameters){
   const bool explicitValues=read<Byte>(parameters,64)!=0;
   char message[192];
   sprintf_s(message,"Lantern light: %s instance=%u explicit=%u radius=%.3f intensity=%.3f",
    *key==GoldLight?"gold":"simple",*out,unsigned(explicitValues),
    explicitValues?double(read<float>(parameters,4)):25.,
    explicitValues?double(read<float>(parameters,52)):10.);
   log(message);
  }
  return result;
 }
 // Never pass a missing key to the native lookup assertion, or substitute a
 // shared torch effect. A failed light leaves the accessory usable without VFX.
 static std::atomic<bool> reported{false};
 if(!reported.exchange(true))log("Lantern: point light unavailable; VFX instance safely omitted");
 *out=0;return out;
}

// FxStart precedes CreateInstance. Kiln checks the native catalogue here and
// clears unknown GUIDs, so register our light before forwarding to that guard.
// These offsets belong to the executable hash checked during initialization.
using FxStart=void(__fastcall*)(void*,Byte*,int64_t,void*);
FxStart originalFxStart=nullptr;
void __fastcall lanternFxStart(void* controller,Byte* entry,int64_t now,void* timing) {
 if(controller&&entry&&entry[0]==1) {
  const auto key=read<Guid>(entry,0x160);
  for(size_t i=0;i<std::size(entries);++i) {
   if(entries[i].kind!=ResourceKind::Light||entries[i].key.guid!=key)continue;
   try {
    const auto renderer=read<void*>(controller,8);
    const auto holder=renderer?read<void*>(renderer,0x98):nullptr;
    const auto scene=holder?read<void*>(holder,0x950):nullptr;
    const auto manager=scene?read<void*>(scene,8):nullptr;
    std::lock_guard<std::mutex> lock(lanternVfxMutex);
    prepareLanternVfx(manager,i);
   }catch(const std::exception&){}
   break;
  }
 }
 originalFxStart(controller,entry,now,timing);
}

#include "equipment_preview.h"
#include "lantern_fit.inl"

thread_local bool drawingEquipmentPage=false;
using DrawCharacter=void(__fastcall*)(void*,const void*,const void*);
DrawCharacter originalDrawCharacter=nullptr;
void __fastcall drawCharacter(void* context,const void* descriptor,const void* color){
 if(!drawingEquipmentPage||!equipped.load()||!owned.load()){
  originalDrawCharacter(context,descriptor,color);return;
 }
 alignas(16) std::array<unsigned char,lantern_preview::descriptorSize> copy;
 std::memcpy(copy.data(),descriptor,copy.size());
 lantern_preview::add(copy.data(),true,equippedItem.load()==lantern::native::goldItemId());
 // Native draw copies this descriptor into its frame queue; no pointer escapes.
 originalDrawCharacter(context,copy.data(),color);
}

struct PreviewWorldTransform {int64_t position[3]{};float rotation[4]{0,0,0,1};float scale[3]{1,1,1};uint32_t padding=0;};
static_assert(sizeof(PreviewWorldTransform)==56);
struct PreviewOffset {std::array<float,3> local,world;std::array<float,4> rotation;};
static_assert(sizeof(PreviewOffset)==40);
struct PreviewModels {const uint32_t* data;uint64_t count;};
struct PreviewTransforms {const PreviewWorldTransform* data;uint64_t count;};
struct PreviewApi {
 void* renderer;
 PreviewWorldTransform transform;
 uint32_t create(const void* model){
  const auto id=ui<uint32_t(__fastcall*)(void*,const void*,const void*)>(0xf38e0)(renderer,model,&transform);
  if(id){
   ui<void(__fastcall*)(void*,uint32_t,unsigned char)>(0xf6eb0)(renderer,id,1);
   if(logger)logger("Lantern: equipment preview model created");
  }
  return id;
 }
 void remove(uint32_t id){const PreviewModels ids{&id,1};ui<void(__fastcall*)(void*,const PreviewModels*)>(0xf67f0)(renderer,&ids);}
 void update(uint32_t id){
  const PreviewModels ids{&id,1};const PreviewTransforms transforms{&transform,1};
  ui<void(__fastcall*)(void*,const PreviewModels*,const PreviewTransforms*)>(0xf6060)(renderer,&ids,&transforms);
 }
};
using UpdateCharacterScene=void(__fastcall*)(void*,const void*,const void*,uint64_t);
UpdateCharacterScene originalUpdateCharacterScene=nullptr;
void __fastcall updateCharacterScene(void* scene,const void* descriptor,const void* time,uint64_t frame){
 originalUpdateCharacterScene(scene,descriptor,time,frame);
 const auto renderer=read<void*>(scene,0x20);if(!renderer)return;
 PreviewApi api{renderer,{}};
 bool enabled=equipped.load()&&owned.load()&&read<unsigned char>(descriptor,0x8d0)!=0&&
  lantern_preview::requested(static_cast<const unsigned char*>(descriptor));
 const auto skeleton=read<const void*>(scene,0xd8);
 enabled=enabled&&skeleton&&read<uint32_t>(scene,0xf8)!=0;
 if(enabled){
  const auto attachments=static_cast<const unsigned char*>(skeleton)+0x640; // native 0x9324c0
  enabled=(read<uint64_t>(attachments,0x900)&(uint64_t(1)<<3))!=0; // pelvis attachment slot
  if(enabled){
   // Match native 0x243ed0: apply the preview yaw to the animated attachment,
   // then the exact same local fit used by the in-world SlotAttachment.
   PreviewWorldTransform root{};
   const float halfYaw=-read<float>(descriptor,0x8bc)*.5f;
   enabled=std::isfinite(halfYaw);
   if(enabled){
    root.rotation[1]=std::sin(halfYaw);root.rotation[3]=std::cos(halfYaw);
    alignas(16) float rootMatrix[16]{},pelvis[16]{},combined[16]{};
    std::memcpy(pelvis,attachments+3*64,64);
    ui<void(__fastcall*)(const void*,void*,const void*)>(0xf7e670)(&root,rootMatrix,&root);
    ui<void*(__fastcall*)(void*,const void*,const void*)>(0x19fc90)(combined,pelvis,rootMatrix);
    enabled=ui<bool(__fastcall*)(void*,const void*,const void*)>(0xf7ff20)(&api.transform,combined,&root);
    if(enabled){
     const PreviewOffset offset{lantern_fit::offset(),{},lantern_fit::rotation};
     ui<void(__fastcall*)(void*,const void*)>(0x965bc0)(&api.transform,&offset);
    }
   }
  }
 }
 lantern_preview::synchronize(static_cast<unsigned char*>(scene),static_cast<const unsigned char*>(descriptor),enabled,api);
}

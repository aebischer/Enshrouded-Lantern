#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Build 1287369: 24 preview rows (88 bytes); scene-owned model pairs (40 bytes).
// The second model is an unskinned accessory. No inventory/equipment slot is used.
namespace lantern_preview {
constexpr size_t count=24, rowSize=88, descriptorSize=0x8d0, sceneRows=0xfc, sceneRowSize=40;
constexpr std::array<unsigned char,16> model={0xf1,0xb5,0x04,0x3b,0xe3,0x00,0xa1,0x5a,0xb0,0x82,0x60,0x76,0x18,0xf6,0xa0,0xe4};
constexpr std::array<unsigned char,16> goldModel={0xd7,0x6b,0x9c,0xb5,0x05,0x22,0x58,0x52,0x9c,0xa4,0xb4,0xc8,0xe6,0x09,0xe8,0xfa};
inline bool zero(const void* p,size_t n){const auto b=static_cast<const unsigned char*>(p);for(size_t i=0;i<n;++i)if(b[i])return false;return true;}
inline bool ours(const void* p){return std::memcmp(p,model.data(),model.size())==0||std::memcmp(p,goldModel.data(),goldModel.size())==0;}
inline bool requested(const unsigned char* descriptor){
 for(size_t i=16;i<count;++i)if(zero(descriptor+i*rowSize,16)&&ours(descriptor+i*rowSize+16))return true;
 return false;
}
inline int add(unsigned char* descriptor,bool enabled,bool gold=false){
 if(!enabled)return -1;
 // Leave native appearance slots 0..15 alone, including the back accessory.
 for(size_t i=count;i-->16;){auto row=descriptor+i*rowSize;if(zero(row,32)){
  std::memcpy(row+16,(gold?goldModel:model).data(),model.size());return static_cast<int>(i);
 }}
 return -1;
}
template<class Api>void synchronize(unsigned char* scene,const unsigned char* descriptor,bool enabled,Api& api){
 for(size_t i=16;i<count;++i){
  auto row=scene+sceneRows+i*sceneRowSize;
  const auto wanted=descriptor+i*rowSize;
  uint32_t id=0;std::memcpy(&id,row+36,4);
  bool own=ours(row+20);
  const bool want=enabled&&zero(wanted,16)&&ours(wanted+16)&&zero(row,20);
  if(own&&!want){if(id)api.remove(id);std::memset(row+20,0,20);continue;}
  if(!want)continue;
  if(own&&std::memcmp(row+20,wanted+16,16)){if(id)api.remove(id);std::memset(row+20,0,20);id=0;own=false;}
  if(!own||!id){
   if(!own&&!zero(row+20,20))continue; // Never replace an accessory owned by the game.
   id=api.create(wanted+16);if(!id)continue;
   std::memcpy(row+20,wanted+16,model.size());std::memcpy(row+36,&id,4);
  }
  if(id)api.update(id);
 }
}
}

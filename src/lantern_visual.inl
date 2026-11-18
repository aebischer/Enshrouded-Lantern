#include "lantern_fit.inl"
// Native accessory template: preserve attachment/render components, replace model and light.
Bytes makeVisual(const Bytes& original,bool gold=false,const Bytes* nativeLight=nullptr) {
 const auto components=array(original,16,12);
 if(components.count!=9)throw std::runtime_error("unexpected attachment baseline");
 Bytes out=original;
 const uint32_t count=components.count+(gold?2u:1u);
 const size_t rows=append(out,Bytes(count*12));
 point(out,16,rows,count);
 for(uint32_t i=0;i<components.count;++i) {
  const auto old=components.at+i*12, row=rows+i*12;
  const uint32_t type=u32(original,old);const auto data=array(original,old+4,1);
  Bytes component=slice(original,data.at,data.count);
  if(type==0x192b2ed8){if(component.size()!=16)throw std::runtime_error("model schema");putGuid(component,0,guid(gold?"d76b9cb5052258529ca4b4c8e609e8fa":"f1b5043be300a15ab082607618f6a0e4"));}
  if(type==0x09561436){
   range(component,0,48);component[41]=3;component[42]=0;
   const auto xyz=lantern_fit::offset();std::memcpy(component.data(),xyz.data(),12);
   std::memcpy(component.data()+24,lantern_fit::rotation.data(),16);
  }
  if(type==0xb3721934)putGuid(component,0,gold?GoldLight:guid("b4a8ef162fc645efb178dea679d20513"));
  if(type==0xe0122600){range(component,0,52);float xyz[]={0,-.18f,0};std::memcpy(component.data()+8,xyz,12);}
  put32(out,row,type);const auto at=append(out,component);point(out,row+4,at,static_cast<uint32_t>(component.size()));
 }
 uint32_t scaleHash=2166136261u;for(char c:std::string("keen::ecs::ScaleComponent"))scaleHash=(scaleHash^static_cast<unsigned char>(c))*16777619u;
 Bytes scale(4);float size=1.f;std::memcpy(scale.data(),&size,4);
 const size_t row=rows+components.count*12;put32(out,row,scaleHash);const auto at=append(out,scale);point(out,row+4,at,4);
 if(gold){
  if(!nativeLight)throw std::runtime_error("gold light parameter source missing");
  const auto overrideData=makeGoldLightOverride(*nativeLight);
  uint32_t type=2166136261u;for(char c:std::string("keen::ecs::VfxParametersOverwriteComponent"))type=(type^static_cast<unsigned char>(c))*16777619u;
  const auto overrideRow=rows+(components.count+1)*12;
  put32(out,overrideRow,type);const auto offset=append(out,overrideData);point(out,overrideRow+4,offset,68);
 }
 string(out,0,"Lantern_BeltLight_Visual");return out;
}

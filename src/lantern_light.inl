// Private copy of the native ground-lantern VFX. Its first independent root
// is the point light; the second root draws the visible flame. Keep only the
// light and its matching state description. Existing relative offsets, native
// programs and GPU parameter ranges stay at their original addresses.
Bytes makeLightOnly(const Bytes& original) {
 range(original,0,216);
 const auto particles=array(original,20,104),states=array(original,28,12);
 if(particles.count!=2||states.count!=2||u32(original,12)!=0)
  throw std::runtime_error("unexpected native lantern light schema");
 if(u32(original,particles.at+12)!=0xffffffffu||u32(original,particles.at+16)!=0||
    u32(original,particles.at+20)!=1)
  throw std::runtime_error("lantern light is not an independent single root");
 const auto members=array(original,states.at,20);
 bool hasIntensity=false,hasRadius=false;
 for(uint32_t i=0;i<members.count;++i){
  const auto name=array(original,members.at+i*20,1);
  const std::string text(original.begin()+name.at,original.begin()+name.at+name.count);
  hasIntensity=hasIntensity||text=="intensity";hasRadius=hasRadius||text=="radius";
  if(text=="alphaClipValue"||text=="imageSeqTimer")
   throw std::runtime_error("unexpected visual particle in lantern light");
 }
 if(!hasIntensity||!hasRadius)throw std::runtime_error("native point light state missing");
 Bytes out=original;put32(out,24,1);put32(out,32,1);
 return out;
}

// Native VfxParametersOverwriteComponent: bool at 0, 64-byte data block at 4.
// Keep all native controls, overriding only the two named float parameters.
Bytes makeGoldLightOverride(const Bytes& original) {
 range(original,116,64);
 Bytes out(68,0);
 std::memcpy(out.data()+4,original.data()+116,64);
 const auto parameters=array(original,108,12);
 unsigned found=0;
 for(uint32_t i=0;i<parameters.count;++i) {
  const size_t row=parameters.at+i*12;
  const uint32_t name=u32(original,row);
  const bool radius=name==0x6f3c8b56u,intensity=name==0x0fd9ca11u;
  if(!radius&&!intensity)continue;
  const unsigned bit=radius?1u:2u;
  const auto offset=original[row+8];
  if((found&bit)||u32(original,row+4)!=0x90b5085du||original[row+9]!=4||offset>60)
   throw std::runtime_error("unexpected lantern light parameter layout");
  float value=0;std::memcpy(&value,original.data()+116+offset,4);
  if(value!=(radius?25.f:10.f))throw std::runtime_error("unexpected lantern light parameter default");
  // Gold values retained after in-game validation; not photometric units.
  value=radius?187.5f:100.f;
  std::memcpy(out.data()+4+offset,&value,4);found|=bit;
 }
 if(found!=3)throw std::runtime_error("lantern light parameters missing");
 return out;
}

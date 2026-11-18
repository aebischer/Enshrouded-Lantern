// Private visual resources, embedded in the DLL; no external paths or archive edits.
const Guid LanternModelId=guid("f1b5043be300a15ab082607618f6a0e4");
const Guid LanternMaterialId=guid("5b5b2d57c5456456a3f5cf58ae02c84b");
const Guid GoldModelId=guid("d76b9cb5052258529ca4b4c8e609e8fa");
const Guid GoldMaterialId=guid("b2a6b2e1be11e85eb0641a01842fdcec");
struct LanternAsset {Key key;const Byte* data;size_t size;};
std::vector<LanternAsset> lanternAssets;
std::vector<LanternAsset> parseLanternAssets(const Byte* data,size_t size,const Guid& model=LanternModelId,const Guid& material=LanternMaterialId) {
 if(!data||size<12||std::memcmp(data,"LAN3D001",8))throw std::runtime_error("invalid lantern model pack");
 const auto count=read<uint32_t>(data,8);
 if(count<4||count>32||size<12+size_t(count)*32)throw std::runtime_error("invalid lantern model table");
 std::vector<LanternAsset> result;
 for(uint32_t i=0;i<count;++i) {
  const auto p=data+12+size_t(i)*32;Key key{};
  key.guid=read<Guid>(p,0);key.type=read<uint32_t>(p,16);key.part=read<uint32_t>(p,20);
  const auto offset=read<uint32_t>(p,24),length=read<uint32_t>(p,28);
  if(key.part||key.guid==Guid{}||!length||offset<12+size_t(count)*32||offset>size||length>size-offset||offset%16)
   throw std::runtime_error("lantern model range invalid");
  const bool known=key.type==0||(key.type==0x177394c7&&key.guid==model)||
   (key.type==0xaee1894b&&key.guid==material);
  if(!known||(!key.type&&read<uint32_t>(p,0)!=length))throw std::runtime_error("lantern model identity invalid");
  for(const auto& prior:result)if(prior.key.guid==key.guid&&prior.key.type==key.type)
   throw std::runtime_error("duplicate lantern model identity");
  result.push_back({key,data+offset,length});
 }
 for(const auto id:{model,material}) {
  bool found=false;for(const auto& asset:result)if(asset.key.guid==id&&asset.key.type)found=true;
  if(!found)throw std::runtime_error("lantern model resource missing");
 }
 return result;
}
void loadLanternAssets() {
 if(!lanternAssets.empty())return;
 HMODULE module=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
     reinterpret_cast<LPCWSTR>(&loadLanternAssets),&module))throw std::runtime_error("lantern model module missing");
 for(int packId:{401,402}) {
 const auto resource=FindResourceW(module,MAKEINTRESOURCEW(packId),MAKEINTRESOURCEW(10));
 if(!resource)throw std::runtime_error("embedded lantern model missing");
 const auto loaded=LoadResource(module,resource);
 if(!loaded)throw std::runtime_error("embedded model resource could not be loaded");
 auto assets=parseLanternAssets(static_cast<const Byte*>(LockResource(loaded)),SizeofResource(module,resource),packId==401?LanternModelId:GoldModelId,packId==401?LanternMaterialId:GoldMaterialId);
 for(const auto& asset:assets){
  bool duplicate=false;for(const auto& prior:lanternAssets)if(prior.key.guid==asset.key.guid&&prior.key.type==asset.key.type){
   if(prior.size!=asset.size||std::memcmp(prior.data,asset.data,asset.size))throw std::runtime_error("conflicting model packs");
   duplicate=true;break;
  }
  if(!duplicate)lanternAssets.push_back(asset);
 }
 }
}
const LanternAsset* lanternAsset(const Guid& id,uint32_t type) {
 for(const auto& a:lanternAssets)if(a.key.guid==id&&a.key.type==type)return &a;
 return nullptr;
}

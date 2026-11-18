// Record the equipment choice only after the native snapshot has succeeded.
// Item data remains in the native snapshot; the marker never contains an item.
using CharacterSnapshot=void*(__fastcall*)(void*,void*,void*,void*,void*,uint64_t,const void*);
CharacterSnapshot originalCharacterSnapshot=nullptr;
std::atomic<unsigned> snapshotDiagnostics{0};
void* __fastcall characterSnapshot(void* result,void* output,void* itemState,
 void* registry,void* layout,uint64_t time,const void* entity){
 equipment_storage::projected=false;
 auto returned=originalCharacterSnapshot(result,output,itemState,registry,layout,time,entity);
 if(returned&&output&&!read<unsigned char>(returned,0)&&equipment_storage::projected){
  try{if(!equipment_storage::record(output)&&logger)logger("Lantern: equipment choice could not be recorded; saved object remains in native bag");}
  catch(...){if(logger)logger("Lantern: equipment choice unavailable; saved object remains in native bag");}
 }
 if(!returned||!output||!entity||!logger)return returned;
 const auto actor=read<uint32_t>(entity,0);
 if(actor!=playerEntity.load()||!actor)return returned;
 const auto status=read<unsigned char>(returned,0);
 // Bound log volume; repeated autosaves must not flood the journal.
 if(snapshotDiagnostics.fetch_add(1)>=4)return returned;
 if(status){char line[128];sprintf_s(line,"Lantern save diagnostic: actor=%u native status=%u",actor,unsigned(status));logger(line);return returned;}
 unsigned occupied=0;
 for(unsigned i=0;i<16;++i)occupied+=read<unsigned char>(output,0x1a68+i*60+56)!=0;
 const auto rows=read<const unsigned char*>(output,0x1e50);
 const auto count=read<uint64_t>(output,0x1e58);
 const auto capacity=read<uint64_t>(output,0x1e60);
 if(count>248||count>capacity||(count&&!rows)){
  logger("Lantern save diagnostic: unexpected native item array; observation skipped");
  return returned;
 }
 unsigned lanterns=0;
 for(uint64_t i=0;i<count;++i)if(lantern::native::isLantern(read<uint32_t>(rows,size_t(i)*56+8)))++lanterns;
 char line[256];sprintf_s(line,
  "Lantern save diagnostic: actor=%u equipment=%u/16 genericPages=%u backpackPages=%u itemRows=%llu lanternRows=%u",
  actor,occupied,unsigned(read<unsigned char>(output,0x1e38)),
  unsigned(read<unsigned char>(output,0x1e44)),static_cast<unsigned long long>(count),lanterns);
 logger(line);
 return returned;
}

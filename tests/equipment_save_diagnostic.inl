namespace save_diagnostic_test {
template<class T>T read(const void* p,size_t at){T value;std::memcpy(&value,static_cast<const unsigned char*>(p)+at,sizeof(value));return value;}
template<class T>void write(void* p,size_t at,T value){std::memcpy(static_cast<unsigned char*>(p)+at,&value,sizeof(value));}
std::atomic<uint32_t> playerEntity{1};
std::vector<std::string> messages;
void log(const char* s){messages.emplace_back(s);}
void (*logger)(const char*)=&log;
namespace lantern::native {unsigned itemId(){return 123456;} bool isLantern(unsigned id){return id==123456||id==123457;}}
namespace equipment_storage {
thread_local bool projected=false;
bool projection=false,recordOk=true;
unsigned records=0;
bool record(const void*){++records;return recordOk;}
}
#include "../src/equipment_save_diagnostic.inl"
unsigned calls=0;
void* expected[6]{};
void* __fastcall nativeSnapshot(void* result,void* output,void* state,void* registry,void* layout,uint64_t time,const void* entity){
 ++calls;
 assert(result==expected[0]&&output==expected[1]&&state==expected[2]&&registry==expected[3]&&layout==expected[4]&&entity==expected[5]);
 assert(time==0x123456789abcdef0ull);
 equipment_storage::projected=equipment_storage::projection;
 return result;
}
void run(){
 originalCharacterSnapshot=&nativeSnapshot;
 std::array<unsigned char,0x5620> output{};
 std::array<unsigned char,112> rows{};
 std::array<unsigned char,24> entity{};
 std::array<unsigned char,8> result{};
 int state=1,registry=2,layout=3;
 write<uint32_t>(entity.data(),0,1);
 write<uint32_t>(rows.data(),8,123456);
 write<const void*>(output.data(),0x1e50,rows.data());
 write<uint64_t>(output.data(),0x1e58,2);
 write<uint64_t>(output.data(),0x1e60,2);
 output[0x1a68+56]=1;output[0x1a68+15*60+56]=1;
 output[0x1e38]=5;output[0x1e44]=2;
 expected[0]=result.data();expected[1]=output.data();expected[2]=&state;
 expected[3]=&registry;expected[4]=&layout;expected[5]=entity.data();
 auto invoke=[&]{
  const auto before=output;const auto previousRows=rows;const auto previousEntity=entity;const auto previousResult=result;
  assert(characterSnapshot(result.data(),output.data(),&state,&registry,&layout,0x123456789abcdef0ull,entity.data())==result.data());
  assert(output==before&&rows==previousRows&&entity==previousEntity&&result==previousResult);
 };
 messages.clear();snapshotDiagnostics=0;invoke();
 assert(messages.size()==1&&messages.back().find("equipment=2/16 genericPages=5 backpackPages=2 itemRows=2 lanternRows=1")!=std::string::npos);
 for(int i=0;i<10;++i)invoke();
 assert(messages.size()==4&&calls==11);
 snapshotDiagnostics=0;messages.clear();result[0]=7;invoke();
 assert(messages.size()==1&&messages.back().find("native status=7")!=std::string::npos);
 result[0]=0;write<uint64_t>(output.data(),0x1e58,249);invoke();
 assert(messages.back().find("unexpected native item array")!=std::string::npos);
 write<uint64_t>(output.data(),0x1e58,1);write<const void*>(output.data(),0x1e50,nullptr);invoke();
 assert(messages.back().find("unexpected native item array")!=std::string::npos);
 write<uint32_t>(entity.data(),0,2);const auto logged=messages.size();invoke();assert(messages.size()==logged);
 logger=nullptr;write<uint32_t>(entity.data(),0,1);invoke();assert(messages.size()==logged);
 logger=&log;
 equipment_storage::projection=true;equipment_storage::records=0;
 invoke();assert(equipment_storage::records==1);
 result[0]=7;invoke();assert(equipment_storage::records==1);
 result[0]=0;logger=nullptr;equipment_storage::recordOk=false;
 invoke();assert(equipment_storage::records==2);logger=&log;
}
}

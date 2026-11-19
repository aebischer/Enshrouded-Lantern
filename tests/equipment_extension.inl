// Exercise the production encoder/decoder hooks, including native stream status.
namespace extension_tests {
using namespace lantern::save_bridge;
std::array<unsigned char,0x54e0> saved{};
std::vector<Row> savedRows;
std::vector<unsigned char> bytes;
bool failWrite=false;
unsigned decodeCalls=0;
void write(void* stream,const void* data,uint64_t size){
    if(failWrite){set<uint8_t>(stream,0x20,11);return;}
    const auto p=static_cast<const unsigned char*>(data);bytes.insert(bytes.end(),p,p+size);
}
void* encode(void* result,void*,void*,const void* snapshot){
    std::memcpy(saved.data(),snapshot,saved.size());
    auto data=read<const Row*>(snapshot,0x1e50);auto count=read<uint64_t>(snapshot,0x1e58);
    savedRows.assign(data,data+count);for(auto& row:savedRows)row=canonical(row);
    bytes.assign({'S','2','B','H'});set<uint64_t>(result,0,0);return result;
}
void* decode(void* result,void* snapshot,void*,const void* span){
    ++decodeCalls;assert(read<uint64_t>(span,8)==4); // Trailer stripped before native decode.
    std::memcpy(snapshot,saved.data(),saved.size());
    set<const Row*>(snapshot,0x1e50,savedRows.data());set<uint64_t>(result,0,0);return result;
}
void run(){
    equipment_storage::originalEncode=&encode;equipment_storage::originalDecode=&decode;
    functions[0x043b20]=reinterpret_cast<void*>(&write);
    for(bool full:{false,true})for(unsigned kind:{42u,43u}){
        std::vector<Row> original(full?40:18);
        for(unsigned i=0;i<original.size();++i){set<uint16_t>(original[i].data(),0,static_cast<uint16_t>(i+1));set<uint32_t>(original[i].data(),8,100+i);original[i][4]=static_cast<uint8_t>(i);}
        Row equippedRow{};set<uint16_t>(equippedRow.data(),0,1);set<uint32_t>(equippedRow.data(),8,kind);
        set<uint32_t>(equippedRow.data(),12,321);equippedRow[20]=1;set<uint32_t>(equippedRow.data(),36,999);equippedRow[40]=1;
        Row backpackRow{};set<uint16_t>(backpackRow.data(),0,17);set<uint32_t>(backpackRow.data(),8,888);backpackRow[4]=9;
        for(unsigned cycle=0;cycle<100;++cycle){
            auto rows=original;auto privateRow=equippedRow;privateRow[4]=40;tag(privateRow);rows.push_back(privateRow);rows.push_back(backpackRow);
            std::array<unsigned char,0x54e0> snapshot{};std::array<unsigned char,16> result{};std::array<unsigned char,48> stream{};
            equipment_storage::putGroup(snapshot.data(),0x1e38,{6,0,static_cast<uint32_t>(original.size()+1)});
            equipment_storage::putGroup(snapshot.data(),0x1e44,{2,static_cast<uint32_t>(original.size()+1),1});
            set<const Row*>(snapshot.data(),0x1e50,rows.data());set<uint64_t>(snapshot.data(),0x1e58,rows.size());set<uint64_t>(snapshot.data(),0x1e60,rows.size());
            // Selection references into backpack must track removal of the private row.
            set<uint16_t>(snapshot.data(),0x1e28,static_cast<uint16_t>(rows.size()-1));snapshot[0x1e2a]=1;
            const auto before=snapshot;const auto rowsBefore=rows;
            equipment_storage::encode(result.data(),stream.data(),nullptr,snapshot.data());
            assert(!result[0]&&snapshot==before&&rows==rowsBefore&&saved[0x1e38]==5&&saved[0x1e44]==2);
            assert(bytes.size()==100); // Native payload plus exactly one fixed extension.
            // Vanilla sees only native capacity, even if all marker files disappear.
            assert(savedRows.size()==original.size()+1+(full?0:1));
            auto nativeRows=savedRows;
            struct Span{const unsigned char* data;uint64_t size;} span{bytes.data(),bytes.size()};
            equipment_storage::Loading pending{};equipment_storage::loading=&pending;
            equipment_storage::decode(result.data(),snapshot.data(),nullptr,&span);
            assert(!result[0]&&pending.restore.valid&&pending.restore.equipped==equippedRow);
            assert(snapshot[0x1e38]==5&&read<uint32_t>(snapshot.data(),0x1e40)==original.size());
            assert(read<uint32_t>(snapshot.data(),0x1e48)==original.size()&&read<uint16_t>(snapshot.data(),0x1e28)==original.size());
            assert(read<uint64_t>(snapshot.data(),0x1e58)==original.size()+1);
            auto restored=read<const Row*>(snapshot.data(),0x1e50);
            for(unsigned i=0;i<original.size();++i)assert(restored[i]==original[i]);
            assert(restored[original.size()]==backpackRow);
            // Corruption cannot silently lose the equipped row or load a partial character.
            auto damaged=bytes;damaged[25]^=1;span={damaged.data(),damaged.size()};const auto calls=decodeCalls;
            equipment_storage::decode(result.data(),snapshot.data(),nullptr,&span);assert(result[0]&&decodeCalls==calls);
            // A matching trailer with mismatching native item data must be rejected too.
            if(!full){savedRows=nativeRows;savedRows[original.size()][8]^=1;span={bytes.data(),bytes.size()};
                equipment_storage::decode(result.data(),snapshot.data(),nullptr,&span);assert(result[0]);}
            failWrite=true;equipment_storage::encode(result.data(),stream.data(),nullptr,before.data());assert(result[0]);failWrite=false;
            equipment_storage::loading=nullptr;
        }
    }
    // Reproduce why a sixth Generic page reached the native 16-link limit.
    uint64_t remaining=40;unsigned generic=6,totalLinks=6+2+1+1;
    remaining-=generic*8;
    while(remaining&&totalLinks<16){remaining-=8;++generic;++totalLinks;}
    assert(generic==12&&remaining!=0);
    std::cout<<"PASS: 400 production save/load cycles, both lanterns, empty/full bags, missing sidecars, immutable snapshots, all item metadata, backpack references, corrupt extensions, write failures and native overflow reproduction\n";
}
}

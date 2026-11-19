// Native inventory bridge for the pinned executable. Included after the ECS
// component helpers. Equipment is committed inside the native character section.
namespace equipment_storage {
using namespace lantern::save_bridge;
using Import=void*(__fastcall*)(void*,void*,uint32_t,uint8_t,const void*,const void*,void*);
using Decode=void*(__fastcall*)(void*,void*,void*,const void*);
using Load=void*(__fastcall*)(void*,const void*,void*,uint32_t,const void*);
using Export=void*(__fastcall*)(void*,void*,void*,void*,const void*,void*,void*,void*,void*,void*);
Import originalImport=nullptr;
Decode originalDecode=nullptr;
using DecodeStream=void*(__fastcall*)(void*,void*,void*,void*);
DecodeStream originalDecodeStream=nullptr;
Load originalLoad=nullptr;
Export originalExport=nullptr;
struct Loading { uint32_t actor=0; bool parsed=false,created=false; Restore restore{}; };
thread_local Loading* loading=nullptr;
thread_local bool projected=false;
std::atomic<uint32_t> page{0};
std::atomic<uint32_t> owner{0};
std::atomic<void*> world{nullptr};

using Cleanup=void(__fastcall*)(void*);
Cleanup originalCleanup=nullptr;
void __fastcall cleanup(void* context) {
    const auto privatePage=page.load();
    auto setup=context?read<unsigned char*>(context,0x30):nullptr;
    const auto ownedSetup=privatePage?component(world.load(),owner.load(),"keen::ecs::InventorySetup",84):View{};
    if(!setup||setup!=ownedSetup.data){originalCleanup(context);return;}
    unsigned index=16;
    for(unsigned i=0;i<16;++i)if(read<uint32_t>(setup,i*4)==privatePage&&setup[64+i]==1){index=i;break;}
    if(index==16){originalCleanup(context);return;}
    // 401200 purges EVERY category-1 page on inventory_setup, even occupied
    // pages. Exclude only ours for this synchronous call, not other cosmetics.
    // Use a known non-purged category; no ring reference is added or changed.
    struct RestoreCategory {
        void* context;uint32_t entity;
        ~RestoreCategory(){
            auto current=read<unsigned char*>(context,0x30);
            if(current)for(unsigned i=0;i<16;++i)if(read<uint32_t>(current,i*4)==entity){current[64+i]=1;return;}
            logger("Lantern: private equipment page lost during native cleanup");
        }
    } restore{context,privatePage};
    setup[64+index]=2;
    originalCleanup(context);
}

Group group(const void* snapshot,size_t at) {
    return {read<uint8_t>(snapshot,at),read<uint32_t>(snapshot,at+4),read<uint32_t>(snapshot,at+8)};
}
bool rowsOf(const void* snapshot,std::vector<Row>& rows) {
    const auto count=read<uint64_t>(snapshot,0x1e58),capacity=read<uint64_t>(snapshot,0x1e60);
    const auto data=read<const Row*>(snapshot,0x1e50);
    if(count>248||count>capacity||(count&&!data))return false;
    if(count)rows.assign(data,data+count);
    return true;
}
std::filesystem::path marker(const void* snapshot,const std::vector<Row>& rows) {
    const auto bytes=identity(group(snapshot,0x1e38),group(snapshot,0x1e44),rows);
    if(bytes.empty())return {};
    BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
    std::array<unsigned char,32> digest{};
    bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0)>=0;
    if(ok)ok=BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)>=0;
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    if(!ok)return {};
    char name[65]{};for(size_t i=0;i<digest.size();++i)sprintf_s(name+i*2,3,"%02x",digest[i]);
    return characterDirectory.parent_path()/L"equipment-markers"/(std::string(name)+".v1");
}
bool record(void* snapshot) {
    std::vector<Row> rows;if(!rowsOf(snapshot,rows))return false;
    const auto r=prepare(group(snapshot,0x1e38),group(snapshot,0x1e44),rows,lantern::native::itemId(),lantern::native::goldItemId());
    if(!r.valid)return false;
    tag(read<Row*>(snapshot,0x1e50)[r.index]);return true;
}
void putGroup(void* snapshot,size_t at,Group value){
    set<uint8_t>(snapshot,at,value.pages);set<uint32_t>(snapshot,at+4,value.begin);set<uint32_t>(snapshot,at+8,value.count);
}
// Character data is decoded/re-encoded between the server snapshot and the
// final KSD section. Rebuild the transient private row for that intermediate
// snapshot so the next encoder can project it again, without growing the save.
bool retainExtension(void* snapshot,const Extension& ext,std::vector<Row> rows) {
    auto bag=group(snapshot,0x1e38),backpack=group(snapshot,0x1e44);
    const auto index=read<uint32_t>(ext.data(),12),mode=read<uint32_t>(ext.data(),8);
    Row equippedRow{};std::memcpy(equippedRow.data(),ext.data()+16,56);
    if(!valid(bag,rows.size())||!valid(backpack,rows.size())||bag.pages>=16)return false;
    if(mode==1){
        if(!bag.count||index!=bag.begin+bag.count-1||index>=rows.size())return false;
        auto actual=canonical(rows[index]);actual[4]=0;
        if(actual!=equippedRow)return false;
    }else{
        if(index!=bag.begin+bag.count||index>rows.size()||rows.size()>=248)return false;
        rows.insert(rows.begin()+index,equippedRow);++bag.count;
        if(backpack.begin>=index)++backpack.begin;
    }
    equippedRow[4]=static_cast<uint8_t>(bag.pages*8);tag(equippedRow);
    rows[index]=equippedRow;++bag.pages;
    auto vector=static_cast<unsigned char*>(snapshot)+0x1e50;
    if(rows.size()>read<uint64_t>(vector,16)){
        // Same native grow callback used by e1afc0. Never replace the allocator.
        const auto grow=read<uint8_t(__fastcall*)(void*,int64_t,uint64_t)>(vector,24);
        if(!grow||!grow(vector,-static_cast<int64_t>(rows.size()),0))return false;
    }
    auto data=read<Row*>(vector,0);
    if(!data||rows.size()>read<uint64_t>(vector,16))return false;
    std::memcpy(data,rows.data(),rows.size()*sizeof(Row));set<uint64_t>(vector,8,rows.size());
    putGroup(snapshot,0x1e38,bag);putGroup(snapshot,0x1e44,backpack);
    if(mode==2)for(size_t at:{size_t(0x1e28),size_t(0x1e2c),size_t(0x1e30),size_t(0x1e34)}){
        const auto value=read<uint16_t>(snapshot,at);
        if(read<uint8_t>(snapshot,at+2)&&value>=index)set<uint16_t>(snapshot,at,static_cast<uint16_t>(value+1));
    }
    return true;
}
void adjustReferences(void* snapshot,uint32_t removed){
    for(size_t at:{size_t(0x1e28),size_t(0x1e2c),size_t(0x1e30),size_t(0x1e34)}){
        const auto value=read<uint16_t>(snapshot,at);
        if(read<uint8_t>(snapshot,at+2)&&value>removed)set<uint16_t>(snapshot,at,static_cast<uint16_t>(value-1));
    }
}
// e37100 serializes a character snapshot into the native output stream. The
// extension is inside the same KSD section/commit, after the native S2BH block.
// Native readers ignore trailing section bytes. No page count is enlarged on disk.
using Encode=void*(__fastcall*)(void*,void*,void*,const void*);
Encode originalEncode=nullptr;
void* __fastcall encode(void* result,void* stream,void* compressor,const void* snapshot){
    try{
        std::vector<Row> rows;
        if(!rowsOf(snapshot,rows))return originalEncode(result,stream,compressor,snapshot);
        bool ours=false;for(const auto& row:rows)ours=ours||tagged(row);
        if(!ours)return originalEncode(result,stream,compressor,snapshot);
        const auto p=project(group(snapshot,0x1e38),group(snapshot,0x1e44),rows,lantern::native::itemId(),lantern::native::goldItemId());
        if(!p.valid){set<uint64_t>(result,0,0x11);return result;}
        std::array<unsigned char,0x54e0> copy{};std::memcpy(copy.data(),snapshot,copy.size());
        putGroup(copy.data(),0x1e38,p.bag);putGroup(copy.data(),0x1e44,p.backpack);
        set<const Row*>(copy.data(),0x1e50,p.rows.data());set<uint64_t>(copy.data(),0x1e58,p.rows.size());set<uint64_t>(copy.data(),0x1e60,p.rows.size());
        if(!p.inBag)adjustReferences(copy.data(),p.index);
        const auto returned=originalEncode(result,stream,compressor,copy.data());
        if(!returned||read<uint8_t>(returned,0))return returned;
        const auto ext=extension(p);
        ui<void(__fastcall*)(void*,const void*,uint64_t)>(0x043b20)(stream,ext.data(),ext.size());
        set<uint8_t>(returned,0,read<uint8_t>(stream,0x20));
        static std::atomic<unsigned> reports{0};
        if(!read<uint8_t>(returned,0)&&reports.fetch_add(1)<2)logger("Lantern: equipped instance encoded with bounded inventory extension");
        return returned;
    }catch(...){set<uint64_t>(result,0,0x11);logger("Lantern: character encoding refused; previous save retained");return result;}
}
void* __fastcall load(void* result,const void* blob,void* server,uint32_t index,const void* argument) {
    Loading state{};
    struct Scope {Loading* previous;~Scope(){loading=previous;}} scope{loading};
    loading=&state;
    const auto returned=originalLoad(result,blob,server,index,argument);
    return returned;
}
// e0f470 is only the span adapter used by world import. Native character
// synchronization (dd30f0, e2356c) calls the underlying stream reader directly.
// Its contiguous buffer uses data/+0, size/+8, cursor/+0x18. Inspect the trailer
// before decoding: compressed readers can consume their input in large blocks.
void* __fastcall decodeStream(void* result,void* snapshot,void* allocator,void* stream) {
    Extension ext{};bool extended=false;
    if(stream&&!loading){
        const auto data=read<const unsigned char*>(stream,0);
        const auto size=read<uint64_t>(stream,8),cursor=read<uint64_t>(stream,0x18);
        if(data&&size<=16*1024*1024&&cursor<=size&&size-cursor>=ext.size()&&
           !std::memcmp(data+size-8,"LNTEND02",8)){
            std::memcpy(ext.data(),data+size-ext.size(),ext.size());
            if(!extensionValid(ext,lantern::native::itemId(),lantern::native::goldItemId())){
                set<uint64_t>(result,0,0x11);logger("Lantern: damaged streamed equipment extension; decode refused");return result;
            }
            extended=true;
        }
    }
    const auto returned=originalDecodeStream(result,snapshot,allocator,stream);
    if(extended&&returned&&!read<uint8_t>(returned,0)&&snapshot)try{
        std::vector<Row> rows;
        if(!rowsOf(snapshot,rows)||!retainExtension(snapshot,ext,std::move(rows))){
            set<uint64_t>(result,0,0x11);logger("Lantern: streamed equipment snapshot refused");
        }else{static std::atomic<unsigned> reports{0};if(reports.fetch_add(1)<2)logger("Lantern: equipped instance retained through native stream decode");}
    }catch(...){set<uint64_t>(result,0,0x11);}
    return returned;
}
void* __fastcall decode(void* result,void* snapshot,void* allocator,const void* span) {
    struct Span {const unsigned char* data;uint64_t size;};
    auto input=read<Span>(span,0);Extension ext{};bool extended=false;
    if(input.data&&input.size>=ext.size()&&input.size<=16*1024*1024&&
       !std::memcmp(input.data+input.size-8,"LNTEND02",8)){
        std::memcpy(ext.data(),input.data+input.size-ext.size(),ext.size());
        if(!extensionValid(ext,lantern::native::itemId(),lantern::native::goldItemId())){
            set<uint64_t>(result,0,0x11);logger("Lantern: damaged equipment extension; character load refused");return result;
        }
        input.size-=ext.size();extended=true;
    }
    const auto returned=originalDecode(result,snapshot,allocator,extended?&input:span);
    if(!returned||read<uint8_t>(returned,0)||!snapshot)return returned;
    if(!loading){
        if(extended)try{
            std::vector<Row> rows;
            if(!rowsOf(snapshot,rows)||!retainExtension(snapshot,ext,std::move(rows))){
                set<uint64_t>(result,0,0x11);logger("Lantern: intermediate equipment snapshot refused");
            }else {static std::atomic<unsigned> reports{0};if(reports.fetch_add(1)<2)logger("Lantern: equipped instance retained through intermediate character decode");}
        }catch(...){set<uint64_t>(result,0,0x11);}
        return returned;
    }
    loading->parsed=true;
    try {
        std::vector<Row> rows;if(!rowsOf(snapshot,rows))return returned;
        if(extended){
            Row equippedRow{};std::memcpy(equippedRow.data(),ext.data()+16,56);
            auto bag=group(snapshot,0x1e38),backpack=group(snapshot,0x1e44);
            const auto index=read<uint32_t>(ext.data(),12),mode=read<uint32_t>(ext.data(),8);
            if(!valid(bag,rows.size())||!valid(backpack,rows.size())){set<uint64_t>(result,0,0x11);return returned;}
            if(mode==1){
                if(!bag.count||index!=bag.begin+bag.count-1||index>=rows.size()){set<uint64_t>(result,0,0x11);return returned;}
                auto actual=canonical(rows[index]);actual[4]=0;
                if(actual!=equippedRow){set<uint64_t>(result,0,0x11);return returned;}
                rows.erase(rows.begin()+index);--bag.count;if(backpack.begin>index)--backpack.begin;
                std::memcpy(read<Row*>(snapshot,0x1e50),rows.data(),rows.size()*sizeof(Row));
                set<uint64_t>(snapshot,0x1e58,rows.size());putGroup(snapshot,0x1e38,bag);putGroup(snapshot,0x1e44,backpack);
                adjustReferences(snapshot,index);
            }else if(index!=bag.begin+bag.count){set<uint64_t>(result,0,0x11);return returned;}
            loading->restore={true,index,bag,equippedRow};return returned;
        }
        // One-time import of pre-0.1.28 saves. Missing markers must not leave
        // the old extra page to overflow native inventory initialization.
        auto restore=prepare(group(snapshot,0x1e38),group(snapshot,0x1e44),rows,lantern::native::itemId(),lantern::native::goldItemId());
        if(!restore.valid)return returned;
        const auto path=marker(snapshot,rows);std::ifstream file(path,std::ios::binary);
        std::string line;std::getline(file,line);
        if(line!="Lantern equipment choice 1"&&restore.bag.pages!=5)return returned;
        putGroup(snapshot,0x1e38,restore.bag);adjustReferences(snapshot,restore.index);
        loading->restore=restore;
    }catch(...){set<uint64_t>(result,0,0x11);logger("Lantern: inventory restoration refused; previous save retained");}
    return returned;
}
void* __fastcall import(void* result,void* references,uint32_t actor,uint8_t category,
                       const void* descriptor,const void* items,void* context) {
    const auto returned=originalImport(result,references,actor,category,descriptor,items,context);
    if(!loading||!loading->parsed||!returned||read<uint8_t>(returned,0))return returned;
    if(category==2&&!loading->actor)loading->actor=actor;
    if(category!=4||actor!=loading->actor||loading->created)return returned;
    loading->created=true;
    // Native e560f0 passes context[0] to e9b2f20's scene-level accessor.
    // Our component() helper takes the ECS storage embedded in that scene,
    // exactly as serverStep does, not the scene header itself.
    const auto scene=context?read<uintptr_t>(context,0):0;
    if(!scene){set<uint64_t>(result,0,0x11);return returned;}
    const auto inventoryWorld=reinterpret_cast<void*>(scene+0xe179c0);
    const auto setup=component(inventoryWorld,actor,"keen::ecs::InventorySetup",84);
    unsigned links=0;
    if(setup.data)for(;links<16&&read<uint32_t>(setup.data,links*4);++links)
        if(setup.data[64+links]==1){set<uint64_t>(result,0,0x11);return returned;}
    if(!setup.data||links>=16){set<uint64_t>(result,0,0x11);return returned;}
    // e560f0 consumes native row values and recreates persistent item data.
    // Its output references are private, so native ring/ammo indices do not shift.
    Row row=loading->restore.valid?loading->restore.equipped:Row{};
    struct Span {const Row* data;uint64_t count;} source{&row,1};
    alignas(8) unsigned char meta[12]{};meta[0]=1;set<uint32_t>(meta,8,1);
    struct References {uint64_t* data;uint64_t count,capacity;void* allocator;uint64_t flags,value;};
    References privateReferences{};privateReferences.data=&privateReferences.value;privateReferences.capacity=1;
    originalImport(result,&privateReferences,actor,1,meta,&source,context);
    if(read<uint8_t>(result,0))return returned;
    const auto after=component(inventoryWorld,actor,"keen::ecs::InventorySetup",84);
    if(!after.data||after.data[64+links]!=1){set<uint64_t>(result,0,0x11);return returned;}
    world=inventoryWorld;owner=actor;page=read<uint32_t>(after.data,links*4);
    logger(loading->restore.valid?"Lantern: equipped instance restored through native inventory import":"Lantern: private equipment inventory created");
    return returned;
}
void* __fastcall exportItems(void* result,void* snapshot,void* descriptor,void* output,
                            const void* setup,void* slots,void* query,void* state,void* layout,void* entity) {
    const auto privatePage=page.load();
    if(!privatePage||!setup||!entity||read<uint32_t>(entity,0)!=owner.load())
        return originalExport(result,snapshot,descriptor,output,setup,slots,query,state,layout,entity);
    unsigned found=16,count=0;
    for(;count<16&&read<uint32_t>(setup,count*4);++count)
        if(read<uint32_t>(setup,count*4)==privatePage&&read<uint8_t>(setup,64+count)==1)found=count;
    if(found==16)return originalExport(result,snapshot,descriptor,output,setup,slots,query,state,layout,entity);
    // Only export an equipped page. Empty private pages must never expand the bag.
    const auto inventory=component(world.load(),privatePage,"keen::ecs::Inventory",96);
    bool occupied=false;
    if(inventory.data)for(unsigned i=0;i<8;++i)occupied=occupied||
        (read<uint32_t>(inventory.data,i*12)!=0&&read<uint32_t>(inventory.data,i*12+4)!=0);
    // Native empty slots can retain an old item ID with zero quantity.
    // Unexpected occupied contents fail snapshot validation rather than being lost.
    if(!occupied)
        return originalExport(result,snapshot,descriptor,output,setup,slots,query,state,layout,entity);
    std::array<unsigned char,84> copy{};std::memcpy(copy.data(),setup,copy.size());
    for(unsigned i=found;i+1<count;++i){set<uint32_t>(copy.data(),i*4,read<uint32_t>(setup,(i+1)*4));copy[64+i]=read<uint8_t>(setup,65+i);}
    set<uint32_t>(copy.data(),(count-1)*4,privatePage);copy[64+count-1]=4;
    const auto returned=originalExport(result,snapshot,descriptor,output,copy.data(),slots,query,state,layout,entity);
    if(returned&&!read<uint8_t>(returned,0))projected=true;
    return returned;
}
}

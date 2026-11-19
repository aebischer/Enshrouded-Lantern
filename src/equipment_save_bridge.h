#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace lantern::save_bridge {
using Row = std::array<std::uint8_t, 56>;
template<class T> inline T get(const void* data, std::size_t offset) {
    T result{}; std::memcpy(&result, static_cast<const std::uint8_t*>(data)+offset, sizeof(result)); return result;
}
template<class T> inline void set(void* data, std::size_t offset, T value) {
    std::memcpy(static_cast<std::uint8_t*>(data)+offset, &value, sizeof(value));
}
struct Group { std::uint8_t pages=0; std::uint32_t begin=0, count=0; };
inline bool valid(Group group, std::size_t count) {
    return group.pages<=16 && group.begin<=count && group.count<=count-group.begin;
}
// Native optional fields have padding and inactive payload bytes. Neither is
// part of item identity and neither may influence the restore marker.
inline Row canonical(const Row& input) {
    Row out{};
    std::memcpy(out.data(), input.data(), 2);
    out[4]=input[4];
    std::memcpy(out.data()+8, input.data()+8, 4);
    const std::array<std::array<std::size_t,3>,6> fields{{
        {{2,1,3}}, {{5,2,7}}, {{12,8,20}}, {{24,8,32}}, {{36,4,40}}, {{44,5,52}}
    }};
    for (const auto& field:fields) if (input[field[2]]) {
        out[field[2]]=1;
        std::memcpy(out.data()+field[0],input.data()+field[0],field[1]);
    }
    return out;
}
inline std::vector<std::uint8_t> identity(Group bag, Group backpack, const std::vector<Row>& rows) {
    if (rows.size()>248 || !valid(bag,rows.size()) || !valid(backpack,rows.size())) return {};
    std::vector<std::uint8_t> bytes;
    for(const auto group:{bag,backpack}) {
        bytes.push_back(group.pages);
        for(auto value:{group.begin,group.count}) for(unsigned shift=0;shift<32;shift+=8)
            bytes.push_back(static_cast<std::uint8_t>(value>>shift));
    }
    for(const auto& row:rows) { const auto value=canonical(row); bytes.insert(bytes.end(),value.begin(),value.end()); }
    return bytes;
}
struct Restore {
    bool valid=false;
    std::uint32_t index=0;
    Group bag{};
    Row equipped{};
};
// The native exporter puts the private page LAST among the main bag pages.
// A marker only identifies that existing row; it never supplies another item.
inline Restore prepare(Group bag, Group backpack, const std::vector<Row>& rows,
                       std::uint32_t lanternItem,std::uint32_t secondItem=0) {
    Restore result{};
    if (!lanternItem || rows.size()>248 || !valid(bag,rows.size()) || !valid(backpack,rows.size()) ||
        bag.pages<2 || !bag.count || (backpack.count && bag.begin+bag.count>backpack.begin)) return result;
    const auto index=bag.begin+bag.count-1;
    const auto& row=rows[index];
    if ((get<std::uint32_t>(row.data(),8)!=lanternItem && (!secondItem || get<std::uint32_t>(row.data(),8)!=secondItem)) || get<std::uint16_t>(row.data(),0)!=1 ||
        row[4]!=(bag.pages-1)*8) return result;
    for(std::uint32_t i=bag.begin;i<index;++i) if(rows[i][4]>=(bag.pages-1)*8) return result;
    result.valid=true; result.index=index; result.bag=bag;
    --result.bag.pages; --result.bag.count;
    result.equipped=row; result.equipped[4]=0;
    return result;
}
// Transient tag in padding that the native item codec never serializes.
inline void tag(Row& row) { row[53]='L'; row[54]='N'; row[55]='T'; }
inline bool tagged(const Row& row) { return row[53]=='L' && row[54]=='N' && row[55]=='T'; }
struct Projection {
    bool valid=false, inBag=false;
    Group bag{}, backpack{};
    std::vector<Row> rows;
    Row equipped{};
    std::uint32_t index=0;
};
inline Projection project(Group bag, Group backpack, const std::vector<Row>& rows,
                          std::uint32_t first, std::uint32_t second) {
    Projection out{};
    const auto restore=prepare(bag,backpack,rows,first,second);
    if(!restore.valid || !tagged(rows[restore.index]))return out;
    out.bag=restore.bag;out.backpack=backpack;out.rows=rows;
    out.equipped=canonical(restore.equipped);out.index=restore.index;
    std::array<bool,128> used{};
    for(std::uint32_t i=bag.begin;i<restore.index;++i){
        const auto slot=rows[i][4];if(slot>=unsigned(out.bag.pages)*8 || used[slot])return {};
        used[slot]=true; // Preserve zero-count native rows too.
    }
    unsigned free=0;for(;free<unsigned(out.bag.pages)*8 && used[free];++free){}
    if(free<unsigned(out.bag.pages)*8){
        out.inBag=true;out.rows[restore.index]=out.equipped;
        out.rows[restore.index][4]=static_cast<std::uint8_t>(free);++out.bag.count;
    }else{
        out.rows.erase(out.rows.begin()+restore.index);
        if(out.backpack.begin>restore.index)--out.backpack.begin;
    }
    out.valid=true;return out;
}
using Extension=std::array<std::uint8_t,96>;
inline std::uint64_t checksum(const std::uint8_t* p,std::size_t size){
    std::uint64_t v=14695981039346656037ull;
    for(std::size_t i=0;i<size;++i)v=(v^p[i])*1099511628211ull;
    return v;
}
inline Extension extension(const Projection& p){
    Extension e{};std::memcpy(e.data(),"LNTINV02",8);
    set<std::uint32_t>(e.data(),8,p.inBag?1u:2u);set<std::uint32_t>(e.data(),12,p.index);
    std::memcpy(e.data()+16,p.equipped.data(),56);
    set<std::uint64_t>(e.data(),80,checksum(e.data(),80));
    std::memcpy(e.data()+88,"LNTEND02",8);return e;
}
inline bool extensionValid(const Extension& e,std::uint32_t first,std::uint32_t second){
    const auto mode=get<std::uint32_t>(e.data(),8),id=get<std::uint32_t>(e.data(),24);
    return !std::memcmp(e.data(),"LNTINV02",8)&&!std::memcmp(e.data()+88,"LNTEND02",8)&&
        (mode==1||mode==2)&&get<std::uint64_t>(e.data(),72)==0&&
        get<std::uint64_t>(e.data(),80)==checksum(e.data(),80)&&
        (id==first||id==second)&&id&&get<std::uint16_t>(e.data(),16)==1&&e[20]==0;
}

}

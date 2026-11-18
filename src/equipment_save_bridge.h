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
}

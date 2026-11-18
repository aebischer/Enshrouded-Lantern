#include "../src/equipment_save_bridge.h"
namespace save_bridge_test {
void run() {
    using namespace lantern::save_bridge;
    Row ordinary{},lantern{},backpackItem{};
    set<std::uint16_t>(ordinary.data(),0,7); set<std::uint32_t>(ordinary.data(),8,300);
    set<std::uint16_t>(lantern.data(),0,1); set<std::uint32_t>(lantern.data(),8,100); lantern[4]=40;
    lantern[20]=1; set<std::uint32_t>(lantern.data(),12,987); set<std::uint32_t>(lantern.data(),16,654);
    set<std::uint16_t>(backpackItem.data(),0,2); set<std::uint32_t>(backpackItem.data(),8,400);
    const std::vector<Row> rows{ordinary,lantern,backpackItem};
    const Group bag{6,0,2}, backpack{2,2,1};
    const auto restore=prepare(bag,backpack,rows,100);
    assert(restore.valid && restore.index==1 && restore.bag.pages==5 && restore.bag.count==1);
    assert(restore.equipped[4]==0 && get<std::uint32_t>(restore.equipped.data(),12)==987);
    assert(rows[1]==lantern && rows[2]==backpackItem); // No source removal or duplication.
    auto dirty=rows; dirty[1][21]=0x55; dirty[1][36]=0xee; // Padding / absent optional field.
    assert(identity(bag,backpack,dirty)==identity(bag,backpack,rows));
    dirty[1][12]^=1;
    assert(identity(bag,backpack,dirty)!=identity(bag,backpack,rows));
    assert(!prepare({6,0,99},backpack,rows,100).valid);
    assert(!prepare(bag,{2,1,1},rows,100).valid);
    dirty=rows; dirty[0][4]=41;
    assert(!prepare(bag,backpack,dirty,100).valid); // Another item on the reserved page.
    dirty=rows; dirty[1][0]=2;
    assert(!prepare(bag,backpack,dirty,100).valid);
    dirty=rows; dirty[1][4]=39;
    assert(!prepare(bag,backpack,dirty,100).valid);
    assert(!prepare(bag,backpack,rows,200).valid);
    std::cout<<"PASS: native save projection, instance metadata, padding-independent marker, malformed ranges and occupied reserved page rejection\n";
}
}

Bytes makeItem(const Bytes& input,bool gold=false) {
 const auto& selectedItem=gold?GoldItem:content::Item;const auto& selectedName=gold?GoldName:content::ItemName;
 const auto& selectedDescription=gold?GoldDescription:lantern::native::Description;
 const auto& selectedVisualId=gold?GoldVisual:lantern::native::VisualId;const auto& selectedIcon=gold?GoldIcon:content::Icon;
 range(input,0,2088);Bytes out=input;
 put32(out,0,hash(selectedItem));putGuid(out,4,selectedItem);out[20]=1;out[21]=0;
 out[22]=0;out[23]=0;put32(out,1652,1);put32(out,1656,1);
 putGuid(out,72,selectedName);putGuid(out,88,Caption);putGuid(out,104,selectedDescription);
 putGuid(out,120,Guid{});putGuid(out,136,Guid{});
 std::memset(out.data()+152,0,52);out[204]=3;
 out[260]=0; // No native slot: equipment is owned by the lantern adapter.
 std::memset(out.data()+264,0,1084);
 putGuid(out,0x148,selectedVisualId);
 putGuid(out,1388,selectedIcon);putGuid(out,1404,Guid{});putGuid(out,1420,Guid{});
 std::memset(out.data()+1544,0,100); // No inherited ring perks or light impact.
 put32(out,1512,0);put32(out,1516,0);put32(out,1520,0);put32(out,1524,0);put32(out,1528,0);put32(out,1532,0);
 string(out,2080,gold?"Lantern_Gilded":"Lantern_BeltLight");return out;
}

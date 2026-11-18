#include "../src/equipment_preview.h"
namespace preview_test {
struct Api {
 unsigned creates=0,updates=0,removes=0;uint32_t result=123;
 uint32_t create(const void*){++creates;return result;}
 void update(uint32_t id){assert(id==result);++updates;}
 void remove(uint32_t id){assert(id==result);++removes;}
};
void run(){
 using namespace lantern_preview;
 std::array<unsigned char,descriptorSize+1> descriptor{},empty{};
 std::array<unsigned char,0x510> scene{};
 Api api;
 assert(add(descriptor.data(),false)==-1&&descriptor==empty);
 assert(add(descriptor.data(),true)==23);
 const auto tagged=descriptor;
 auto row=scene.data()+sceneRows+23*sceneRowSize;
 synchronize(scene.data(),descriptor.data(),true,api);
 assert(api.creates==1&&api.updates==1&&ours(row+20)&&zero(row,20));
 synchronize(scene.data(),descriptor.data(),true,api);
 assert(api.creates==1&&api.updates==2); // No per-frame model allocation.
 assert(descriptor==tagged); // Renderer input remains immutable.
 synchronize(scene.data(),descriptor.data(),false,api);
 assert(api.removes==1&&zero(row,40)); // Unequip or missing pelvis.
 synchronize(scene.data(),descriptor.data(),false,api);
 assert(api.removes==1);
 synchronize(scene.data(),descriptor.data(),true,api);
 assert(api.creates==2);
 synchronize(scene.data(),empty.data(),true,api);
 assert(api.removes==2&&zero(row,40)); // Close N, native scene reset, other preview.
 api.result=0;
 synchronize(scene.data(),descriptor.data(),true,api);
 assert(zero(row,40)); // Failed native create is retried, never published as valid.
 api.result=123;
 row[20]=0x55;
 const auto occupied=scene;
 synchronize(scene.data(),descriptor.data(),true,api);
 assert(scene==occupied); // Native accessory is never replaced.
 scene={};row[0]=0x55;
 const auto primary=scene;
 synchronize(scene.data(),descriptor.data(),true,api);
 assert(scene==primary); // Native appearance model takes precedence.
 descriptor={};
 for(size_t i=16;i<count;++i)descriptor[i*rowSize]=0x55;
 auto full=descriptor;
 assert(add(descriptor.data(),true)==-1&&descriptor==full);
 descriptor[20*rowSize]=0;
 assert(add(descriptor.data(),true)==20); // Find an available slot without touching others.
 std::memset(descriptor.data()+20*rowSize+16,0,16);
 assert(descriptor[20*rowSize]==0);
 full[20*rowSize]=0;assert(descriptor==full);
 descriptor={};scene={};api={};
 add(descriptor.data(),true);synchronize(scene.data(),descriptor.data(),true,api);
 descriptor={};add(descriptor.data(),true,true);synchronize(scene.data(),descriptor.data(),true,api);
 assert(api.creates==2&&api.removes==1&&!std::memcmp(row+20,goldModel.data(),16));
 synchronize(scene.data(),descriptor.data(),true,api);assert(api.creates==2&&api.removes==1);
 descriptor={};add(descriptor.data(),true);synchronize(scene.data(),descriptor.data(),true,api);
 assert(api.creates==3&&api.removes==2&&!std::memcmp(row+20,model.data(),16));
 std::cout<<"PASS: equipment preview equip/unequip, repeated frames, close/reset, failed creation, occupied native rows\n";
}
}

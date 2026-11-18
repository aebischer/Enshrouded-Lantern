#include "../src/content_catalog.h"
#include "../../CarryOn/src/content_catalog.h"
#include <fstream>
#include <iostream>
#include <cassert>
#include <cmath>
#include <mutex>
#include <atomic>
#include "vfx.inl"
#include "equipment_preview.inl"
#include "preview_runtime.inl"
#include "equipment_save_diagnostic.inl"
#include "equipment_selection.inl"
#include "equipment_save_bridge.inl"
using namespace lantern::content;
Bytes load(const char* file){std::ifstream f(file,std::ios::binary);if(!f)throw std::runtime_error(file);return Bytes(std::istreambuf_iterator<char>(f),{});}
namespace visual_test {
using namespace lantern::content;
#include "../src/lantern_light.inl"
#include "../src/lantern_visual.inl"
}
int main(){
 vfx_test::run();
 preview_test::run();
 preview_runtime_test::run();
 save_diagnostic_test::run();
 selection_test::run();
 save_bridge_test::run();
 auto items=load("../../work/resource-5dd364304273ca40bdc9aad58f83bf45-0x72ab39b6-0.bin");
 const auto nativeRefs=array(items,0,16);
 for(const auto& required:{IronBar,GoldBar,Candle}){
  unsigned matches=0;for(unsigned j=0;j<nativeRefs.count;++j)if(equal(items,nativeRefs.at+j*16,required))++matches;
  assert(matches==1);
 }
 auto recipes=load("../../work/resource-0fde575e0c2859458191f02c1ba656e2-0x5f5382cf-0.bin");
 auto menu=load("../../work/resource-f64de1468ffb664f9463e23b9e19e48e-0xfef918b8-0.bin");
 for(bool first:{false,true}){
  auto i=first?carryon::content::add_item_registry(add_item_registry(items)):add_item_registry(carryon::content::add_item_registry(items));
  assert(array(i,0,16).count==array(items,0,16).count+3);
  auto r=first?carryon::content::add_recipe_registry(add_recipe_registry(recipes)):add_recipe_registry(carryon::content::add_recipe_registry(recipes));
  const auto rows=array(r,8,168);assert(rows.count==array(recipes,8,168).count+3);
  unsigned found=0;
  for(bool gold:{false,true}) {
  const auto& expectedRecipe=gold?GoldRecipe:Recipe;const auto& expectedItem=gold?GoldItem:Item;const auto& metal=gold?GoldBar:IronBar;
  for(unsigned j=0;j<rows.count;++j){size_t row=rows.at+j*168;if(!equal(r,row,expectedRecipe))continue;++found;
   const auto ing=array(r,row+40,48),out=array(r,row+48,24);
   assert(ing.count==2&&out.count==1&&equal(r,ing.at,metal)&&u32(r,ing.at+16)==hash(metal)&&u32(r,ing.at+20)==2&&equal(r,ing.at+48,Candle)&&u32(r,ing.at+64)==hash(Candle)&&u32(r,ing.at+68)==1&&equal(r,out.at,expectedItem)&&u32(r,out.at+20)==1);
  }
  }assert(found==2);
  auto m=first?carryon::content::add_crafting_menu(add_crafting_menu(menu)):add_crafting_menu(carryon::content::add_crafting_menu(menu));
  assert(m.size()>menu.size());
 }
 auto nativeLight=load("../../work/resource-18de49ca6f61874b8c0019efe3f027bb-0xe64ee036-0.bin");
 auto goldVisual=visual_test::makeVisual(load("../../work/resource-618b3ac8dbea284d914688c4c8d9cb98-0x39768775-0.bin"),true,&nativeLight);
 const auto components=array(goldVisual,16,12);unsigned goldModels=0;
 for(unsigned j=0;j<components.count;++j){const auto row=components.at+j*12;if(u32(goldVisual,row)==0x192b2ed8){
  const auto model=array(goldVisual,row+4,1);assert(equal(goldVisual,model.at,guid("d76b9cb5052258529ca4b4c8e609e8fa")));++goldModels;
 }}assert(goldModels==1);
 auto donor=load("../../work/resource-618b3ac8dbea284d914688c4c8d9cb98-0x39768775-0.bin");
 auto visual=visual_test::makeVisual(donor);const auto parts=array(visual,16,12);assert(parts.count==10);
 bool model=false,attachment=false,light=false,scale=false;
 for(unsigned j=0;j<parts.count;++j){const auto row=parts.at+j*12;const auto data=array(visual,row+4,1);const auto type=u32(visual,row);
  if(type==0x192b2ed8){model=equal(visual,data.at,guid("f1b5043be300a15ab082607618f6a0e4"));}
  if(type==0x09561436){attachment=data.count==48&&visual[data.at+41]==3&&visual[data.at+42]==0;
   float q[4];std::memcpy(q,visual.data()+data.at+24,sizeof(q));
   assert(std::abs(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]-1.f)<1e-6f);
   // The measured strap contact stays on the belt after the inward lean.
   const auto anchor=visual_test::lantern_fit::rotate(visual_test::lantern_fit::strapAnchor);
   float offset[3];std::memcpy(offset,visual.data()+data.at,12);
   for(unsigned k=0;k<3;++k)assert(std::abs(anchor[k]+offset[k]-visual_test::lantern_fit::beltAnchor[k])<1e-6f);
   const auto down=visual_test::lantern_fit::rotate({0,-1,0});
   assert(down[0]<-.97f&&down[1]>.14f&&down[1]<.15f&&std::abs(down[1]+down[2])<1e-6f);
   const auto belt=visual_test::lantern_fit::beltAnchor;
   assert(belt[1]<0&&belt[2]>0&&std::abs(belt[1]+belt[2])<1e-6f);
   assert(std::abs(belt[1]*belt[1]+belt[2]*belt[2]-.22f*.22f)<1e-6f);
   // Upper loop faces diagonally inward at the new front-quarter location.
   const auto inward=visual_test::lantern_fit::rotate({0,0,-1});
   assert(inward[1]>.69f&&inward[2]<-.69f);}

  if(type==0xb3721934){light=equal(visual,data.at,guid("b4a8ef162fc645efb178dea679d20513"));}
  if(data.count==4){float value=0;std::memcpy(&value,visual.data()+data.at,4);scale=scale||value==1.f;}
 }assert(model&&attachment&&light&&scale);
 auto lightOnly=visual_test::makeLightOnly(nativeLight);
 const auto roots=array(lightOnly,20,104),states=array(lightOnly,28,12);
 assert(roots.count==1&&states.count==1&&u32(lightOnly,12)==0);
 assert(std::equal(nativeLight.begin()+roots.at,nativeLight.begin()+roots.at+104,lightOnly.begin()+roots.at));
 for(size_t i=0;i<nativeLight.size();++i)if(i!=24&&i!=32)assert(nativeLight[i]==lightOnly[i]);
 auto value=[](const Bytes& b,size_t at){float v=0;std::memcpy(&v,b.data()+at,4);return v;};
 const auto overrideData=visual_test::makeGoldLightOverride(nativeLight);
 assert(overrideData.size()==68&&u32(overrideData,0)==0);
 assert(value(overrideData,8)==187.5f&&value(overrideData,56)==100.f);
 for(size_t i=0;i<64;++i)if(!(i>=4&&i<8)&&!(i>=52&&i<56))assert(overrideData[4+i]==nativeLight[116+i]);
 assert(visual_test::makeLightOnly(nativeLight)==lightOnly);
 const auto params=array(nativeLight,108,12);
 auto rejects=[](const Bytes& data){try{visual_test::makeGoldLightOverride(data);return false;}catch(const std::runtime_error&){return true;}};
 auto missing=nativeLight;put32(missing,params.at+12,0);assert(rejects(missing));
 auto wrongType=nativeLight;put32(wrongType,params.at+16,0);assert(rejects(wrongType));
 auto badRange=nativeLight;badRange[params.at+20]=63;assert(rejects(badRange));
 auto wrongValue=nativeLight;put32(wrongValue,120,0);assert(rejects(wrongValue));
 auto duplicate=nativeLight;std::memcpy(duplicate.data()+params.at,duplicate.data()+params.at+12,12);assert(rejects(duplicate));
 uint32_t overrideType=2166136261u;for(char c:std::string("keen::ecs::VfxParametersOverwriteComponent"))overrideType=(overrideType^static_cast<unsigned char>(c))*16777619u;
 unsigned overrides=0;
 assert(components.count==11);
 for(unsigned j=0;j<components.count;++j){const auto row=components.at+j*12;
  if(u32(goldVisual,row)==overrideType){const auto data=array(goldVisual,row+4,1);assert(data.count==68);
   assert(std::equal(overrideData.begin(),overrideData.end(),goldVisual.begin()+data.at));++overrides;}}
 assert(overrides==1);
 for(unsigned j=0;j<parts.count;++j)assert(u32(visual,parts.at+j*12)!=overrideType);
 bool noSource=false;try{visual_test::makeVisual(donor,true);}catch(const std::runtime_error&){noSource=true;}assert(noSource);
 // The gold visual alone points at the gold VFX, with no flame root added.
 bool goldBinding=false;
 for(unsigned j=0;j<components.count;++j){const auto row=components.at+j*12;if(u32(goldVisual,row)==0xb3721934){
  const auto data=array(goldVisual,row+4,1);goldBinding=equal(goldVisual,data.at,GoldLight);
 }}assert(goldBinding);
 auto malformed=nativeLight;put32(malformed,roots.at+16,1);
 bool badLight=false;try{visual_test::makeLightOnly(malformed);}catch(const std::runtime_error&){badLight=true;}assert(badLight);
 bool rejected=false;try{add_item_registry(add_item_registry(items));}catch(const std::runtime_error&){rejected=true;}assert(rejected);
 std::cout<<"PASS: native catalogs, iron/gold recipes with native ingredient identities, both mod orders, duplicate rejection, pelvis attachment, inward strap, isolated native point light without flame root\n";
}

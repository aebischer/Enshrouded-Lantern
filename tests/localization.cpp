// Exercise the production text adapter without loading or patching the game.
#include "../src/native_content.cpp"
#include "../sdk/tests/catalog_checks.h"
namespace lantern::native {
static unsigned forwarded=0;
static uint64_t* __fastcall fallbackLookup(uint64_t* out,void*,uint32_t) {++forwarded;return out;}
static uint64_t* __fastcall fallbackFormat(uint64_t* out,uint32_t) {++forwarded;return out;}
static void runLocalization() {
 checkCatalogue(LanternTexts);
 originalLookup=&fallbackLookup;originalFormat=&fallbackFormat;
 for(const auto lang:enshrouded::i18n::supportedLanguages) {
  unsigned char pool[0xe0]{};write<uint32_t>(pool,0xd8,lang);
  for(const auto id:{hash(Name),hash(Description),hash(Caption),hash(GoldName),hash(GoldDescription)}) {
   uint64_t out[3]{};
   const auto expected=text(id,lang);assert(expected&&*expected);
   assert(lookup(out,pool,id)==out);
   assert(out[1]==reinterpret_cast<uint64_t>(expected)&&out[2]==std::strlen(expected));
   assert(format(out,id)==out);
   assert(out[0]==reinterpret_cast<uint64_t>(expected)&&out[1]==std::strlen(expected));
  }
  assert(equipmentTitle()==enshrouded::i18n::find(LanternTexts,"equipment.title",lang));
 }
 assert(!text(0,5));uint64_t out[3]{};lookup(out,nullptr,0);format(out,0);assert(forwarded==2);
 assert(text(hash(Name),999)==text(hash(Name),0));
 assert(text(hash(Name),22)==text(hash(Name),5));
 std::puts("PASS: 15 complete languages, both lantern resources, native lookup/format, equipment title, locale changes, fallback and native passthrough.");
}
}
int main(){lantern::native::runLocalization();}

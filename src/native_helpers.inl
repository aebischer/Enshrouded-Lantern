// UTF-8 follows the native signed-byte hash.
uint64_t nativeTextHash(const char* text) {
 uint64_t value=0xcbf29ce484222325ull;
 for(auto p=text;*p;++p)value=(value^static_cast<uint64_t>(static_cast<int64_t>(static_cast<int8_t>(*p))))*0x100000001b3ull;
 return value;
}

#pragma once
namespace lantern::native {
bool initialize(void(__cdecl*)(const char*));
const void* itemDescriptor(unsigned id=0) noexcept;
const void* equipmentDescriptor(unsigned id=0) noexcept;
const char* equipmentTitle() noexcept;
unsigned itemId() noexcept;
unsigned goldItemId() noexcept;
bool isLantern(unsigned id) noexcept;
}

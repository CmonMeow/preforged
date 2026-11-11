#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <stdexcept>
namespace capture {
struct ChatCommandHook {std::uintptr_t target,dispatcher;};
// Pinned GWCA4.8.5: enable only the outgoing chat target, then register its
// existing command parser for UI event3000001b. Do not enable ChatModule's
// other detours (rendering, timestamps, links, global chat UI).
inline ChatCommandHook chat_command_hook(HMODULE module){
    if(!module)throw std::runtime_error("GWCA module missing");
    const auto base=reinterpret_cast<std::uintptr_t>(module);
    const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE)throw std::runtime_error("invalid GWCA image");
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.TimeDateStamp!=0x6ac209c6||nt->OptionalHeader.SizeOfImage!=0x5a000)
        throw std::runtime_error("unsupported GWCA chat-hook image");
    constexpr unsigned char detour[]{0x55,0x8b,0xec,0x51,0x51,0xe8,0xb9,0x7c,0,0,0x8b,0x45,0x08,0x89,0x45,0xf8,
        0x8b,0x45,0x0c,0x89,0x45,0xfc,0x8d,0x45,0xf8,0x6a,0,0x50,0x68,0x1b,0,0,0x30};
    constexpr unsigned char parser[]{0x51,0x51,0x57,0x8b,0x7c,0x24,0x10,0x80,0x3f,0,0x0f,0x85,0xd6,0x01,0,0};
    if(std::memcmp(reinterpret_cast<const void*>(base+0x42c9),detour,sizeof detour)||
       std::memcmp(reinterpret_cast<const void*>(base+0x4899),parser,sizeof parser))
        throw std::runtime_error("GWCA chat dispatcher signature mismatch");
    const auto init=reinterpret_cast<const unsigned char*>(base+0x4d5a);
    std::uint32_t slot=0,hook=0,trampoline=0;
    std::memcpy(&slot,init+2,4);std::memcpy(&trampoline,init+9,4);std::memcpy(&hook,init+14,4);
    if(init[0]!=0x39||init[1]!=0x2d||init[8]!=0x68||init[13]!=0xba||
       slot!=base+0x53b40||trampoline!=base+0x53b44||hook!=base+0x42c9)
        throw std::runtime_error("GWCA chat target binding mismatch");
    const auto target=*reinterpret_cast<const std::uint32_t*>(slot);
    const auto original=*reinterpret_cast<const std::uint32_t*>(trampoline);
    MEMORY_BASIC_INFORMATION region{};
    if(!target||!original||!VirtualQuery(reinterpret_cast<void*>(target),&region,sizeof region)||region.State!=MEM_COMMIT||
       !(region.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))
        throw std::runtime_error("GWCA outgoing chat target not initialized/executable");
    return {target,base+0x4899};
}
}

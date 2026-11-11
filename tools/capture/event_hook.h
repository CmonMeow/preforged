#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <stdexcept>
namespace capture {
// GWCA 4.8.5 distributed at ef3d3670: EventMgrModule.enable_hooks is
// RVA 6d72. It enables the target stored by `mov ecx,[absolute]` at entry.
// GameThread::EnableHooks registers event 2 and does not enable this detour.
inline std::uintptr_t event_dispatch_target(HMODULE module){
    if(!module)throw std::runtime_error("GWCA module missing");
    const auto base=reinterpret_cast<std::uintptr_t>(module);
    auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE)throw std::runtime_error("invalid GWCA image");
    auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.TimeDateStamp!=0x6ac209c6||nt->OptionalHeader.SizeOfImage!=0x5a000)
        throw std::runtime_error("unsupported GWCA event-hook image");
    const auto code=reinterpret_cast<const unsigned char*>(base+0x6d72);
    if(code[0]!=0x8b||code[1]!=0x0d||code[6]!=0x85||code[7]!=0xc9)
        throw std::runtime_error("GWCA event-hook signature mismatch");
    std::uint32_t slot=0;std::memcpy(&slot,code+2,4);
    if(slot<base||slot>base+nt->OptionalHeader.SizeOfImage-4)
        throw std::runtime_error("GWCA event target slot outside image");
    const auto target=*reinterpret_cast<const std::uint32_t*>(slot);
    if(!target)throw std::runtime_error("GWCA event target not initialized");
    MEMORY_BASIC_INFORMATION region{};
    if(!VirtualQuery(reinterpret_cast<void*>(target),&region,sizeof region)||region.State!=MEM_COMMIT||
       !(region.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))
        throw std::runtime_error("GWCA event target not executable");
    return target;
}
}

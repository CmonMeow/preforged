#pragma once
#include <windows.h>
#include <GWCA/Utilities/Hooker.h>
#include <cstdint>
#include <cstring>
#include <stdexcept>
namespace capture {
inline void disable_orphan_chat_render(std::uintptr_t library,std::uintptr_t game,std::uint32_t game_image_size,void(__cdecl* disable)(GW::Hook::HookTarget)){
 if(!disable)throw std::runtime_error("chat render cleanup API unavailable");
 constexpr unsigned char creation[]{0x68};
 const auto* bind=reinterpret_cast<const unsigned char*>(library+0x59f6);
 std::uint32_t trampoline=0,detour=0,slot=0;
 std::memcpy(&trampoline,bind+1,4);std::memcpy(&detour,bind+6,4);std::memcpy(&slot,bind+11,4);
 if(bind[0]!=creation[0]||bind[5]!=0x68||bind[10]!=0x68||bind[15]!=0xe8||
    trampoline!=library+0x53b68||detour!=library+0x43f7||slot!=library+0x53b64)throw std::runtime_error("unsupported chat render hook binding");
 const auto address=*reinterpret_cast<const std::uint32_t*>(slot);
 const auto original=*reinterpret_cast<const std::uint32_t*>(trampoline);
 if(!address)return; // Native initializer did not create this hook.
 if(game_image_size<8||address<game||address>game+game_image_size-8||!original)throw std::runtime_error("unsupported chat render target");
 constexpr unsigned char prologue[]{0x55,0x8b,0xec,0x83,0xec,0x30,0x53,0x8b};
 if(std::memcmp(reinterpret_cast<const void*>(original),prologue,6))throw std::runtime_error("unsupported chat render original trampoline");
 const auto* bytes=reinterpret_cast<const unsigned char*>(address);
 if(bytes[0]==0xe9){std::int32_t relative=0;std::memcpy(&relative,bytes+1,4);
  if(address+5+relative!=detour)throw std::runtime_error("foreign chat render detour");
  disable(GW::Hook::HookTarget(reinterpret_cast<void*>(address)));
 }
 if(std::memcmp(reinterpret_cast<const void*>(address),prologue,sizeof prologue))throw std::runtime_error("chat render detour cleanup failed");
}
}

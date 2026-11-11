#pragma once
#include <windows.h>
#include <intrin.h>
#include <GWCA/Utilities/Hooker.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <atomic>
#include <cstring>
#include <stdexcept>
#include <array>
#include <utility>
#include "chat_render_cleanup.h"
namespace capture::tls_guard {
inline void* target=nullptr;
inline void(__cdecl* original)()=nullptr;
inline void* bootstrap_target=nullptr;
inline void(__cdecl* bootstrap_original)()=nullptr;
inline void* stoc_initializer_target=nullptr;
inline void(__cdecl* stoc_initializer_original)()=nullptr;
inline std::atomic<unsigned> skipped_stoc_initializers{0};
inline uintptr_t library=0,game=0,game_tls_index=0;
inline uint32_t game_image_size=0;
inline std::atomic<unsigned> deferred{0},accepted{0};
inline std::atomic<uintptr_t> validated_context{0};
inline std::atomic<bool> inert{false};
inline std::atomic<bool> bootstrap_queued{false};
inline std::atomic<unsigned> bootstrap_retries{0};
inline bool readable(uintptr_t p,size_t n){MEMORY_BASIC_INFORMATION r{};return p&&VirtualQuery(reinterpret_cast<void*>(p),&r,sizeof r)&&r.State==MEM_COMMIT&&!(r.Protect&(PAGE_NOACCESS|PAGE_GUARD))&&p+n>=p&&p+n<=reinterpret_cast<uintptr_t>(r.BaseAddress)+r.RegionSize;}
inline uintptr_t from_tls(uintptr_t vector,uint32_t index,uint32_t scanned_index){
 if(!vector||index>1023||index!=scanned_index||!readable(vector+index*4,4))return 0;
 const auto block=*reinterpret_cast<const uintptr_t*>(vector+index*4);if(!readable(block,12))return 0;
 const auto value=*reinterpret_cast<const uintptr_t*>(block+8);return readable(value,0x48)?value:0;
}
inline uintptr_t context(){
 const auto vector=uintptr_t(__readfsdword(0x2c));if(!readable(game_tls_index,4)||!readable(library+0x53db8,4))return 0;
 return from_tls(vector,*reinterpret_cast<const uint32_t*>(game_tls_index),*reinterpret_cast<const uint32_t*>(library+0x53db8));
}
inline void __cdecl dispatch(){if(inert)return;const auto value=context();if(!value){++deferred;return;}validated_context=value;++accepted;if(original)original();}
inline void __cdecl bootstrap_dispatch(){
 if(inert)return;
 const auto value=context();
 if(!value){
  ++deferred;
  // Enqueue(false) has an inline path governed by dispatcher-active state.
  // Its recursive critical section does not establish current gameTLS.
  // Force queueing, so this bootstrap cannot escape the TLS check.
  if(!bootstrap_queued.exchange(true)){
   ++bootstrap_retries;
   GW::GameThread::Enqueue([]{bootstrap_queued=false;bootstrap_dispatch();},true);
  }
  return;
 }
 validated_context=value;++accepted;if(bootstrap_original)bootstrap_original();
}
inline bool detoured(void* entry,void* destination){
 const auto address=reinterpret_cast<uintptr_t>(entry);
 if(!readable(address,5)||*reinterpret_cast<const unsigned char*>(address)!=0xe9)return false;
 int32_t relative=0;std::memcpy(&relative,reinterpret_cast<void*>(address+1),4);
 return address+5+relative==reinterpret_cast<uintptr_t>(destination);
}
// Snapshot capture never subscribes to StoC or touches its decoder tables.
// The pinned initializer enables its decoder hook before Initialize returns;
// that hook asserts an obsolete487-entry table. Skip the unused initializer,
// not its fatal assertion or the native game decoder.
inline void __cdecl skip_stoc_initializer(){++skipped_stoc_initializers;}
inline bool stoc_signature(uintptr_t base){
 if(!readable(base+0x51cbc,24)||!readable(base+0x41170,12)||!readable(base+0x143d6,18))return false;
 const uintptr_t descriptor[]{base+0x41170,0,base+0x143d6,base+0x1441f,base+0x1423d,base+0x142c3};
 if(std::memcmp(reinterpret_cast<void*>(base+0x51cbc),descriptor,sizeof descriptor)||std::memcmp(reinterpret_cast<void*>(base+0x41170),"StoCModule",11))return false;
 const auto code=reinterpret_cast<const unsigned char*>(base+0x143d6);uint32_t section=0,initialize=0;
 std::memcpy(&section,code+1,4);std::memcpy(&initialize,code+7,4);
 return code[0]==0x68&&section==base+0x53ea4&&code[5]==0xff&&code[6]==0x15&&initialize==base+0x37038&&code[11]==0x68&&code[12]==0xff&&code[13]==0x0f&&code[14]==0&&code[15]==0;
}
inline bool stoc_snapshot_safe(){return skipped_stoc_initializers==1&&detoured(stoc_initializer_target,reinterpret_cast<void*>(&skip_stoc_initializer))&&readable(library+0x53ed4,8)&&!*reinterpret_cast<const uintptr_t*>(library+0x53ed4)&&!*reinterpret_cast<const uintptr_t*>(library+0x53ed8);}
// Full GWCA initialization also enables unrelated renderer/network modules.
// Snapshot capture retains only audited API dependencies. Descriptor changes
// affect this fresh private GWCA instance, never the native game module.
struct Module {uint32_t descriptor,name,init,exit,enable,disable;bool retain;};
inline constexpr std::array<Module,20> modules{{
 {0x51b20,0x3f47c,0x197f,0x1c06,0x1b31,0x1bc8,true},
 {0x51b38,0x3fce0,0x367a,0x3731,0x3745,0x3766,false},
 {0x51b50,0x40000,0x4b6b,0x509a,0x4efd,0x5005,true},
 {0x51b68,0x40168,0x678d,0x6801,0,0,false},
 {0x51b80,0x401b8,0x6d3e,0x6d9e,0x6d72,0x6d88,true},
 {0x51b98,0x405f4,0x9a9c,0x9bad,0x9b12,0x9b8c,false},
 {0x51bb4,0x407b4,0xb45b,0xb49b,0xb4d2,0xb46e,true},
 {0x51bcc,0x40800,0,0,0,0,false},
 {0x51be4,0x409b8,0xc4c1,0xc8f8,0xc7d0,0xc864,true},
 {0x51bfc,0x40b10,0xe05e,0xe161,0xe10b,0xe136,true},
 {0x51c14,0x40c40,0xe834,0xe95c,0xe930,0xe946,true},
 {0x51c2c,0x40cec,0xf3fc,0,0xf3ba,0xf3db,true},
 {0x51c44,0x40da4,0xf8dd,0xf9e3,0xf9b7,0xf9cd,false},
 {0x51c5c,0x40e3c,0x107a9,0,0,0,false},
 {0x51c74,0x40e88,0x10b52,0x10cfb,0x10ca5,0x10cd0,false},
 {0x51c8c,0x40f2c,0x111ce,0x11321,0x1136c,0x112cc,false},
 {0x51ca4,0x410e8,0x127e5,0x12a9a,0x129c6,0x12a30,false},
 {0x51cbc,0x41170,0x143d6,0x1441f,0x1423d,0x142c3,false},
 {0x51cd4,0x41190,0,0,0,0,false},
 {0x51cec,0x4164c,0x15761,0x15abc,0x158b1,0x159f5,true}
}};
inline std::atomic<unsigned> initialization_stage{0},initialization_completed{0};
inline void(*observe_initialization)(const char*,bool,bool)noexcept=nullptr;
inline bool selective_installed=false;
template<size_t Index> void __cdecl module_initialize(){
 const auto& m=modules[Index];initialization_stage=static_cast<unsigned>(Index+1);
 const auto name=reinterpret_cast<const char*>(library+m.name);
 if(observe_initialization)observe_initialization(name,true,m.retain);
 if(m.retain&&m.init)reinterpret_cast<void(__cdecl*)()>(library+m.init)();
 else if(m.descriptor==0x51cbc)skip_stoc_initializer();
 ++initialization_completed;
 if(observe_initialization)observe_initialization(name,false,m.retain);
 initialization_stage=0;
}
template<size_t... I> constexpr auto initialization_wrappers(std::index_sequence<I...>){return std::array<void(__cdecl*)(),sizeof...(I)>{&module_initialize<I>...};}
inline void install_snapshot_modules(){
 if(selective_installed)throw std::runtime_error("snapshot modules already configured");
 for(const auto& m:modules){
  if(!readable(library+m.descriptor,24)||!readable(library+m.name,32))throw std::runtime_error("snapshot module descriptor unreadable");
  const uintptr_t expected[]{library+m.name,m.descriptor==0x51cec?library+0x538a0:0,m.init?library+m.init:0,m.exit?library+m.exit:0,m.enable?library+m.enable:0,m.disable?library+m.disable:0};
  if(std::memcmp(reinterpret_cast<void*>(library+m.descriptor),expected,sizeof expected))throw std::runtime_error("snapshot module descriptor changed; preserve client");
 }
 const auto wrappers=initialization_wrappers(std::make_index_sequence<modules.size()>{});
 DWORD old=0;if(!VirtualProtect(reinterpret_cast<void*>(library+0x51b20),0x1e4,PAGE_READWRITE,&old))throw std::runtime_error("snapshot module configuration protection");
 for(size_t i=0;i<modules.size();++i){const auto& m=modules[i];auto data=reinterpret_cast<uintptr_t*>(library+m.descriptor);data[2]=reinterpret_cast<uintptr_t>(wrappers[i]);if(!m.retain){data[3]=0;data[4]=0;data[5]=0;}}
 DWORD restored=0;if(!VirtualProtect(reinterpret_cast<void*>(library+0x51b20),0x1e4,old,&restored))throw std::runtime_error("snapshot module protection restore");
 selective_installed=true;
}
inline void install(HMODULE dll,HMODULE exe){
 library=reinterpret_cast<uintptr_t>(dll);game=reinterpret_cast<uintptr_t>(exe);if(!library||!game)throw std::runtime_error("TLS guard modules missing");
 const auto pe=[](uintptr_t base){const auto d=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);if(d->e_magic!=IMAGE_DOS_SIGNATURE)throw std::runtime_error("TLS guard DOS signature");const auto n=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+d->e_lfanew);if(n->Signature!=IMAGE_NT_SIGNATURE||n->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386)throw std::runtime_error("TLS guard PE signature");return n;};
 const auto a=pe(library),b=pe(game);if(a->FileHeader.TimeDateStamp!=0x6ac209c6||a->OptionalHeader.SizeOfImage!=0x5a000||b->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||!b->OptionalHeader.SizeOfImage)throw std::runtime_error("unsupported TLS guard images");
 game_image_size=b->OptionalHeader.SizeOfImage;
 const unsigned char prefix[]{0x83,0xec,0x0c,0x80,0x3d};const unsigned char bootstrap[]{0x56,0x64,0x8b,0x35,0x2c,0,0,0,0x85,0xf6,0x74,0x29};
 if(std::memcmp(reinterpret_cast<void*>(library+0xb37a),prefix,sizeof prefix)||std::memcmp(reinterpret_cast<void*>(library+0xac64),bootstrap,sizeof bootstrap))throw std::runtime_error("TLS guard native signature changed");
 uint32_t active=0;std::memcpy(&active,reinterpret_cast<void*>(library+0xb37f),4);if(active!=library+0x53929)throw std::runtime_error("TLS guard dispatcher relocation changed");
 const auto dir=b->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];if(game_image_size<sizeof(IMAGE_TLS_DIRECTORY32)||dir.Size<sizeof(IMAGE_TLS_DIRECTORY32)||!dir.VirtualAddress||dir.VirtualAddress>game_image_size-sizeof(IMAGE_TLS_DIRECTORY32)||!readable(game+dir.VirtualAddress,sizeof(IMAGE_TLS_DIRECTORY32)))throw std::runtime_error("TLS guard game TLS missing");
 const auto tls=reinterpret_cast<const IMAGE_TLS_DIRECTORY32*>(game+dir.VirtualAddress);game_tls_index=tls->AddressOfIndex;
 if(game_tls_index<game||game_tls_index>game+game_image_size-sizeof(uint32_t)||!readable(game_tls_index,sizeof(uint32_t)))throw std::runtime_error("TLS guard game index outside image");
 if(*reinterpret_cast<const unsigned char*>(library+0x538af))throw std::runtime_error("GWCA already initialized; preserve existing consumer");
 if(!stoc_signature(library))throw std::runtime_error("unused StoC initializer signature changed; preserve client");
 GW::Hook::Initialize();stoc_initializer_target=reinterpret_cast<void*>(library+0x143d6);
 if(GW::Hook::CreateHook(&stoc_initializer_target,GW::Hook::HookTarget(&skip_stoc_initializer),reinterpret_cast<void**>(&stoc_initializer_original))!=0||!stoc_initializer_original)throw std::runtime_error("unused StoC initializer guard failed");
 GW::Hook::EnableHooks(GW::Hook::HookTarget(stoc_initializer_target));
 if(!detoured(stoc_initializer_target,reinterpret_cast<void*>(&skip_stoc_initializer)))throw std::runtime_error("unused StoC initializer physical guard missing");
 target=reinterpret_cast<void*>(library+0xb37a);if(GW::Hook::CreateHook(&target,GW::Hook::HookTarget(&dispatch),reinterpret_cast<void**>(&original))!=0||!original)throw std::runtime_error("TLS dispatcher guard hook failed");
 bootstrap_target=reinterpret_cast<void*>(library+0xac64);
 if(GW::Hook::CreateHook(&bootstrap_target,GW::Hook::HookTarget(&bootstrap_dispatch),reinterpret_cast<void**>(&bootstrap_original))!=0||!bootstrap_original)throw std::runtime_error("TLS bootstrap guard hook failed");
 GW::Hook::EnableHooks(GW::Hook::HookTarget(target));GW::Hook::EnableHooks(GW::Hook::HookTarget(bootstrap_target));
 if(!detoured(target,reinterpret_cast<void*>(&dispatch))||!detoured(bootstrap_target,reinterpret_cast<void*>(&bootstrap_dispatch)))throw std::runtime_error("TLS guard physical detour missing");
}
inline bool bootstrapped(){return accepted.load()&&validated_context.load()&&readable(library+0x53be8,4)&&*reinterpret_cast<const uintptr_t*>(library+0x53be8)==validated_context.load();}
inline void restore_chat_render_patch(){
 // Chat's render detour can toggle this patch during Initialize's EnableAll
 // window. Module Disable removes the detour, but does not undo this patch.
 // Use its original thiscall Toggle(false), never write client code ourselves.
 const auto object=library+0x53b08;
 constexpr unsigned char toggle[]{0x83,0x39,0x00,0x75,0x04,0x32,0xc0,0xeb,0x11};
 if(!readable(object,17)||std::memcmp(reinterpret_cast<const void*>(library+0xf0fc),toggle,sizeof toggle))throw std::runtime_error("unsupported GWCA chat patch toggle");
 const auto destination=*reinterpret_cast<const uintptr_t*>(object);
 const auto replacement=*reinterpret_cast<const uintptr_t*>(object+4);
 const auto saved=*reinterpret_cast<const uintptr_t*>(object+8);
 const auto length=*reinterpret_cast<const uint32_t*>(object+12);
 constexpr unsigned char normal[]{0x75,0x0c}, patched[]{0x90,0x90};
 if(game_image_size<2||destination<game||destination>game+game_image_size-2||length!=2||!readable(saved,2)||!readable(replacement,2)||!readable(destination,2)||
    std::memcmp(reinterpret_cast<const void*>(saved),normal,2)||std::memcmp(reinterpret_cast<const void*>(replacement),patched,2)||
    (std::memcmp(reinterpret_cast<const void*>(destination),normal,2)&&std::memcmp(reinterpret_cast<const void*>(destination),patched,2)))throw std::runtime_error("unsupported GWCA chat render patch binding");
 reinterpret_cast<bool(__thiscall*)(void*,bool)>(library+0xf0fc)(reinterpret_cast<void*>(object),false);
 if(*reinterpret_cast<const unsigned char*>(object+16)||std::memcmp(reinterpret_cast<const void*>(destination),normal,2))throw std::runtime_error("GWCA chat render patch restore failed");
}
inline void disable_observation_modules(){
 capture::disable_orphan_chat_render(library,game,game_image_size,&GW::Hook::DisableHooks);
 // Exact GW::DisableHooks af44 module loop, deliberately omit its initial
 // DisableHooks(NULL). Disable-all would briefly expose queued bootstrap.
 const auto first=*reinterpret_cast<const uintptr_t*>(library+0x53bcc);
 const auto last=*reinterpret_cast<const uintptr_t*>(library+0x53bd0);
 if(last<first||(last-first)%4||(last-first)/4>32||(first!=last&&!readable(first,last-first)))throw std::runtime_error("invalid GWCA initialized module table");
 for(auto row=first;row<last;row+=4){
  const auto descriptor=*reinterpret_cast<const uintptr_t*>(row);
  if(descriptor<library||descriptor>library+0x5a000-24)throw std::runtime_error("invalid GWCA module descriptor");
  if(descriptor==library+0x51cbc)continue; // This unused module was never initialized.
  const auto callback=*reinterpret_cast<const uintptr_t*>(descriptor+0x14);
  if(callback){if(callback<library||callback>=library+0x5a000)throw std::runtime_error("invalid GWCA module disable callback");reinterpret_cast<void(__cdecl*)()>(callback)();}
 }
 restore_chat_render_patch();
 if(!detoured(target,reinterpret_cast<void*>(&dispatch))||!detoured(bootstrap_target,reinterpret_cast<void*>(&bootstrap_dispatch)))throw std::runtime_error("TLS guard lost during module disable");
}
inline void enable(){GW::Hook::EnableHooks(GW::Hook::HookTarget(target));GW::Hook::EnableHooks(GW::Hook::HookTarget(bootstrap_target));if(!detoured(target,reinterpret_cast<void*>(&dispatch))||!detoured(bootstrap_target,reinterpret_cast<void*>(&bootstrap_dispatch)))throw std::runtime_error("TLS guard physical detour missing");}
}

#include "../tools/capture/tls_dispatch_guard.h"
#include <array>
#include <iostream>
int main()try{
 using namespace capture::tls_guard;std::array<uint32_t,4> vector{};std::array<uint32_t,3> block{};std::array<uint32_t,18> owned{};
 vector[3]=reinterpret_cast<uintptr_t>(block.data());block[2]=reinterpret_cast<uintptr_t>(owned.data());
 const auto check=[](bool b){if(!b)throw std::runtime_error("TLS guard regression failed");};
 check(!from_tls(0,3,3));check(!from_tls(reinterpret_cast<uintptr_t>(vector.data()),3,2));check(!from_tls(reinterpret_cast<uintptr_t>(vector.data()),1024,1024));
 check(from_tls(reinterpret_cast<uintptr_t>(vector.data()),3,3)==reinterpret_cast<uintptr_t>(owned.data()));
 block[2]=0;check(!from_tls(reinterpret_cast<uintptr_t>(vector.data()),3,3));block[2]=1;check(!from_tls(reinterpret_cast<uintptr_t>(vector.data()),3,3));
 vector[3]=0;check(!from_tls(reinterpret_cast<uintptr_t>(vector.data()),3,3));
 library=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"gwca.dll"));check(library!=0);game=0;
 check(stoc_signature(library)&&!stoc_signature(library+4));
 GW::Hook::Initialize();stoc_initializer_target=reinterpret_cast<void*>(library+0x143d6);
 check(GW::Hook::CreateHook(&stoc_initializer_target,GW::Hook::HookTarget(&skip_stoc_initializer),reinterpret_cast<void**>(&stoc_initializer_original))==0);
 GW::Hook::EnableHooks(GW::Hook::HookTarget(stoc_initializer_target));check(detoured(stoc_initializer_target,reinterpret_cast<void*>(&skip_stoc_initializer)));
 reinterpret_cast<void(__cdecl*)()>(stoc_initializer_target)();check(skipped_stoc_initializers==1&&stoc_snapshot_safe());
 check(*reinterpret_cast<uintptr_t*>(library+0x53ed4)==0&&*reinterpret_cast<uintptr_t*>(library+0x53ed8)==0);
 bootstrap_target=reinterpret_cast<void*>(library+0xac64);
 target=reinterpret_cast<void*>(library+0xb37a);
 check(GW::Hook::CreateHook(&target,GW::Hook::HookTarget(&dispatch),reinterpret_cast<void**>(&original))==0);
 GW::Hook::EnableHooks(GW::Hook::HookTarget(target));check(detoured(target,reinterpret_cast<void*>(&dispatch)));
 check(GW::Hook::CreateHook(&bootstrap_target,GW::Hook::HookTarget(&bootstrap_dispatch),reinterpret_cast<void**>(&bootstrap_original))==0);
 GW::Hook::EnableHooks(GW::Hook::HookTarget(bootstrap_target));check(detoured(bootstrap_target,reinterpret_cast<void*>(&bootstrap_dispatch)));
 // Initialize the real native Patch object against a private executable page.
 // No client process/image is loaded. Toggle uses GWCA's original implementation.
 auto page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));check(page!=nullptr);
 page[0]=0x75;page[1]=0x0c;const unsigned char nop[]{0x90,0x90};
 game=reinterpret_cast<uintptr_t>(page)-0x1401fc;
 reinterpret_cast<void(__thiscall*)(void*,void*,const void*,size_t)>(library+0xef74)(reinterpret_cast<void*>(library+0x53b08),page,nop,2);
 *reinterpret_cast<unsigned char*>(library+0x51d17)=1;
 reinterpret_cast<bool(__thiscall*)(void*,bool)>(library+0xf0fc)(reinterpret_cast<void*>(library+0x53b08),true);
 check(page[0]==0x90&&page[1]==0x90);
 game+=0xf49000;bool denied=false;try{restore_chat_render_patch();}catch(const std::runtime_error&){denied=true;}check(denied&&page[0]==0x90);game-=0xf49000;
 // Real print-hook creation can survive native ChatModule Disable, which
 // omits slot53b64. Explicit cleanup must disable the owned detour alone.
 auto render=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));check(render!=nullptr);
 constexpr unsigned char print_prologue[]{0x55,0x8b,0xec,0x83,0xec,0x30,0x53,0x8b};std::memcpy(render,print_prologue,8);
 *reinterpret_cast<uintptr_t*>(library+0x53b64)=reinterpret_cast<uintptr_t>(render);
 reinterpret_cast<void(__cdecl*)()>(library+0x59f6)();
 GW::Hook::EnableHooks(GW::Hook::HookTarget(render));check(detoured(render,reinterpret_cast<void*>(library+0x43f7)));
 reinterpret_cast<void(__cdecl*)()>(library+0x5005)();check(detoured(render,reinterpret_cast<void*>(library+0x43f7))); // native omission reproduced
 const auto render_game=reinterpret_cast<uintptr_t>(render)-0x13ef40;
 bool wrong_render=false;try{capture::disable_orphan_chat_render(library,reinterpret_cast<uintptr_t>(render)+4096,&GW::Hook::DisableHooks);}catch(const std::runtime_error&){wrong_render=true;}check(wrong_render&&detoured(render,reinterpret_cast<void*>(library+0x43f7)));
 capture::disable_orphan_chat_render(library,render_game,&GW::Hook::DisableHooks);check(!std::memcmp(render,print_prologue,8));
 *reinterpret_cast<uintptr_t*>(library+0x53b64)=0; // fixture no longer presents a native game target
 disable_observation_modules();check(page[0]==0x75&&page[1]==0x0c);
 check(!*reinterpret_cast<unsigned char*>(library+0x53b08+16));
 check(detoured(target,reinterpret_cast<void*>(&dispatch))&&detoured(bootstrap_target,reinterpret_cast<void*>(&bootstrap_dispatch)));
 game=0;
 // Exercise an active-dispatch inline branch with deliberately absent gameTLS.
 // Production callers share a critical section; this does not prove a race.
 InitializeCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(library+0x53bec));
 *reinterpret_cast<unsigned char*>(library+0x53929)=1;
 *reinterpret_cast<unsigned char*>(library+0x5392a)=1;
 reinterpret_cast<void(__cdecl*)()>(bootstrap_target)();
 check(deferred==1&&bootstrap_retries==1&&bootstrap_queued&&validated_context==0);
 reinterpret_cast<void(__cdecl*)()>(bootstrap_target)();check(deferred==2&&bootstrap_retries==1);
 install_snapshot_modules();check(selective_installed);
 for(const auto& m:modules){const auto descriptor=reinterpret_cast<const uintptr_t*>(library+m.descriptor);check(descriptor[2]!=0);if(!m.retain)check(!descriptor[3]&&!descriptor[4]&&!descriptor[5]);else{check(descriptor[4]==(m.enable?library+m.enable:0));}}
 const auto before_modules=initialization_completed.load();
 reinterpret_cast<void(__cdecl*)()>(*reinterpret_cast<uintptr_t*>(library+0x51b38+8))();
 check(initialization_completed==before_modules+1&&initialization_stage==0); // Camera init skipped, no native game required.
 bool repeated_denied=false;try{install_snapshot_modules();}catch(const std::runtime_error&){repeated_denied=true;}check(repeated_denied); inert=true;*reinterpret_cast<unsigned char*>(library+0x53929)=0;*reinterpret_cast<unsigned char*>(library+0x5392a)=0;
 GW::Hook::DisableHooks(GW::Hook::HookTarget(stoc_initializer_target));
 GW::Hook::DisableHooks(GW::Hook::HookTarget(target));
 GW::Hook::DisableHooks(GW::Hook::HookTarget(bootstrap_target));
 std::cout<<"PASS context validation, actual GWCA detour and inline-bootstrap forced queue/no duplicate\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}



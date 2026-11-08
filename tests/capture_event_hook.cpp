#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../tools/capture/event_hook.h"
#include <iostream>
void executable_target(){}
int main()try{
 auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x5a000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!memory)return 2;
 struct Cleanup{void*p;~Cleanup(){VirtualFree(p,0,MEM_RELEASE);}}cleanup{memory};
 auto module=reinterpret_cast<HMODULE>(memory);
 auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(memory);dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x100;
 auto nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(memory+0x100);nt->Signature=IMAGE_NT_SIGNATURE;
 nt->FileHeader.TimeDateStamp=0x6ac209c6;nt->OptionalHeader.SizeOfImage=0x5a000;
 auto code=memory+0x6d72;code[0]=0x8b;code[1]=0x0d;code[6]=0x85;code[7]=0xc9;
 auto slot=reinterpret_cast<std::uint32_t*>(memory+0x53000);
 auto address=reinterpret_cast<std::uint32_t>(slot);std::memcpy(code+2,&address,4);
 *slot=reinterpret_cast<std::uint32_t>(&executable_target);
 if(capture::event_dispatch_target(module)!=*slot)return 3;
 auto rejects=[&](){try{capture::event_dispatch_target(module);return false;}catch(const std::runtime_error&){return true;}};
 nt->FileHeader.TimeDateStamp++;if(!rejects())return 4;nt->FileHeader.TimeDateStamp--;
 code[0]=0x90;if(!rejects())return 5;code[0]=0x8b;
 *slot=0;if(!rejects())return 6;
 *slot=reinterpret_cast<std::uint32_t>(memory);if(!rejects())return 7;
 address=1;std::memcpy(code+2,&address,4);if(!rejects())return 8;
 std::cout<<"PASS hook target, mismatched image, signature, null target, non-executable target and out-of-image slot\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

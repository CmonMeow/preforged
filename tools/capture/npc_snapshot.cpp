// Read-only developer capture. Layout provenance: supplied GWCA AgentMgr.cpp,
// Agent.h and Array.h. No injection, process writes, credentials or chat reads.
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
struct Handle { HANDLE h; ~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);} };
void read(HANDLE h,std::uintptr_t address,void* out,std::size_t n){
    SIZE_T actual=0;if(!ReadProcessMemory(h,reinterpret_cast<void*>(address),out,n,&actual)||actual!=n)
        throw std::runtime_error("client memory unavailable; no complete snapshot produced");
}
template<class T>T read(HANDLE h,std::uintptr_t address){T value{};read(h,address,&value,sizeof value);return value;}
struct Array { std::uint32_t buffer,capacity,size,param; };
struct Npc {std::uint32_t id,model,plane;float x,y,z,facing;};
template<class T>T field(const std::array<unsigned char,0x100>& b,std::size_t offset){T value;std::memcpy(&value,b.data()+offset,sizeof value);return value;}
int main(int argc,char**argv)try{
    if(argc!=4||std::string(argv[2])!="--legacy-layout"){
        std::cerr<<"Usage: npc_snapshot PID --legacy-layout OUTPUT.json\n"
            <<"Developer tool: supplied pre-Reforged GWCA layout only. Not validated for Reforged.\n";return 2;
    }
    auto pid=std::stoul(argv[1]);Handle process{OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid)};
    if(!process.h)throw std::runtime_error("cannot open client for read-only capture");
    Handle modules{CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid)};
    MODULEENTRY32W mod{};mod.dwSize=sizeof mod;
    if(!Module32FirstW(modules.h,&mod))throw std::runtime_error("cannot enumerate client module");
    if(_wcsicmp(mod.szModule,L"Gw.exe"))throw std::runtime_error("target main module is not Gw.exe");
    auto base=reinterpret_cast<std::uintptr_t>(mod.modBaseAddr);
    const auto dos=read<IMAGE_DOS_HEADER>(process.h,base);
    const auto nt=read<IMAGE_NT_HEADERS32>(process.h,base+dos.e_lfanew);
    if(dos.e_magic!=IMAGE_DOS_SIGNATURE||nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_I386)
        throw std::runtime_error("expected 32-bit GW image");
    constexpr unsigned char signature[]={0xff,0x50,0x10,0x47,0x83,0xc6,0x04,0x3b,0xfb,0x75,0xe1};
    std::vector<std::uint32_t> candidates;
    auto section_base=base+dos.e_lfanew+4+sizeof(IMAGE_FILE_HEADER)+nt.FileHeader.SizeOfOptionalHeader;
    for(unsigned i=0;i<nt.FileHeader.NumberOfSections;++i){
        auto section=read<IMAGE_SECTION_HEADER>(process.h,section_base+i*sizeof(IMAGE_SECTION_HEADER));
        if(!(section.Characteristics&IMAGE_SCN_MEM_EXECUTE))continue;
        if(section.Misc.VirtualSize>64*1024*1024)throw std::runtime_error("invalid image section");
        std::vector<unsigned char> bytes(section.Misc.VirtualSize);read(process.h,base+section.VirtualAddress,bytes.data(),bytes.size());
        for(std::size_t at=0;at+17<=bytes.size();++at)if(!std::memcmp(bytes.data()+at,signature,sizeof signature)){
            std::uint32_t pointer;std::memcpy(&pointer,bytes.data()+at+13,4);candidates.push_back(pointer);
        }
    }
    if(candidates.size()!=1)throw std::runtime_error("agent-array signature missing or ambiguous; unsupported client build");
    auto array=read<Array>(process.h,candidates[0]);
    std::cerr<<"array capacity="<<array.capacity<<" size="<<array.size<<" buffer_present="<<(array.buffer!=0)<<"\n";
    if(!array.buffer||array.size>array.capacity||array.capacity>65536)throw std::runtime_error("invalid agent array layout");
    std::vector<std::uint32_t> pointers(array.size);read(process.h,array.buffer,pointers.data(),pointers.size()*4);
    std::vector<Npc> npcs;unsigned living=0;
    for(unsigned index=0;index<pointers.size();++index){
        if(!pointers[index])continue;
        std::array<unsigned char,0x100> b{};read(process.h,pointers[index],b.data(),b.size());
        if(field<std::uint32_t>(b,0x2c)!=index)throw std::runtime_error("agent identity mismatch; snapshot rejected");
        auto type=field<std::uint32_t>(b,0x9c);if(!(type&1))continue;
        ++living;auto model=field<std::uint32_t>(b,0xf4);
        if((model&0xffff0000)!=0x20000000)continue; // NPCs only; exclude player records.
        Npc n{index,model,field<std::uint32_t>(b,0x7c),field<float>(b,0x74),field<float>(b,0x78),field<float>(b,0x30),field<float>(b,0x4c)};
        if(!std::isfinite(n.x)||!std::isfinite(n.y)||!std::isfinite(n.z)||!std::isfinite(n.facing))throw std::runtime_error("invalid coordinates");
        npcs.push_back(n);
    }
    auto after=read<Array>(process.h,candidates[0]);
    std::vector<std::uint32_t> recheck(array.size);read(process.h,array.buffer,recheck.data(),recheck.size()*4);
    if(std::memcmp(&array,&after,sizeof array)||recheck!=pointers)throw std::runtime_error("agent array changed during snapshot; retry");
    // Open only after all validation. Never replace an existing capture.
    Handle output{CreateFileA(argv[3],GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr)};
    if(output.h==INVALID_HANDLE_VALUE)throw std::runtime_error("output exists or cannot be created");
    std::string json="{\"schema\":1,\"layout\":\"gwca-legacy\",\"pe_timestamp\":"+std::to_string(nt.FileHeader.TimeDateStamp)+",\"image_size\":"+std::to_string(nt.OptionalHeader.SizeOfImage)+",\"observed_positions_not_spawns\":true,\"npcs\":[";
    for(std::size_t i=0;i<npcs.size();++i){auto n=npcs[i];if(i)json+=",";json+="{\"agent\":"+std::to_string(n.id)+",\"model\":"+std::to_string(n.model)+",\"plane\":"+std::to_string(n.plane)+",\"x\":"+std::to_string(n.x)+",\"y\":"+std::to_string(n.y)+",\"z\":"+std::to_string(n.z)+",\"facing\":"+std::to_string(n.facing)+"}";}
    json+="]}\n";DWORD written=0;if(!WriteFile(output.h,json.data(),DWORD(json.size()),&written,nullptr)||written!=json.size())throw std::runtime_error("capture write failed");
    std::cout<<"Captured "<<npcs.size()<<" NPC observations; examined "<<living<<" living agents.\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}



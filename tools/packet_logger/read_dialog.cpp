#include <windows.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

// Read-only retrieval of guild-hall dialog text already decoded by the client.
// No injection, hooks, packets, or memory writes.
int wmain(int argc,wchar_t** argv){
 if(argc!=3){std::cerr<<"Usage: read_dialog PID output.txt\n";return 2;}
 DWORD pid=std::stoul(argv[1]);HANDLE process=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,pid);
 if(!process){std::cerr<<"Cannot open client\n";return 1;}
 wchar_t path[32768];DWORD length=32768;
 if(!QueryFullProcessImageNameW(process,0,path,&length)||_wcsicmp(std::filesystem::path(path).filename().c_str(),L"Gw.exe")){CloseHandle(process);return 3;}
 std::wcout<<L"Read-only scan: "<<path<<L" PID "<<pid<<L"\n";
 std::set<std::wstring> matches;
 constexpr size_t chunk=1024*1024,overlap=32768;
 uintptr_t address=0;
 while(address<0x7fff0000){
  MEMORY_BASIC_INFORMATION region{};
  if(!VirtualQueryEx(process,reinterpret_cast<void*>(address),&region,sizeof region))break;
  uintptr_t base=reinterpret_cast<uintptr_t>(region.BaseAddress),end=base+region.RegionSize;
  if(end<=address)break;
  if(region.State==MEM_COMMIT&&!(region.Protect&(PAGE_NOACCESS|PAGE_GUARD))){
   for(uintptr_t cursor=base;cursor<end;cursor+=chunk-overlap){
    size_t size=(std::min)(chunk,size_t(end-cursor));std::vector<wchar_t> buffer(size/2+1);SIZE_T read=0;
    if(!ReadProcessMemory(process,reinterpret_cast<void*>(cursor),buffer.data(),size,&read))continue;
    size_t count=read/2;
    for(size_t i=0;i+3<count;++i){
     if(buffer[i]!=L'<'||buffer[i+1]!=L'a'||buffer[i+2]!=L'=')continue;
     size_t first=i,last=i;
     while(first&&i-first<2048&&buffer[first-1])--first;
     while(last<count&&last-first<8192&&buffer[last])++last;
     if(last==count||last-first>=8192)continue;
     std::wstring text(buffer.data()+first,last-first);
     if(text.find(L"Take me to Warrior's Isle.")==std::wstring::npos&&text.find(L"What islands can you show me?")==std::wstring::npos)continue;
     // Reject arbitrary binary buffers that happen to contain an anchor.
     if(std::any_of(text.begin(),text.end(),[](wchar_t c){return c<32&&c!=L'\n'&&c!=L'\r'&&c!=L'\t';}))continue;
     matches.insert(std::move(text));
    }
   }
  }
  address=end;
 }
 CloseHandle(process);std::ofstream output{std::filesystem::path(argv[2])};
 for(const auto& text:matches){
  if(std::any_of(matches.begin(),matches.end(),[&](const auto& other){return other.size()>text.size()&&other.find(text)!=std::wstring::npos;}))continue;
  int n=WideCharToMultiByte(CP_UTF8,0,text.data(),int(text.size()),nullptr,0,nullptr,nullptr);
  std::string utf8(n,'\0');WideCharToMultiByte(CP_UTF8,0,text.data(),int(text.size()),utf8.data(),n,nullptr,nullptr);
  output<<utf8<<"\n\n";
 }
 std::cout<<"Decoded guild-hall dialog candidates: "<<matches.size()<<"\n";
 return output?0:4;
}

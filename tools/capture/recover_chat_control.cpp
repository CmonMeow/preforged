#define wmain capture_control_original_entry
#include "control.cpp"
#undef wmain
int wmain(int argc,wchar_t** argv)try{
 if(argc!=2)throw std::runtime_error("Usage: recover_chat_control PID");
 const auto pid=std::stoul(argv[1]);
 Handle process{OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,pid)};
 if(!process.h)throw std::runtime_error("cannot open client");
 wchar_t path[32768]{},home[32768]{},own[32768]{};DWORD count=32768;
 if(!QueryFullProcessImageNameW(process.h,0,path,&count)||!GetEnvironmentVariableW(L"USERPROFILE",home,32768))throw std::runtime_error("cannot verify client");
 const auto expected=std::wstring(home)+L"\\Desktop\\Guild Wars\\Gw.exe";
 if(_wcsicmp(path,expected.c_str()))throw std::runtime_error("refusing different client path");
 GetModuleFileNameW(nullptr,own,32768);
 const auto dll=std::filesystem::path(own).parent_path()/L"chat_recovery.dll";
 auto remote=load(process.h,pid,dll);
 auto local=LoadLibraryExW(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);
 if(!local)throw std::runtime_error("cannot inspect recovery export");
 auto entry=GetProcAddress(local,"RecoverChat");
 if(!entry){FreeLibrary(local);throw std::runtime_error("recovery export missing");}
 const auto offset=reinterpret_cast<std::uintptr_t>(entry)-reinterpret_cast<std::uintptr_t>(local);
 FreeLibrary(local);
 const auto result=invoke(process.h,remote+offset,nullptr);
 std::cout<<"Recovery result="<<result<<" (0 means native chat detour restored).\n";
 return result?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

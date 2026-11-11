#include <windows.h>
#include <tlhelp32.h>
#include <filesystem>
#include <string>
#include <iostream>
#include <stdexcept>
#include <fstream>
struct Handle{HANDLE h;~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}};
std::uintptr_t module(DWORD pid,const std::wstring& name){
 Handle list{CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid)};
 MODULEENTRY32W e{};e.dwSize=sizeof e;
 if(Module32FirstW(list.h,&e))do{if(!_wcsicmp(e.szModule,name.c_str()))return reinterpret_cast<std::uintptr_t>(e.modBaseAddr);}while(Module32NextW(list.h,&e));
 return 0;
}
DWORD invoke(HANDLE process,std::uintptr_t function,void* argument){
 Handle thread{CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(function),argument,0,nullptr)};
 if(!thread.h)throw std::runtime_error("cannot start client control call");
 if(WaitForSingleObject(thread.h,10000)!=WAIT_OBJECT_0)throw std::runtime_error("control call still pending; do not retry until checked");
 DWORD result=0;if(!GetExitCodeThread(thread.h,&result))throw std::runtime_error("cannot read control result");return result;
}
std::uintptr_t remote_loader(DWORD pid){
 auto function=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");HMODULE owner=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(function),&owner))throw std::runtime_error("cannot resolve loader owner");
 wchar_t path[32768];GetModuleFileNameW(owner,path,32768);auto base=module(pid,std::filesystem::path(path).filename().wstring());
 if(!base)throw std::runtime_error("client loader module absent");
 return base+reinterpret_cast<std::uintptr_t>(function)-reinterpret_cast<std::uintptr_t>(owner);
}
std::uintptr_t load(HANDLE process,DWORD pid,const std::filesystem::path& path){
 auto text=path.wstring();const auto bytes=(text.size()+1)*sizeof(wchar_t);
 void* remote=VirtualAllocEx(process,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!remote)throw std::runtime_error("cannot allocate DLL path");SIZE_T written=0;
 if(!WriteProcessMemory(process,remote,text.c_str(),bytes,&written)||written!=bytes){VirtualFreeEx(process,remote,0,MEM_RELEASE);throw std::runtime_error("cannot write DLL path");}
 // On timeout retain this small allocation: the loader may still be reading it.
 auto result=invoke(process,remote_loader(pid),remote);VirtualFreeEx(process,remote,0,MEM_RELEASE);
 if(!result)throw std::runtime_error("client could not load library");return result;
}
bool official_path(const wchar_t* path){
 return !_wcsicmp(std::filesystem::path(path).filename().c_str(),L"Gw.exe");
}
void compatible_client(const wchar_t* path){
 std::ifstream input(std::filesystem::path(path),std::ios::binary);
 IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS32 nt{};
 input.read(reinterpret_cast<char*>(&dos),sizeof dos);
 if(!input||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<0||dos.e_lfanew>1048576)throw std::runtime_error("invalid client PE image");
 input.seekg(dos.e_lfanew);input.read(reinterpret_cast<char*>(&nt),sizeof nt);
 if(!input||nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||!nt.OptionalHeader.SizeOfImage)
  throw std::runtime_error("unsupported client image; expected a valid 32-bit Guild Wars PE executable");
}
DWORD find_client(){
 Handle list{CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0)};
 PROCESSENTRY32W e{};e.dwSize=sizeof e;DWORD found=0;
 if(Process32FirstW(list.h,&e))do{
  if(_wcsicmp(e.szExeFile,L"Gw.exe"))continue;
  Handle process{OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,e.th32ProcessID)};
  wchar_t path[32768];DWORD size=32768;
  if(process.h&&QueryFullProcessImageNameW(process.h,0,path,&size)&&
     official_path(path)){
   if(found)throw std::runtime_error("multiple official clients found; specify PID");found=e.th32ProcessID;
  }
 }while(Process32NextW(list.h,&e));
 if(!found)throw std::runtime_error("Gw.exe is not running; open Guild Wars first");return found;
}
int wmain(int argc,wchar_t**argv)try{
 if(argc!=3){std::cout<<"Usage: capture_control auto|PID start|stop|status|inspect\n";return 2;}
 const auto pid=std::wstring(argv[1])==L"auto"?find_client():std::stoul(argv[1]);std::wstring action=argv[2];
 if(action!=L"start"&&action!=L"stop"&&action!=L"status"&&action!=L"inspect"&&action!=L"start-test")throw std::runtime_error("unknown command");

 const DWORD access=action==L"inspect"?PROCESS_QUERY_LIMITED_INFORMATION:PROCESS_QUERY_INFORMATION|PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ;
 Handle process{OpenProcess(access,FALSE,pid)};
 if(!process.h)throw std::runtime_error("cannot open specified client");
 wchar_t executable[32768];DWORD length=32768;
 if(!QueryFullProcessImageNameW(process.h,0,executable,&length))throw std::runtime_error("cannot identify target");
 if(action==L"start-test"){
  if(std::wstring(argv[1])==L"auto"||!official_path(executable))throw std::runtime_error("test attach requires explicit PID of authorized official installation");
  action=L"start";
 }else if(!official_path(executable))throw std::runtime_error("refusing unrecognized client");
 if(action==L"start")compatible_client(executable);
 std::wcout<<L"Selected PID "<<pid<<L": "<<executable<<L"\n";
 if(action==L"inspect"){
  std::cout<<"Official client PID "<<pid<<"; GWCA loaded="<<(module(pid,L"gwca.dll")!=0)
    <<"; collector loaded="<<(module(pid,L"world_capture.dll")!=0)<<'\n';return 0;
 }
 wchar_t own[32768];GetModuleFileNameW(nullptr,own,32768);auto directory=std::filesystem::path(own).parent_path();
 auto target=module(pid,L"world_capture.dll");
 if(target){
  Handle list{CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid)};
  MODULEENTRY32W entry{};entry.dwSize=sizeof entry;bool same=false;
  if(Module32FirstW(list.h,&entry))do{
   if(reinterpret_cast<std::uintptr_t>(entry.modBaseAddr)==target)
    same=!_wcsicmp(entry.szExePath,(directory/L"world_capture.dll").c_str());
  }while(Module32NextW(list.h,&entry));
  if(!same)throw std::runtime_error("another logger copy is attached; use that original folder to control it");
 }
 if(!target){
  if(action!=L"start")throw std::runtime_error("collector not attached");
  // Do not reuse an unknown GWCA already loaded by another tool.
  if(module(pid,L"gwca.dll"))throw std::runtime_error("GWCA already loaded; collector coexistence has not been verified");
  load(process.h,pid,directory/L"gwca.dll");target=load(process.h,pid,directory/L"world_capture.dll");
 }
 auto image=LoadLibraryExW((directory/L"world_capture.dll").c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);
 if(!image)throw std::runtime_error("cannot inspect local collector exports");
 auto name=action==L"start"?"StartCapture":action==L"stop"?"StopCapture":"CaptureStatus";
 auto function=GetProcAddress(image,name);if(!function)throw std::runtime_error("collector export missing");
 auto offset=reinterpret_cast<std::uintptr_t>(function)-reinterpret_cast<std::uintptr_t>(image);FreeLibrary(image);
 auto result=invoke(process.h,target+offset,nullptr);
 std::cout<<"Result "<<result<<" (status: 0 idle, 1 starting, 2 stopping, 3 finished, 4 ready).\n";
 if(action==L"start"&&result==0){
  auto local=LoadLibraryExW((directory/L"world_capture.dll").c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);
  if(!local)throw std::runtime_error("cannot inspect collector status export");
  auto status=GetProcAddress(local,"CaptureStatus");
  if(!status){FreeLibrary(local);throw std::runtime_error("collector status export missing");}
  const auto status_offset=reinterpret_cast<std::uintptr_t>(status)-reinterpret_cast<std::uintptr_t>(local);
  FreeLibrary(local);
  const auto deadline=GetTickCount64()+10000;
  do{
   DWORD process_status=0;
   if(!GetExitCodeProcess(process.h,&process_status)||process_status!=STILL_ACTIVE)
    throw std::runtime_error("client exited during logger initialization; see world-*.diagnostic.txt; do not retry attachment");
   const auto state=invoke(process.h,target+status_offset,nullptr);
   if(state==4){std::cout<<"Logger initialization completed. Capture is ready.\n";return 0;}
   if(state==2||state==3)throw std::runtime_error("logger initialization stopped or failed; see world-*.jsonl diagnostics");
   Sleep(250);
  }while(GetTickCount64()<deadline);
  throw std::runtime_error("logger is still initializing; no ready state received. Check STATUS.cmd before another start");
 }
 return action==L"status"?0:static_cast<int>(result);
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

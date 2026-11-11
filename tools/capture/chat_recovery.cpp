// Separate recovery only: never initializes, loads GWCA, or registers hooks.
#include "chat_render_cleanup.h"
#include "chat_hook.h"
#include <bcrypt.h>
#include <fstream>
#include <string>
#include <array>
#include <vector>
namespace {
std::string hash(const wchar_t* path){
 std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("recovery image unavailable");
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE digest=nullptr;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA provider unavailable");
 struct Cleanup{BCRYPT_ALG_HANDLE& a;BCRYPT_HASH_HANDLE& h;~Cleanup(){if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);}} cleanup{algorithm,digest};
 DWORD length=0,used=0;if(BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&length),4,&used,0)<0)throw std::runtime_error("SHA size unavailable");
 std::vector<unsigned char> object(length);if(BCryptCreateHash(algorithm,&digest,object.data(),length,nullptr,0,0)<0)throw std::runtime_error("SHA creation failed");
 std::array<char,65536> buffer{};while(in){in.read(buffer.data(),buffer.size());const auto n=in.gcount();if(n&&BCryptHashData(digest,reinterpret_cast<PUCHAR>(buffer.data()),static_cast<ULONG>(n),0)<0)throw std::runtime_error("SHA read failed");}if(!in.eof())throw std::runtime_error("image read failed");
 std::array<unsigned char,32> bytes{};if(BCryptFinishHash(digest,bytes.data(),32,0)<0)throw std::runtime_error("SHA finish failed");
 std::string result;constexpr char hex[]="0123456789abcdef";for(auto c:bytes){result+=hex[c>>4];result+=hex[c&15];}return result;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI RecoverChat(void*){
 try{
  auto gwca=GetModuleHandleW(L"gwca.dll"),game=GetModuleHandleW(nullptr);if(!gwca||!game)return ERROR_MOD_NOT_FOUND;
  wchar_t client_path[32768]{},library_path[32768]{},home[32768]{};
  if(!GetModuleFileNameW(game,client_path,32768)||!GetModuleFileNameW(gwca,library_path,32768)||!GetEnvironmentVariableW(L"USERPROFILE",home,32768))return ERROR_BAD_PATHNAME;
  const auto expected=std::wstring(home)+L"\\Desktop\\Guild Wars\\Gw.exe";
  if(_wcsicmp(client_path,expected.c_str())||hash(client_path)!="8e50edfb83515fabfec50bfd9d17968e8635a39b4c3e796e311bbc5cd15e5878"||hash(library_path)!="b33375229bcb06e415b71246ba317966c0ec6c76f71f284bc49f1783685e1975")return ERROR_REVISION_MISMATCH;
  const auto b=reinterpret_cast<std::uintptr_t>(gwca),g=reinterpret_cast<std::uintptr_t>(game);
  const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(b);if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return ERROR_BAD_EXE_FORMAT;
  const auto* pe=reinterpret_cast<const IMAGE_NT_HEADERS32*>(b+dos->e_lfanew);if(pe->Signature!=IMAGE_NT_SIGNATURE||pe->FileHeader.TimeDateStamp!=0x6a9065a5||pe->OptionalHeader.SizeOfImage!=0x5a000)return ERROR_BAD_EXE_FORMAT;
  const auto* game_dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(g);if(game_dos->e_magic!=IMAGE_DOS_SIGNATURE)return ERROR_BAD_EXE_FORMAT;
  const auto* game_pe=reinterpret_cast<const IMAGE_NT_HEADERS32*>(g+game_dos->e_lfanew);if(game_pe->Signature!=IMAGE_NT_SIGNATURE||game_pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||game_pe->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||game_pe->OptionalHeader.SizeOfImage<8)return ERROR_BAD_EXE_FORMAT;
  auto api=reinterpret_cast<void(__cdecl*)(GW::Hook::HookTarget)>(GetProcAddress(gwca,"?DisableHooks@Hook@GW@@YAXUHookTarget@12@@Z"));
  if(!api)return ERROR_PROC_NOT_FOUND;
  const auto chat=capture::chat_command_hook(gwca);
  const auto* code=reinterpret_cast<const unsigned char*>(chat.target);
  const auto original=*reinterpret_cast<const std::uint32_t*>(b+0x53b44);
  if(code[0]==0xe9){
   std::int32_t relative=0;std::memcpy(&relative,code+1,4);
   if(chat.target+5+relative!=b+0x42c9)return ERROR_REVISION_MISMATCH;
   api(GW::Hook::HookTarget(reinterpret_cast<void*>(chat.target)));
  }
  if(std::memcmp(reinterpret_cast<const void*>(chat.target),reinterpret_cast<const void*>(original),5))return ERROR_INVALID_DATA;
  capture::disable_orphan_chat_render(b,g,game_pe->OptionalHeader.SizeOfImage,api);return ERROR_SUCCESS;
 }catch(...){return ERROR_INVALID_DATA;}
}
BOOL WINAPI DllMain(HMODULE module,DWORD reason,void*){if(reason==DLL_PROCESS_ATTACH)DisableThreadLibraryCalls(module);return TRUE;}

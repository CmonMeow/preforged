#include <windows.h>
#include <iostream>
#include <string>
#include <algorithm>
int wmain(int argc,wchar_t**argv){
 if(argc!=2)return 2;
 std::wstring path=argv[1];std::replace(path.begin(),path.end(),L'/',L'\\');
 auto module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if(!module){std::cerr<<"load failed "<<GetLastError()<<'\n';return 1;}
 using Call=DWORD(WINAPI*)(void*);
 auto start=reinterpret_cast<Call>(GetProcAddress(module,"StartCapture"));
 auto stop=reinterpret_cast<Call>(GetProcAddress(module,"StopCapture"));
 auto status=reinterpret_cast<Call>(GetProcAddress(module,"CaptureStatus"));
 if(!start||!stop||!status||status(nullptr)!=0||stop(nullptr)!=0||status(nullptr)!=0)return 1;
 FreeLibrary(module);std::cout<<"PASS exports, dependency loading, idle status and pre-start stop\n";return 0;
}


#include "network.h"
#include "server_settings.h"
#include "runtime_log.h"
#include "gw_salvage_catalog.h"
#include "gw_identification_catalog.h"
#include <windows.h>
#include <sodium.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <atomic>
#include <chrono>
#include <array>
#include <cstdlib>
#include <syncstream>
namespace { std::atomic<bool> stop_requested{false};
void build_info(){
    std::cout<<"Guild Wars 2022 single server; native direct listeners; client build37130\n"
             <<"Host entrypoint compiled "<<__DATE__<<' '<<__TIME__<<"; process "<<GetCurrentProcessId()<<'\n';
    // The executable hash identifies the linked core as well as this entrypoint;
    // an unchanged entrypoint object alone cannot identify a gameplay update.
    wchar_t module[32768]{};
    if(!GetModuleFileNameW(nullptr,module,32768)||sodium_init()<0)return;
    std::ifstream binary(std::filesystem::path(module),std::ios::binary);
    if(!binary)return;
    crypto_hash_sha256_state state;crypto_hash_sha256_init(&state);
    std::array<char,65536> buffer{};
    while(binary){binary.read(buffer.data(),buffer.size());const auto bytes=binary.gcount();
        if(bytes>0)crypto_hash_sha256_update(&state,reinterpret_cast<const unsigned char*>(buffer.data()),static_cast<unsigned long long>(bytes));}
    if(binary.bad())return;
    std::array<unsigned char,crypto_hash_sha256_BYTES> hash{};crypto_hash_sha256_final(&state,hash.data());
    std::array<char,crypto_hash_sha256_BYTES*2+1> hex{};sodium_bin2hex(hex.data(),hex.size(),hash.data(),hash.size());
    std::cout<<"Executable SHA256 "<<hex.data()<<'\n';
}
BOOL WINAPI control(DWORD event) {
    if(event==CTRL_C_EVENT || event==CTRL_BREAK_EVENT || event==CTRL_CLOSE_EVENT) { stop_requested=true; return TRUE; }
    return FALSE;
}}
int main(int argc,char** argv) {
    try {
        wchar_t path[32768]{}; GetModuleFileNameW(nullptr,path,32768);
        std::filesystem::current_path(std::filesystem::path(path).parent_path().parent_path());
        if(argc==2 && std::string_view(argv[1])=="--build-info") { build_info(); return 0; }
        if(argc!=1) throw std::runtime_error("Usage: server_host.exe (no arguments)");
        if(sodium_init()<0) throw std::runtime_error("Random source unavailable");
        std::filesystem::create_directories("data/logs");
        RuntimeLog log("data/logs/server.log");
        build_info();
        _putenv_s("FW_GW_PROBE_EMPTY_MANIFEST","1");
        _putenv_s("FW_GW_PROBE_PLAYER_VIEW","1");
        _putenv_s("FW_GW_PROBE_MAP_EXPLORATION","1");
        gwserver::ServerSettings settings;
        std::ifstream endpoints("config/server.txt");
        if(!endpoints)throw std::runtime_error("config/server.txt unavailable");
        gwserver::load_endpoints(endpoints,settings);
        gwserver::NetworkManager manager(settings.database,settings.item_catalogs,settings.quest_catalog);
        std::cout<<"QUEST content_catalog "<<(settings.quest_catalog?"loaded":"unconfigured")<<" quests="<<manager.quest_catalog().size()<<'\n';
        const auto& catalogs=manager.item_catalogs();
        std::cout<<"ITEM salvage_catalog "<<(catalogs.paths().salvage?"loaded":"unconfigured")<<" recipes="<<catalogs.salvage().size()<<'\n';
        std::cout<<"ITEM identification_catalog "<<(catalogs.paths().identification?"loaded":"unconfigured")<<" recipes="<<catalogs.identification().size()<<'\n';
        std::cout<<"ITEM consumable_catalog "<<(catalogs.paths().consumables?"loaded":"unconfigured")<<" recipes="<<catalogs.consumables().size()<<'\n';
        SetConsoleCtrlHandler(control,TRUE);
        manager.server().start(settings);
        std::osyncstream(std::cout)<<"Server ready. Portal "<<settings.portal_port<<" / auth "<<settings.auth_port<<" / world "<<settings.auth_port+1<<".\nLocal login: test@example.test / test\nKeep this console open. Ctrl+C stops the server.\n"<<std::flush;
        while(!stop_requested && manager.server().healthy()) std::this_thread::sleep_for(std::chrono::milliseconds(25));
        const bool failed=!stop_requested;
        manager.server().stop(); SetConsoleCtrlHandler(control,FALSE);
        return failed?1:0;
    } catch(const std::exception& e) {
        const auto message=std::string("Server error: ")+e.what();
        std::cerr<<message<<'\n';
        // RuntimeLog has unwound here; retain the failure after restoring streams.
        std::ofstream failure("data/logs/server.log",std::ios::app);
        if(failure)failure<<message<<'\n';
        DWORD console_processes[2]{};
        if(GetConsoleProcessList(console_processes,2)==1){
            std::cerr<<"Press Enter to close this window.\n"<<std::flush;
            std::string line;std::getline(std::cin,line);
        }
        return 1;
    }
}

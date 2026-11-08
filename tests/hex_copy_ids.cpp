#include "gw_hex_copy_ids.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills;
void check(bool ok){if(!ok)throw std::runtime_error("Hex copy allocation");}
int main()try{using namespace effects;Scope owner{1,42,1};Slots bar{};bar[0]={28,0};bar[1]={28,2};HexCopyIds ids(owner);
 check(ids.allocate(owner,28,bar)==Slot{28,3});check(ids.allocate(owner,28,bar)==Slot{28,4});
 bar={};check(ids.allocate(owner,28,bar)==Slot{28,5}); // Removal does not make an old identity reusable.
 check(ids.allocate(owner,30,bar)==Slot{30,1});
 auto stale=owner;++stale.generation;check(!ids.allocate(stale,28,bar));check(!ids.allocate(owner,22,bar));
 bar[0]={28,65534};check(ids.allocate(owner,28,bar)==Slot{28,65535});check(!ids.allocate(owner,28,Slots{}));
 HexCopyIds fresh(owner);bar[0]={28,65535};check(!fresh.allocate(owner,28,bar));
 check(fresh.allocate(owner,28,Slots{})==Slot{28,1}); // Failed preflight does not poison its cursor.
 bar[0]={28,65536};check(!fresh.allocate(owner,28,bar));
 std::cout<<"PASS Scope-owned nonreused copy IDs, occupied-copy exclusion and16-bit pending-key boundary\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

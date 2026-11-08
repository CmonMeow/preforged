#include "gw_panic.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Panic policy");}
int main()try{
 check(panic(0)->duration==1000&&panic(12)->duration==8000&&panic(15)->duration==10000&&panic(20)->duration==13000&&!panic(21));
 const Scope victim{1,42,1};const Entry hex{1,43,52,12,Category::hex,100,8100};PanicUse use{victim,1,1,200,300,true};
 check(bool(panic_reaction(use,hex)));auto failed=use;failed.success=false;check(!panic_reaction(failed,hex));
 auto basic=use;basic.skill=0;check(!panic_reaction(basic,hex));auto expired=use;expired.used=8100;check(!panic_reaction(expired,hex));
 std::vector<PanicContact> contacts{{victim,0,true,true,true},{{1,44,1},252,true,true,true},{{1,45,1},100,true,true,false},{{1,46,1},252.1f,true,true,true},{{1,47,1},10,true,false,true},{{1,48,1},10,false,true,true}};
 const auto recipients=panic_recipients(victim,contacts);check(recipients&&recipients->size()==2&&recipients->front().owner.actor==44&&recipients->front().interrupt_attempt&&!recipients->back().interrupt_attempt);
 auto bad=contacts;bad.push_back(contacts[1]);check(!panic_recipients(victim,bad));bad=contacts;++bad[0].owner.generation;check(!panic_recipients(victim,bad));
 PanicEvents events(victim);const auto first=events.observe(use,hex);check(first&&first->reaction);check(!events.observe(use,hex));
 ++use.event;use.used=400;const auto no_hex=events.observe(use,{});check(no_hex&&!no_hex->reaction);check(!events.observe(use,hex));
 ++use.event;use.used=399;check(!events.observe(use,hex));auto stale=use;++stale.owner.generation;check(!events.observe(stale,hex));
 std::cout<<"PASS Panic successful skill-use gate, trigger exclusion,252 boundary, independent attempt recipients and replay/lifetime isolation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

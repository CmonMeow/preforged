#include "gw_power_block_disables.h"
#include "gw_humility_disables.h"
#include "gw_diversion_authority.h"
#include "gw_keystone_recharge.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw::skills;using namespace effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Disable reset line "+std::to_string(at.line()));}
int main()try{
 const Scope owner{7,1,2},caster{7,2,3};PowerBlockDisables power(owner);
 check(power.apply(owner,caster,*power_block(12),66,100));check(power.disabled_until(owner,Slot{62,1},100)==10100);
 auto reset=power.propose_reset_slots(owner,{{62,1}},200);check(reset&&reset->revision()==power.revision()+1&&power.disabled_until(owner,Slot{62,1},200)==10100);
 check(reset->disabled_until(owner,Slot{62,1},200)==0&&reset->disabled_until(owner,62,200)==10100&&reset->disabled_until(owner,66,200)==10100);
 check(reset->apply(owner,caster,*power_block(0),66,200));check(reset->disabled_until(owner,Slot{62,1},200)==1200&&reset->disabled_until(owner,62,200)==10100&&reset->disabled_until(owner,Slot{62,1},1200)==0);
 auto h=HumilityDisables(owner).propose(owner,*signet_humility(20),false,100);check(bool(h));h=h->propose(owner,*signet_humility(20),true,100);check(bool(h));
 auto hr=h->propose_reset_slots(owner,{{294,1}},200);check(hr&&hr->disabled_until(owner,Slot{294,1},200)==0&&hr->disabled_until(owner,294,200)==21100&&hr->disabled_until(owner,259,200)==21100);
 hr=hr->propose(owner,*signet_humility(0),false,200);check(hr&&hr->disabled_until(owner,Slot{294,1},200)==1200);hr=hr->propose(owner,*signet_humility(0),true,300);check(hr&&hr->disabled_until(owner,Slot{294,1},300)==10300);
 KeystoneCooldowns snapshot{owner,1,0,100,{{{2,0},0,10100,false},{{2,1},0,10100,true}}};auto k=prepare_keystone_recharge(snapshot,owner,200,1);check(k&&k->cleared==std::vector<Slot>{{2,0}});
 auto spent=h->propose_reset_slots(owner,k->cleared,200);check(spent&&spent->disabled_until(owner,2,200)==0&&spent->disabled_until(owner,Slot{2,1},200)==10100);
 DiversionAuthority diversion(owner);check(diversion.record({owner,1,{62,1},true,false},100,10000)&&diversion.record({owner,2,{62,0},true,false},100,20000));
 auto dr=diversion.propose_reset_slots(owner,{{62,1}},200);check(dr&&dr->revision()==diversion.revision()+1&&dr->until(owner,{62,1},200)==0&&dr->until(owner,{62,0},200)==20000&&diversion.until(owner,{62,1},200)==10000);
 check(!dr->record({owner,2,{62,1},true,false},200,500));check(dr->record({owner,3,{62,1},true,false},200,500)&&dr->until(owner,{62,1},200)==500);
 const Scope stale{7,1,99};check(!power.propose_reset_slots(stale,{{62,1}},200)&&!h->propose_reset_slots(stale,{{294,1}},200)&&!diversion.propose_reset_slots(stale,{{62,1}},200));
 check(!power.propose_reset_slots(owner,{{62,1},{62,1}},200)&&!h->propose_reset_slots(owner,{{294,65536}},200)&&!diversion.propose_reset_slots(owner,{{0,0}},200));
 check(!reset->propose_reset_slots(owner,{{62,1}},199)&&!hr->propose_reset_slots(owner,{{294,1}},299)&&!dr->propose_reset_slots(owner,{{62,1}},199));
 const Scope invalid_actor{7,65536,2};DiversionAuthority invalid(invalid_actor);
 check(!invalid.record({invalid_actor,1,{62,0},true,false},100,500));
 check(!invalid.propose_reset_slots(invalid_actor,{{62,0}},100)&&!invalid.until(invalid_actor,{62,0},100));
 std::cout<<"PASS copy-scoped Power Block/Humility/Diversion reset proposals, same-time shorter re-disable, spent-copy isolation, revision and stale/replay validation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

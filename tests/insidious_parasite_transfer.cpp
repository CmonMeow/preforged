#include "gw_insidious_parasite_transfer.h"
#include "gw_weapon_hit.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;using namespace combat::weapon;
void check(bool ok){if(!ok)throw std::runtime_error("Insidious Parasite transfer regression");}
int main()try{
 const Scope caster{1,42,7},afflicted{1,20,9};const Entry hex{10,42,123,12,Category::hex,100,13100};
 QualifiedHit hit{afflicted,caster,1,100,600};
 auto transfer=insidious_parasite_transfer(caster,afflicted,hex,hit,{100,500},{500,500});check(bool(transfer));
 check(transfer->at==600&&transfer->health.stolen==39&&transfer->health.source.current==139&&transfer->health.target.current==461);
 // The caster is also the incoming attack recipient. Using the old100HP
 // snapshot incorrectly kills them;committing the39HP steal first leaves19HP.
 combat::Actor recipient{42,7,transfer->health.source,{},0,true};
 auto struck=apply_resolved(recipient,120,true);check(struck&&struck->after.current==19&&!struck->killed);
 transfer=insidious_parasite_transfer(caster,afflicted,hex,hit,{490,500},{500,500});check(transfer&&transfer->health.stolen==39&&transfer->health.gained==10&&transfer->health.source.current==500);
 transfer=insidious_parasite_transfer(caster,afflicted,hex,hit,{100,500},{10,500});check(transfer&&transfer->health.stolen==11&&transfer->health.source.current==111&&transfer->health.killed);
 transfer=insidious_parasite_transfer(caster,afflicted,hex,hit,{100,500},{500,500},5);check(transfer&&transfer->health.stolen==34);
 check(!insidious_parasite_transfer(caster,afflicted,hex,hit,{0,500},{500,500}));
 check(!insidious_parasite_transfer(caster,afflicted,hex,hit,{100,500},{0,500}));
 check(!insidious_parasite_transfer({2,42,7},afflicted,hex,hit,{100,500},{500,500}));
 check(!insidious_parasite_transfer({1,43,7},afflicted,hex,hit,{100,500},{500,500}));
 auto invalid=hit;invalid.event=0;check(!insidious_parasite_transfer(caster,afflicted,hex,invalid,{100,500},{500,500}));
 invalid=hit;invalid.at=13100;check(!insidious_parasite_transfer(caster,afflicted,hex,invalid,{100,500},{500,500}));
 std::cout<<"PASS Insidious Parasite pre-damage Health transfer,survival ordering,overheal/overkill,reduction and invalid actor/result gates;arithmetic only;see world lifecycle suites\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

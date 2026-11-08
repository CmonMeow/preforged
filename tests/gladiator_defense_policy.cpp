#include "gw_gladiator_defense.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Gladiator Defense policy");}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){auto p=gladiator_defense(rank);check(p&&p->retaliation==5+rank*2&&p->block==.75);}
 check(!gladiator_defense(21));
 const Scope owner{1,25,2},attacker{1,26,3};Registry registry(owner);
 check(registry.apply(owner,{100,25,372,12,Category::stance,100,10100},Replace::category).accepted);
 const auto entries=registry.active(owner,200);
 const auto reviewed=reviewed_block_sources(entries,200);check(reviewed&&reviewed->size()==1&&reviewed->front().skill==372);
 check(reviewed_block_probability(entries,false,200)==.75);
 check(reviewed_block_probability(entries,true,200)==0.);
 check(reviewed_block_probability(entries,false,10100)==0.);
 auto invalid=entries;invalid.front().rank=21;check(!reviewed_block_probability(invalid,false,200)&&!reviewed_block_sources(invalid,200));
 auto rigor=entries;rigor.push_back({101,26,137,0,Category::hex,100,1100});
 check(reviewed_block_probability(rigor,false,200)==0.);
 const std::array<BlockSource,2> sources{{{258,99,.5},{372,100,.75}}};
 const auto other=*resolve_attributed_block_draw(sources,.1);
 const auto own=*resolve_attributed_block_draw(sources,.6);
 check(!gladiator_retaliation(registry,owner,attacker,other,true,200));
 const auto hit=gladiator_retaliation(registry,owner,attacker,own,true,200);
 check(hit&&hit->damage==29&&hit->source==owner&&hit->target==attacker&&hit->at==200);
 check(!gladiator_retaliation(registry,owner,attacker,own,false,200));
 check(!gladiator_retaliation(registry,owner,attacker,own,true,99));
 check(!gladiator_retaliation(registry,owner,attacker,own,true,10100));
 check(!gladiator_retaliation(registry,{1,25,3},attacker,own,true,200));
 check(!gladiator_retaliation(registry,owner,{2,26,3},own,true,200));
 check(!gladiator_retaliation(registry,owner,owner,own,true,200));
 check(!gladiator_retaliation(registry,owner,attacker,*resolve_attributed_block_draw(sources,.9),true,200));
 auto wrong=own;wrong.source->instance=101;check(!gladiator_retaliation(registry,owner,attacker,wrong,true,200));
 check(registry.remove(owner,100).has_value());check(!gladiator_retaliation(registry,owner,attacker,own,true,200));
 std::cout<<"PASS Gladiator Defense pinned policy,attributed melee-only retaliation,scope/expiry/removal and competing block rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

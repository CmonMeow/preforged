#include "gw_necrotic_traversal.h"
#include "gw_corpse_reservations.h"
#include <iostream>
#include <limits>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Necrotic Traversal line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=necrotic_traversal(rank);check(p&&p->poison==5000+1000*rank&&p->energy==5&&p->activation==750);}
 check(!necrotic_traversal(21));const auto plan=*necrotic_traversal(12);const Scope caster{1,1,1};
 const TraversalCorpse near{{1,100,1},1,0,100,0,true,true,false},far{{1,101,1},2,0,1000,0,true,true,false};
 std::vector<TraversalCorpse> corpses{near,far};
 auto pool=necrotic_traversal_pool(plan,caster,0,0,corpses);check(pool&&pool->corpses.size()==2);
 const auto chosen=select_necrotic_traversal_corpse(*pool,1,100);check(chosen&&chosen->corpse==far);
 check(select_necrotic_traversal_corpse(*pool,0,100)->corpse==near);check(!select_necrotic_traversal_corpse(*pool,2,100));
 check(!select_necrotic_traversal_corpse(*pool,0,0));check(!select_necrotic_traversal_corpse(*pool,0,UINT64_MAX));
 auto consumed=consume_necrotic_traversal_corpse(*chosen,far);check(consumed&&consumed->consumed&&consumed->revision==3);
 check(!consume_necrotic_traversal_corpse(*chosen,*consumed));
 auto changed=far;++changed.scope.generation;check(!consume_necrotic_traversal_corpse(*chosen,changed));
 changed=far;++changed.revision;check(!consume_necrotic_traversal_corpse(*chosen,changed));
 changed=far;changed.x+=1;check(!consume_necrotic_traversal_corpse(*chosen,changed));
 changed=far;changed.exploitable.reset();check(!consume_necrotic_traversal_corpse(*chosen,changed));
 auto badplan=plan;badplan.poison=1;check(!necrotic_traversal_pool(badplan,caster,0,0,corpses));
 auto unknown=near;unknown.exploitable.reset();corpses={unknown,far};check(!necrotic_traversal_pool(plan,caster,0,0,corpses));
 unknown.x=1248.01f;corpses={unknown,far};check(necrotic_traversal_pool(plan,caster,0,0,corpses)->corpses.size()==1);
 unknown=near;unknown.exploitable=false;corpses={unknown,far};check(necrotic_traversal_pool(plan,caster,0,0,corpses)->corpses.size()==1);
 unknown.exploitable.reset();unknown.consumed=true;corpses={unknown,far};check(necrotic_traversal_pool(plan,caster,0,0,corpses)->corpses.size()==1);
 unknown=near;unknown.dead=false;unknown.exploitable.reset();corpses={unknown,far};check(necrotic_traversal_pool(plan,caster,0,0,corpses)->corpses.size()==1);
 auto edge=near;edge.x=1248;edge.plane=1;corpses={edge};check(necrotic_traversal_pool(plan,caster,0,0,corpses)->corpses.size()==1); // Navigation, not plane identity, decides the step.
 corpses={near,near};check(!necrotic_traversal_pool(plan,caster,0,0,corpses));
 auto invalid=near;invalid.revision=UINT64_MAX;corpses={invalid};check(!necrotic_traversal_pool(plan,caster,0,0,corpses));
 invalid=near;invalid.x=std::numeric_limits<float>::quiet_NaN();corpses={invalid};check(!necrotic_traversal_pool(plan,caster,0,0,corpses));
 auto forged=*pool;forged.corpses[1].x=5000;check(!select_necrotic_traversal_corpse(forged,1,100));
 auto badclaim=*chosen;badclaim.corpse.scope.actor=0;check(!consume_necrotic_traversal_corpse(badclaim,badclaim.corpse));
 badclaim=*chosen;badclaim.corpse.plane=65536;check(!consume_necrotic_traversal_corpse(badclaim,badclaim.corpse));
 badclaim=*chosen;badclaim.corpse.x=std::numeric_limits<float>::infinity();check(!consume_necrotic_traversal_corpse(badclaim,badclaim.corpse));
 quests::CorpseReservations ledger;check(ledger.reserve_exploitation(near.scope,caster,97,1));
 check(!ledger.reserve_well(near.scope,caster,97,1)&&!ledger.owns_well(near.scope,caster,97,1)&&!ledger.consume_well(near.scope,caster,97,1));
 check(!ledger.reserve_well(near.scope,caster,92,2));check(!ledger.reserve(near.scope,{caster,82,1}));
 check(!ledger.consume_exploitation(near.scope,caster,97,2));check(ledger.release(near.scope,{caster,0,0,97,1}));
 check(ledger.reserve_well(near.scope,caster,92,2));check(!ledger.reserve_exploitation(near.scope,caster,97,3));
 check(ledger.consume_well(near.scope,caster,92,2));check(!ledger.reserve_exploitation(near.scope,caster,97,3));
 auto next=near.scope;++next.generation;check(ledger.reserve_exploitation(next,caster,97,3));check(ledger.consume_exploitation(next,caster,97,3));
 check(!ledger.reserve_exploitation(far.scope,caster,98,4));
 const Scope pet{1,102,1};check(ledger.can_pre_exploit(pet)&&ledger.pre_exploit(pet)&&ledger.pre_exploit(pet));
 check(!ledger.available(pet)&&!ledger.reserve_well(pet,caster,92,5)&&!ledger.reserve_exploitation(pet,caster,97,5));
 const Scope reserved{1,103,1};check(ledger.reserve_well(reserved,caster,92,6)&&!ledger.pre_exploit(reserved));
 ledger.retire(pet);check(ledger.available(pet));
 std::cout<<"Necrotic97 ranks0..20, non-nearest selection, unknown-pool refusal, scoped compare/consume and shared corpse competition pass\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

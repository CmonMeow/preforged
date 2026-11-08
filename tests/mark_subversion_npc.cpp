#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace combat;using namespace combat::npcs;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Mark Subversion NPC line "+std::to_string(at.line()));
}
int main()try{
 const Lifetime life{1,2,3,146,100};const Scope source{1,1,1},ally{1,101,2};
 Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=Relation::hostile_to_players;
 d.spawn_health=Health{1000,1000};d.damageable=d.conditions_allowed=true;
 for(unsigned rank=0;rank<=20;++rank){
  LiveActor a(d,life);const auto target=a.hex_scope();const MarkSubversionOwnership credit{source,target,77,100};
  const SubversionSpellTarget selected{target,ally,SpellTargetRelation::ally};
  check(a.apply_mark_subversion(life,source,*mark_subversion(rank),77,100).has_value());
  check(!a.apply_mark_subversion_failure(life,credit,source,ally,281,selected,1,200,{50,500}));
  check(a.actor(life)->health->current==1000&&a.hex_effect(life,77));
  check(a.advance_conditions(life,200).has_value());
  auto stale=source;++stale.generation;
  check(!a.apply_mark_subversion_failure(life,credit,stale,ally,281,selected,1,200,{50,500}));
  check(!a.apply_mark_subversion_failure(life,credit,source,ally,320,selected,1,200,{50,500}));
  auto foe=selected;foe.relation=SpellTargetRelation::foe;
  check(!a.apply_mark_subversion_failure(life,credit,source,ally,281,foe,1,200,{50,500}));
  check(a.actor(life)->health->current==1000&&a.hex_effect(life,77));
  const auto result=a.apply_mark_subversion_failure(life,credit,source,ally,281,selected,1,200,{50,500});
  check(result&&result->health.stolen==mark_subversion(rank)->amount&&result->consumed.instance==77);
  check(a.actor(life)->health->current==1000-mark_subversion(rank)->amount&&!a.hex_effect(life,77));
  check(!(a.actor(life)->effects&0x800));
  check(!a.apply_mark_subversion_failure(life,credit,source,ally,281,selected,2,200,{50,500}));
 }
 {auto p=d;p.spawn_health=Health{10,1000};LiveActor a(p,life);const auto target=a.hex_scope();
  check(a.apply_mark_subversion(life,source,*mark_subversion(12),77,100).has_value());
  const auto result=a.apply_mark_subversion_failure(life,{source,target,77,100},source,ally,281,{target,ally,SpellTargetRelation::ally},1,100,{50,500});
  check(result&&result->health.killed&&result->health.stolen==11&&a.actor(life)->health->current==0&&(a.actor(life)->effects&0x10));}
 std::cout<<"PASS scoped NPC Mark application/one-use consumption,atomic rejection,life steal and lethal state;World failure/Runtime pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

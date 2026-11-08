#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Judgment cleanup line "+std::to_string(at.line()));}
void counts(world::Instance& w,unsigned effects,unsigned retired){const auto actual=w.shield_judgment_ownership();if(actual.first!=effects||actual.second!=retired)std::cerr<<"expected="<<effects<<","<<retired<<" actual="<<actual.first<<","<<actual.second<<"\n";check(actual.first==effects&&actual.second==retired);}
int main()try{
 for(unsigned mode=0;mode<6;++mode){const bool npc=mode==1;world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id=1;id<=3;++id){w.join(id,"Actor",id==1?0:100,0,box,0,{}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);check(w.ordinary_weapon(source,combat::weapon::Owned{1,1,{20,20,2},12,false,false,27},20));check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));check(w.party_action(1,158,3,100)&&w.party_action(3,154,1,100));
  check(w.complete_shield_judgment(source,target,*skills::effects::shield_judgment(12),0,100));
  if(mode==4)check(w.complete_shield_judgment(source,*w.effect_scope(3),*skills::effects::shield_judgment(20),0,100));
  counts(w,mode==4?2:1,0);w.leave(1);counts(w,mode==4?2:1,1);
  if(mode==5){w.set_vitals(2,{1,1000,100,100},100);check(w.physical_armor(target,60));w.join(4,"Attacker",60,0,box,0,{}, {},200,2);w.set_vitals(4,{1000,1000,100,100},100);const auto attacker=*w.effect_scope(4);const combat::weapon::Owned weapon{4,1,{20,20,2},12,false,false,27};check(w.ordinary_weapon(attacker,weapon,20));check(w.player_holy_equipment(attacker,0));check(w.melee_attack(attacker,target,weapon,*combat::weapon::attack_skill(320,0),500));check(w.resources(2)->health==0&&w.resources(4)->health==959);counts(w,0,0);check(w.shield_judgment_failures()==0&&w.shield_judgment_pending_contacts()==0);continue;}
  if(mode==2){w.set_vitals(2,{0,1000,100,100},100);counts(w,0,0);continue;}
  if(mode==3){w.join(4,"New caster",0,0,box,0,{}, {},200,1);w.set_vitals(4,{500,1000,100,100},100);const auto next=*w.effect_scope(4);check(w.ordinary_weapon(next,combat::weapon::Owned{4,1,{20,20,2},12,false,false,27},20));check(w.party_action(4,158,2,200)&&w.party_action(2,154,4,200));check(w.complete_shield_judgment(next,target,*skills::effects::shield_judgment(0),0,200));counts(w,1,0);w.tick(8200);counts(w,0,0);continue;}
  w.tick(18100);counts(w,mode==4?1:0,mode==4?1:0);if(mode==4){w.tick(24100);counts(w,0,0);}
 }
 std::cout<<"PASS6 Judgment idle player/NPC expiry,death,renewal and shared retired-source ownership cleanup and sealed lethal-hit cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

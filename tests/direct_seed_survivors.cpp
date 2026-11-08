#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Direct Seed survivor line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;bool npc;effects::Scope source,target,bonded;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool is_npc,unsigned hp=500,bool conditions=true,bool friend_near=false):npc(is_npc){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health={double(npc?hp:500),1000};d.damageable=true;d.conditions_allowed=conditions;d.healing_allowed=true;d.healing_evidence="synthetic";
  d.earth_armor=d.fire_armor=d.cold_armor=60;d.fire_armor_evidence=d.cold_armor_evidence="synthetic";
  d.holy_traits.light_sensitive=false;d.light_sensitive_evidence="synthetic";d.knockdown_allowed=true;d.knockdown_evidence="synthetic";
  d.spawn_energy=combat::npcs::Energy{20,100};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:friend_near?100:700,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==1?1000u:id==2&&!npc?hp:500u,1000,id==1?100u:20u,100},100);check(w.combat_armor(*w.effect_scope(id),60));check(w.elemental_armor(*w.effect_scope(id),60,60));check(w.player_holy_equipment(*w.effect_scope(id),0));check(w.energy_capacity(id,6,0,100,{80,-2}));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);bonded=*w.effect_scope(3);box->drain();
 }
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;}
 void reversal(unsigned rank){check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,900,rank,100,8100,true));}
 bool rr(unsigned at){const auto entries=*w.status_snapshot(target,at);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});}
 unsigned cast(unsigned skill,unsigned rank=12){
  bool ok;if(skill==195||skill==247||skill==1113)ok=w.damage_field(source,*effects::damage_field(skill,rank),20,100);
  else if(skill==830)ok=w.ray_of_judgment(source,target,*effects::ray_of_judgment(rank),20,100);
  else ok=w.fire_storm(source,target,*effects::fire_storm(rank,skill),20,100);
  check(ok);return 100+effects::damage_field(skill,rank)->first_ms;
 }
 void wound(){if(npc){check(w.apply_condition(source,source,combat::conditions::Kind::deep_wound,100,10000));check(w.condition_transfer(source,target,*effects::condition_transfer(132,12),100));}else check(w.apply_condition(source,target,combat::conditions::Kind::deep_wound,100,10000));}
};
double expected(double hp,unsigned amount,int rank){const auto heal=rank<0?0u:std::min(amount,effects::reversal_fortune(rank)->maximum_healing);return std::max(0.,std::min(1000.,hp+heal)-(amount-heal));}
unsigned word(const world::Packet& p,unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);}
unsigned attack(Fixture& f,unsigned skill){
 if(skill!=33&&skill!=3181){const auto at=f.cast(skill);f.w.tick(at);return at;}
 skills::Slots bar{};bar[0]={skill,0};check(f.w.configure_adrenaline(f.source,bar,100));check(f.w.status_mutation(f.source,skill,123,12,100,30100,true));combat::weapon::Owned axe{12,1,{20,20,2},12,false,false,2};check(f.w.ordinary_weapon(f.source,axe,20));check(f.w.melee_attack(f.source,f.target,axe,*combat::weapon::attack_skill(340,12),200));return 200;
}
unsigned damage(unsigned skill){return skill==33||skill==3181?effects::illusionary_weaponry(skill,12)->damage:effects::damage_field(skill,12)->damage;}
int main()try{
 unsigned cases=0;
 for(unsigned skill:{187u,192u,167u,215u,197u,195u,1094u,247u,1113u,830u,33u,3181u})for(unsigned hp:{5u,10u,500u,1000u})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u}){
  ++cases;Fixture f(false,hp,true,true);check(f.w.party_action(2,158,3,100)&&f.w.party_action(3,154,2,100));check(f.w.complete_healing_seed(f.bonded,f.target,*effects::healing_seed(rank),0,100));if(rr>=0)f.reversal(rr);f.box->drain();const auto at=attack(f,skill);const auto raw=damage(skill);const auto converted=rr<0?0u:std::min(raw,effects::reversal_fortune(rr)->maximum_healing);const auto healed=std::min(1000.,hp+double(converted));const auto loss=std::min(healed,double(raw-converted));const auto seed=effects::healing_seed(rank)->healing;double wanted=healed-loss;if(wanted>0&&loss>0)wanted=std::min(1000.,wanted+seed);
  if(std::abs(f.hp()-wanted)>=1.01)throw std::runtime_error("skill="+std::to_string(skill)+" hp="+std::to_string(hp)+" rr="+std::to_string(rr)+" seed_rank="+std::to_string(rank)+" actual="+std::to_string(f.hp())+" expected="+std::to_string(wanted));
  const auto ally=500+(loss>0?seed:0)-(skill==33||skill==3181?0:raw);check(f.w.resources(3)->health==ally);if(!wanted)check(f.w.player_death_times({2,3})[0]==at);
  const auto packets=f.box->drain();unsigned first_loss=unsigned(packets.size()),first_gain=first_loss,gains=0;for(unsigned i=0;i<packets.size();++i){const auto& p=packets[i];if(p.size()!=18||p[0]!=162)continue;const auto v=std::bit_cast<float>(word(p,14));if(word(p,6)==2&&v<0)first_loss=std::min(first_loss,i);if(word(p,6)==3&&v>0){first_gain=std::min(first_gain,i);++gains;}}
  check(gains==unsigned(loss>0));if(loss>0)check(first_loss<first_gain);
 }
 // Real healing can settle lethal Deep Wound debt on a party recipient.
 for(unsigned skill:{197u,187u,167u,830u,33u,3181u})for(unsigned rank:{0u,12u,20u}){
  ++cases;Fixture f(false,500,true,true);check(f.w.party_action(2,158,3,100)&&f.w.party_action(3,154,2,100));check(f.w.complete_healing_seed(f.bonded,f.target,*effects::healing_seed(rank),0,100));f.w.set_vitals(3,{10,1000,100,100},100);check(f.w.apply_condition(f.source,f.bonded,combat::conditions::Kind::deep_wound,100,10000));const auto before=*f.w.control_epoch(f.bonded);const auto at=attack(f,skill);check(f.w.resources(3)->health==0&&f.w.player_death_times({3,2})[0]==at&&*f.w.control_epoch(f.bonded)==before+1);f.w.tick(at+100);check(f.w.resources(3)->health==0&&*f.w.control_epoch(f.bonded)==before+1);
 }
 check(cases==594);std::cout<<"PASS "<<cases<<" direct Seed survivor/last-hit/conversion/packet-order and healing-death cleanup cases;rendered timing and NPC damaged triggers remain open\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

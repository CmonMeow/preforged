#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Spiteful Seed line "+std::to_string(at.line()));
}
static unsigned word(const world::Packet& p,unsigned offset){unsigned v=0;for(unsigned i=0;i<4;++i)v|=unsigned(p[offset+i])<<(8*i);return v;}
struct Fixture{
 world::Instance w;effects::Scope foe,owner,target,monk;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(unsigned hp,unsigned rank,bool unknown=false){
  if(unknown){world::Instance::Npc n;n.id=n.definition.id=100;n.x=210;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;
   d.spawn_health={500,1000};d.damageable=true;n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Actor",id==1?1000.f:id==2?0.f:id==3?60.f:210.f,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?1000u:id==3?hp:500u,1000,100,100},100);}
  foe=*w.effect_scope(1);owner=*w.effect_scope(2);target=*w.effect_scope(3);monk=*w.effect_scope(4);
  check(w.party_action(3,158,4,100)&&w.party_action(4,154,3,100));
  check(w.complete_healing_seed(monk,target,*effects::healing_seed(rank),0,100));
  check(w.spiteful_spirit(foe,owner,*effects::spiteful_spirit(12),20,100));
 }
 void rr(int rank){if(rank>=0)check(w.status_mutation(target,307,900,rank,100,8100,true));}
 void source_state(unsigned state){if(state==1)check(w.kill(foe,100));if(state>=2)w.leave(1);if(state==3){w.join(1,"Replacement",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);}}
 effects::SpitefulAction action(bool completion)const{return {owner,completion?effects::SpitefulPhase::skill_completed:effects::SpitefulPhase::attack_release,completion?1239u:0u,1,200,true};}
};
#include "spiteful_seed_death_cases.inc"
int main()try{
 unsigned cases=0;const auto damage=effects::spiteful_spirit(12)->damage;
 for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{1u,10u,500u,1000u})for(unsigned state:{0u,1u,2u,3u})for(bool completion:{false,true}){
  Fixture f(hp,rank);f.rr(rr);f.source_state(state);f.box->drain();std::string detail;check(f.w.spiteful_action(f.action(completion),&detail)&&detail=="applied");
  const auto converted=rr<0?0u:std::min(damage,effects::reversal_fortune(rr)->maximum_healing);
  const double healed=std::min(1000.,hp+double(converted)),loss=std::min(healed,double(damage-converted));
  const bool triggered=loss>0;const auto seed=effects::healing_seed(rank)->healing;
  const double wanted=healed-loss>0?std::min(1000.,healed-loss+(triggered?seed:0)):0;
  check(std::abs(double(f.w.resources(3)->health)-wanted)<1.01);
  check(f.w.resources(2)->health==500-damage+(triggered?seed:0));
  check(f.w.resources(4)->health==500+(triggered?seed:0));
  const auto wire=f.box->drain();unsigned gains=0,first_gain=unsigned(wire.size()),last_loss=0;
  for(unsigned i=0;i<wire.size();++i){const auto& p=wire[i];if(p.size()!=18||p[0]!=162)continue;
   const auto value=std::bit_cast<float>(word(p,14));
   if(value<0)last_loss=i;
   if(value>0&&word(p,10)==4){++gains;first_gain=std::min(first_gain,i);}}
  check(gains==(triggered?(wanted>0?3u:2u):0u));if(triggered&&state<2)check(last_loss<first_gain);
  check(!f.w.spiteful_action(f.action(completion)));f.w.tick(201);check(f.w.resources(3)->health==unsigned(wanted));
  if(!wanted)check(f.w.player_death_times({3,2})[0]==200);++cases;
 }
 // Two seeded area recipients each react once, after both base impacts.
 for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20}){
  Fixture f(500,rank);f.rr(rr);check(f.w.complete_healing_seed(f.monk,f.owner,*effects::healing_seed(rank),0,100));
  check(f.w.spiteful_action(f.action(false)));const auto converted=rr<0?0u:std::min(damage,effects::reversal_fortune(rr)->maximum_healing);
  const auto seed=effects::healing_seed(rank)->healing;const bool second=damage>converted;
  check(f.w.resources(2)->health==500-damage+seed+(second?seed:0));
  check(f.w.resources(3)->health==500+converted-(damage-converted)+seed+(second?seed:0));
  check(f.w.resources(4)->health==500+(second?seed:0));++cases;
 }
 // Missing heal eligibility rejects before spending the area action or Reversal.
 for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0}){
  Fixture f(500,rank,true);f.rr(rr);f.box->drain();std::string detail;
  check(!f.w.spiteful_action(f.action(false),&detail));check(detail=="player_seed");
  check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500&&f.w.resources(4)->health==500&&f.box->drain().empty());
  check(f.w.status_mutation(f.target,307,901,20,200,8200,true));
  check(f.w.spiteful_action(f.action(false)));check(f.w.resources(2)->health==500-damage&&f.w.resources(3)->health==500+damage&&f.w.resources(4)->health==500);++cases;
 }
 cases+=spiteful_seed_death_cases();check(cases==444);std::cout<<"PASS "<<cases<<" Spiteful Seed area/conversion/lethal/deduplication/source-lifetime/preflight cases; synthetic mechanics, no rendered proof\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

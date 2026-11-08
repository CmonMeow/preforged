#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Single target Seed line "+std::to_string(at.line()));
}
static unsigned word(const world::Packet& p,unsigned at){unsigned v=0;for(unsigned i=0;i<4;++i)v|=unsigned(p[at+i])<<(i*8);return v;}
struct Fixture{
 world::Instance w;effects::Scope source,target,monk;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();std::vector<world::Packet> packets;
 Fixture(unsigned hp,unsigned seed_rank,bool unknown=false){
  if(unknown){world::Instance::Npc n;n.id=n.definition.id=100;n.x=90;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0.f:id==2?50.f:100.f,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==1?1000u:id==2?hp:500u,1000,100,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(2);monk=*w.effect_scope(3);
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
  check(w.complete_healing_seed(monk,target,*effects::healing_seed(seed_rank),0,100));box->drain();
 }
 void rr(int rank){if(rank>=0)check(w.status_mutation(target,307,900,rank,100,8100,true));}
 void blind(){check(w.apply_condition(source,target,combat::conditions::Kind::blind,100,10000));}
 bool cast(unsigned skill,unsigned rank,bool deferred){
  box->drain();std::vector<world::Packet> visuals;std::function<void()> publication;bool accepted=false;
  if(skill==817||skill==2863)accepted=w.discord(source,target,*effects::discord(rank,skill==2863),20,200,&visuals,deferred?&publication:nullptr);
  else if(skill==876)accepted=w.signet_shadows(source,target,*effects::signet_shadows(rank),20,200,&visuals,deferred?&publication:nullptr);
  else{const auto result=w.complete_signet_sorrow(source,target,*effects::signet_sorrow(rank),20,200);accepted=result.accepted;publication=result.publication;}
  if(publication){check(box->drain().empty());publication();packets=box->drain();publication();check(box->drain().empty());}
  else packets=box->drain();
  return accepted;
 }
};
static double amount(unsigned skill,unsigned rank,bool blind){
 if(skill==817||skill==2863)return blind?effects::discord(rank,skill==2863)->damage:0;
 if(skill==876){const auto p=*effects::signet_shadows(rank);return p.damage+(blind?p.blind_bonus:0);}
 return effects::signet_sorrow(rank)->damage;
}
int main()try{
 unsigned cases=0;
 for(unsigned skill:{817u,2863u,876u,1363u})for(bool blind:{false,true})for(int rr:{-1,0,12,20})for(unsigned hp:{1u,10u,500u,1000u})for(unsigned seed_rank:{0u,12u,20u})for(unsigned rank:{0u,12u,20u})for(bool deferred:{false,true}){
  Fixture f(hp,seed_rank);if(blind)f.blind();f.rr(rr);check(f.cast(skill,rank,deferred));
  const auto damage=amount(skill,rank,blind),converted=rr<0?0.:std::min(damage,double(effects::reversal_fortune(rr)->maximum_healing));
  const auto healed=std::min(1000.,hp+converted),loss=std::min(healed,damage-converted);const bool triggered=loss>0;
  const auto healing=effects::healing_seed(seed_rank)->healing;const auto wanted=healed-loss>0?std::min(1000.,healed-loss+(triggered?healing:0)):0.;
  if(std::abs(double(f.w.resources(2)->health)-wanted)>=1.01)throw std::runtime_error("skill="+std::to_string(skill)+" actual="+std::to_string(f.w.resources(2)->health)+" wanted="+std::to_string(wanted));
  check(f.w.resources(3)->health==500+(triggered?healing:0)&&f.w.resources(1)->health==1000);
  unsigned gains=0,first_gain=unsigned(f.packets.size()),last_loss=0;
  for(unsigned i=0;i<f.packets.size();++i){const auto& p=f.packets[i];if(p.size()!=18||p[0]!=162)continue;const auto value=std::bit_cast<float>(word(p,14));
   if(value<0)last_loss=i;if(value>0&&word(p,10)==3){++gains;first_gain=std::min(first_gain,i);}}
  check(gains==(triggered?(wanted>0?2u:1u):0u));if(triggered)check(last_loss<first_gain);
  const auto status=*f.w.status_snapshot(f.target,200);check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;})==bool(rr>=0&&!damage));
  if(!wanted)check(f.w.player_death_times({2,3})[0]==200);f.w.tick(201);check(f.w.resources(3)->health==500+(triggered?healing:0));++cases;
 }
 // Unknown nearby NPC healing rules fail before damage/output or conversion consumption.
 for(unsigned skill:{817u,2863u,876u,1363u})for(unsigned seed_rank:{0u,12u,20u})for(int rr:{-1,0})for(bool deferred:{false,true}){
  Fixture f(500,seed_rank,true);f.blind();f.rr(rr);check(!f.cast(skill,12,deferred));
  check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500&&f.packets.empty());
  const auto status=*f.w.status_snapshot(f.target,200);check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;})==bool(rr>=0));
  check(f.w.status_mutation(f.target,307,901,20,200,8200,true));check(f.cast(skill,12,deferred));
  check(f.w.resources(2)->health==500+amount(skill,12,true)&&f.w.resources(3)->health==500);++cases;
 }
 // A conversion-healing death has no damage receipt and cannot trigger Seed.
 for(unsigned skill:{817u,2863u,876u,1363u})for(unsigned seed_rank:{0u,12u,20u})for(int rr:{0,12,20})for(bool deferred:{false,true}){
  Fixture f(10,seed_rank);f.blind();f.rr(rr);check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));const auto epoch=*f.w.control_epoch(f.target);
  check(f.cast(skill,12,deferred));check(f.w.resources(2)->health==0&&f.w.resources(3)->health==500&&*f.w.control_epoch(f.target)==epoch+1);f.w.tick(201);check(*f.w.control_epoch(f.target)==epoch+1);++cases;
 }
 // Deferred signet/spell output must retain cleanup when the Seed heal kills an ally.
 for(unsigned skill:{817u,2863u,876u,1363u})for(unsigned seed_rank:{0u,12u,20u})for(bool deferred:{false,true}){
  Fixture f(500,seed_rank);f.blind();f.w.set_vitals(3,{10,1000,100,100},100);check(f.w.apply_condition(f.source,f.monk,combat::conditions::Kind::deep_wound,100,10000));const auto epoch=*f.w.control_epoch(f.monk);
  check(f.cast(skill,12,deferred));check(f.w.resources(3)->health==0&&*f.w.control_epoch(f.monk)==epoch+1&&f.w.player_death_times({3,2})[0]==200);f.w.tick(201);check(*f.w.control_epoch(f.monk)==epoch+1);++cases;
 }
 check(cases==2448);std::cout<<"PASS "<<cases<<" Discord PvE/PvP, Shadows and Sorrow Seed cases: predicates, blindness bonus, conversion/survival, deferred output, failed preflight and death cleanup; no rendered proof\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

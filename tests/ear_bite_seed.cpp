#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ear Bite Seed line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& p,unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);}
struct Fixture{
 world::Instance w;effects::Scope foe,target,monk;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(unsigned hp,unsigned seed_rank,int armor=60,bool far=false,bool unknown=false){
  if(unknown){world::Instance::Npc n;n.id=n.definition.id=100;n.x=80;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;
   d.healing_allowed=std::nullopt;d.creature_type="spirit";d.creature_type_evidence="authored unsupported Seed gain recipient";d.fleshy=false;d.fleshy_evidence="authored non-fleshy spirit";n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",id==1?0:id==2?60:far?700:100,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?1000u:id==2?hp:500u,1000,100,100},100);
   check(w.physical_armor(*w.effect_scope(id),armor));
  }
  foe=*w.effect_scope(1);target=*w.effect_scope(2);monk=*w.effect_scope(3);
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
  check(w.complete_healing_seed(monk,target,*effects::healing_seed(seed_rank),0,100));
 }
 void reversal(int rank){if(rank>=0)check(w.status_mutation(target,307,900,rank,100,8100,true));}
 bool has_reversal(unsigned at=200){const auto entries=*w.status_snapshot(target,at);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});}
 bool cast(unsigned rank,bool deferred,unsigned at=200){
  std::function<void()> publish;box->drain();const bool ok=w.ear_bite(foe,target,*effects::ear_bite(rank),20,at,deferred?&publish:nullptr);
  if(ok&&deferred){check(box->drain().empty()&&bool(publish));publish();}
  if(ok){const auto packets=box->drain();unsigned loss=unsigned(packets.size()),heal=loss,heals=0;
   for(unsigned i=0;i<packets.size();++i){const auto& p=packets[i];if(p.size()!=18||p[0]!=162)continue;
    const auto amount=std::bit_cast<float>(word(p,14));if(word(p,6)==2&&amount<0)loss=std::min(loss,i);
    if(word(p,6)==3&&amount>0){heal=std::min(heal,i);++heals;}}
   if(heals)check(heals==1&&loss<heal);
   if(deferred){publish();check(box->drain().empty());}
  }else check(!publish&&box->drain().empty());
  return ok;
 }
 unsigned hp(){return w.resources(2)->health;}
};
int main()try{
 unsigned cases=0;
 for(unsigned rank:{0u,5u,10u})for(int armor:{40,60,100})for(int rr:{-1,0,12,20})
 for(unsigned hp:{10u,500u,1000u})for(unsigned seed_rank:{0u,12u,20u})for(bool deferred:{false,true}){
  ++cases;Fixture f(hp,seed_rank,armor);f.reversal(rr);
  const auto raw=effects::ear_bite(rank)->damage*std::exp2((60.-armor)/40.);
  const auto converted=rr<0?0.:std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));
  const auto before=std::min(1000.,hp+converted),loss=std::min(before,raw-converted);
  const bool trigger=loss>0;const auto healing=effects::healing_seed(seed_rank)->healing;
  auto wanted=before-loss;if(wanted>0&&trigger)wanted=std::min(1000.,wanted+healing);
  if(!f.cast(rank,deferred)||std::abs(double(f.hp())-wanted)>=1.01)
   throw std::runtime_error("matrix rank="+std::to_string(rank)+" armor="+std::to_string(armor)+" rr="+std::to_string(rr)+" hp="+std::to_string(hp));
  check(f.w.resources(3)->health==500+(trigger?healing:0));check(!f.has_reversal());
  check(f.w.has_condition(f.target,combat::conditions::Kind::bleeding,200)==(wanted>0));
  if(!wanted)check(f.w.player_death_times({2,3})[0]==200);
 }
 // An explicit spirit missing required gain facts rejects the complete damage/condition
 // transaction before Reversal is consumed. Fully converted hits need no Seed.
 for(int rr:{-1,0,12,20})for(bool deferred:{false,true}){
  ++cases;Fixture f(500,12,60,false,true);f.reversal(rr);
  const bool loss=rr<0||60>effects::reversal_fortune(rr)->maximum_healing;
  check(f.cast(5,deferred)==!loss);check(f.hp()==(loss?500:560));
  check(f.w.resources(3)->health==500&&f.has_reversal()==(rr>=0&&loss));
  check(f.w.has_condition(f.target,combat::conditions::Kind::bleeding,200)==!loss);
 }
 for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u}){
  ++cases;Fixture f(500,rank,60,true);f.reversal(rr);check(f.cast(5,true));check(f.w.resources(3)->health==500);
 }
 // Deep Wound can turn Reversal healing into death. No artificial damage or
 // Seed trigger, and Bleeding must not be added after that death.
 for(unsigned rr:{0u,12u,20u})for(bool deferred:{false,true}){
  ++cases;Fixture f(10,12);check(f.w.apply_condition(f.foe,f.target,combat::conditions::Kind::deep_wound,100,10000));
  f.reversal(rr);check(f.cast(5,deferred));check(f.hp()==0&&!f.has_reversal()&&f.w.resources(3)->health==500);
  check(!f.w.has_condition(f.target,combat::conditions::Kind::bleeding,200));
 }
 // Bleeding degeneration is Health loss, not a second direct damage trigger.
 for(unsigned rank:{0u,12u,20u}){
  ++cases;Fixture f(500,rank);check(f.cast(5,true));const auto hp=f.hp(),ally=f.w.resources(3)->health;
  f.w.tick(1200);check(f.hp()==hp-6&&f.w.resources(3)->health==ally);
 }
 check(cases==677);std::cout<<"PASS "<<cases<<" Ear Bite Healing Seed armor/conversion/survival/publication/failure cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

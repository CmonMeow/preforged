#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;namespace casting=skills::casting;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Breath line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,ally,foe,npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();std::vector<world::Packet> feedback;
 Fixture(unsigned hp=500,std::optional<bool> healing=true,float distance=60,bool member=true){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=distance;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={double(hp),1000};d.damageable=true;d.conditions_allowed=true;
  d.healing_allowed=healing;d.healing_evidence=healing.has_value()?"fixture":"-";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:distance,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);
   w.set_vitals(id,{hp,1000,100,100},100);}
  source=*w.effect_scope(1);ally=*w.effect_scope(2);foe=*w.effect_scope(3);npc=*w.effect_scope(100);
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  if(member)check(w.set_npc_party_owner(npc,source));
 }
 void burn(effects::Scope target){
  check(w.apply_condition(foe,target,combat::conditions::Kind::burning,100,10000));
 }
 bool cast(unsigned rank,unsigned divine=0,unsigned now=100,bool deferred=true){
  std::function<void()> publish;box->drain();const bool ok=w.party_heal(source,*effects::party_heal(2221,rank,divine),now,{},deferred?&publish:nullptr);
  if(ok&&deferred){check(box->drain().empty()&&bool(publish));publish();feedback=box->drain();check(!feedback.empty());publish();check(box->drain().empty());}
  if(ok&&!deferred)feedback=box->drain();if(!ok)check(!publish&&box->drain().empty());return ok;
 }
};
int main()try{
 unsigned cases=0;
 for(unsigned rank=0;rank<=10;++rank)for(unsigned hp:{10u,500u,990u,1000u})for(bool burning:{false,true})for(bool deferred:{false,true}){
  ++cases;Fixture f(hp);if(burning)for(auto target:{f.source,f.ally,f.npc})f.burn(target);
  const auto heal=50+2*std::min(rank,5u);check(f.cast(rank,0,100,deferred));
  check(f.w.resources(1)->health==std::min(1000u,hp+heal)&&f.w.resources(2)->health==std::min(1000u,hp+heal));
  check(f.w.npc_combat_snapshot(f.npc)->health->current==std::min(1000u,hp+heal)&&f.w.resources(3)->health==hp);
  unsigned gains=0;for(const auto& packet:f.feedback){if(packet.size()!=18||packet[0]!=162)continue;
   const auto word=[&](unsigned at){return unsigned(packet[at])|(unsigned(packet[at+1])<<8)|(unsigned(packet[at+2])<<16)|(unsigned(packet[at+3])<<24);};
   if(word(6)==2&&std::bit_cast<float>(word(14))>0){++gains;check(std::abs(std::bit_cast<float>(word(14))-std::min(heal,1000-hp)/1000.f)<.000001);}}
  check(gains==unsigned(hp<1000));
  for(auto target:{f.source,f.ally,f.npc})check(!f.w.has_condition(target,combat::conditions::Kind::burning,100));
 }
 for(unsigned rank:{0u,5u,10u})for(unsigned divine:{0u,12u,20u}){
  ++cases;Fixture f;f.w.public_professions(1,3,6);check(f.cast(rank,divine));
  check(f.w.resources(1)->health==500+effects::recipient_heal(*effects::party_heal(2221,rank,divine),true));
  check(f.w.resources(2)->health==500+effects::party_heal(2221,rank,0)->base_heal);
 }
 for(unsigned mode=0;mode<5;++mode){
  ++cases;Fixture f(500,mode==0?std::nullopt:std::optional(mode!=1),mode==2?5001:60,mode!=3);f.burn(f.source);f.burn(f.npc);
  check(f.cast(5)==(mode!=0));
  if(mode==0){check(f.w.resources(1)->health==500&&f.w.has_condition(f.source,combat::conditions::Kind::burning,100));}
  else if(mode==1){check(f.w.npc_combat_snapshot(f.npc)->health->current==500&&!f.w.has_condition(f.npc,combat::conditions::Kind::burning,100));}
  else if(mode==2||mode==3){check(f.w.npc_combat_snapshot(f.npc)->health->current==500&&f.w.has_condition(f.npc,combat::conditions::Kind::burning,100));}
 }
 // Burning removal does not erase another condition. Degeneration is settled
 // before this completion; healing a dead party member cannot resurrect them.
 for(unsigned mode=0;mode<3;++mode){
  ++cases;Fixture f;f.burn(f.ally);check(f.w.apply_condition(f.foe,f.ally,combat::conditions::Kind::poison,100,10000));
  if(mode==0)check(f.w.kill(f.ally,100));if(mode==1)f.w.leave(2);
  check(f.cast(5));if(mode==2)check(f.w.has_condition(f.ally,combat::conditions::Kind::poison,100));
  if(mode==0)check(f.w.resources(2)->health==0);
 }
 // Normal paid Runtime path, completion receipt, cancellation, recycled source,
 // missing hook/rank and too-low Energy. Publication follows finish and caster FX.
 for(unsigned rank:{0u,5u,10u})for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<8;++mode){
  ++cases;Fixture f;f.burn(f.source);f.burn(f.ally);f.w.public_professions(1,6,3);
  if(version){const auto effect=*effects::ether_renewal(version,15);check(f.w.status_mutation(f.source,version,80,15,100,100+effect.duration,true));}
  casting::Runtime runtime(1);casting::Hooks h;unsigned commits=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
  h.consume_energy_at=[&](auto n,auto at){return f.w.consume_energy(1,n,f.source,at);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.target_valid=[&](auto target,auto range,auto self){return f.w.valid_heal_target(1,target,range,self);};
  h.before_spell_effect=[&](auto id,auto event,auto at){return f.w.before_spell_effect(f.source,id,event,at);};
  h.deldrimor_rank=[&]()->std::optional<unsigned>{return rank;};
  h.party_heal_published=[&](const auto& plan,auto event,auto at)->casting::EffectCompletion{
   ++commits;std::function<void()> publication;const bool ok=f.w.party_heal(f.source,plan,at,event,&publication);
   if(ok)check(!f.w.party_heal(f.source,plan,at,event));return {ok,{},std::nullopt,std::move(publication)};};
  if(mode==4)h.party_heal_published={};if(mode==5)h.deldrimor_rank={};
  if(mode==6)h.deldrimor_rank=[](){return std::optional(11u);};
  if(mode==7)f.w.set_vitals(1,{500,1000,9,100},100);
  skills::Slots slots{};slots[0]={2221,0};skills::Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[2221]=1;attributes::Ranks ranks{};
  const auto admission=runtime.start(100,{2221,0,3},slots,policy,ranks,true,h);
  check((admission==casting::Reason::accepted)==(mode<4));
  if(mode>=4){check(commits==0);continue;}
  check(f.w.resources(1)->energy==90&&commits==0);
  if(mode==1)runtime.cancel(h);if(mode==2){f.w.leave(1);f.w.join(1,"Replacement",0,0,f.box,0,{}, {},200,1);f.w.set_vitals(1,{500,1000,100,100},100);}
  if(mode==3)h.party_heal_published={};runtime.update(349,h);check(commits==0);f.box->drain();runtime.update(350,h);
  check(commits==unsigned(mode==0));
  if(mode==0){check(f.w.resources(1)->health==497+effects::party_heal(2221,rank,0)->base_heal+(version?effects::ether_renewal(version,15)->healing_per_enchantment:0)&&!f.w.has_condition(f.source,combat::conditions::Kind::burning,350));
   check(f.w.resources(1)->energy==90+(version?effects::ether_renewal(version,15)->energy_per_enchantment:0));
   const auto packets=f.box->drain();const auto finished=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
   const auto body=casting::skill_success_body_effect(1,2221);check(body.has_value());const auto visual=std::find(packets.begin(),packets.end(),*body);
   check(finished!=packets.end()&&visual!=packets.end()&&finished<visual&&visual+1!=packets.end());
   runtime.update(1100,h);check(runtime.start(1100,{2221,0,1},slots,policy,ranks,true,h)==casting::Reason::recharge);}
 }
 // Removal-triggered Fragility can fire Seed and heal another planned NPC.
 // The later party heal must rebuild that recipient,not overwrite the reaction.
 for(int rr:{-1,0,20})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f;f.w.set_vitals(2,{hp,1000,100,100},100);f.burn(f.ally);
  check(f.w.degeneration_hex(f.foe,f.ally,*effects::degeneration_hex(19,12),100));
  check(f.w.complete_healing_seed(f.source,f.ally,*effects::healing_seed(12),0,100));
  if(rr>=0)check(f.w.status_mutation(f.ally,307,900,rr,100,8100,true));
  const auto damage=effects::fragility(19,12)->damage;
  const auto converted=rr<0?0u:std::min(damage,effects::reversal_fortune(rr)->maximum_healing);
  const auto before=std::min(1000u,hp+converted),loss=std::min(before,damage-converted);
  const auto seed=loss?effects::healing_seed(12)->healing:0u;
  auto wanted=before-loss;if(wanted)wanted=std::min(1000u,wanted+seed+60);
  check(f.cast(5));check(f.w.resources(2)->health==wanted);
  check(f.w.resources(1)->health==560+seed&&f.w.npc_combat_snapshot(f.npc)->health->current==560+seed);
 }
 for(unsigned rank:{0u,5u,10u}){
  ++cases;Fixture f(10);check(f.w.apply_condition(f.foe,f.npc,combat::conditions::Kind::deep_wound,100,10000));
  check(f.cast(rank));check(f.w.npc_combat_snapshot(f.npc)->health->current==0);
 }
 auto invalid=*effects::party_heal(2221,0,0);invalid.remove_burning=false;Fixture f;check(!f.w.party_heal(f.source,invalid,100));++cases;
 check(!effects::party_heal(2221,11,0));check(cases==278);std::cout<<"PASS "<<cases<<" Breath of the Great Dwarf rank/party/cleanse/healing/immunity/lifetime/paid-cast cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

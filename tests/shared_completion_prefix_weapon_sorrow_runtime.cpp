// Actual Runtime -> World owner -> strict manual producer -> native World
// skill_event route. No fabricated success receipt or client-render claim.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include "gw_completion_publication.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;namespace effects=skills::effects;
static void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Weapon/Sorrow prefix Runtime line "+std::to_string(at.line()));}
int main()try{
 for(unsigned skill:{793u,1363u})for(bool refuse:{false,true}){
  auto storage=std::make_unique<world::Instance>();auto& w=*storage;
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{100,100};d.damageable=true;n.combat_definition=d;w.configure_npcs({n});
  auto box=std::make_shared<world::Outbox>();const unsigned primary=skill==793?8u:4u;
  w.join(1,"Prefix caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);w.public_professions(1,primary,0);
  w.join(2,"Prefix ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{500,1000,100,100},100);
  const auto owner=*w.effect_scope(1),ally=*w.effect_scope(2),npc=*w.effect_scope(100);
  check(w.energy_capacity(1,primary,0,100,{80,-2}));check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto vigor=*effects::vigorous_spirit(0);if(skill==793)check(w.complete_vigorous_spirit(owner,owner,vigor,0,100));
  Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};check(w.configure_adrenaline(owner,bar,100));
  Policy policy;policy.primary=static_cast<unsigned char>(primary);policy.campaigns=255;policy.learned[skill]=1;
  attributes::Ranks ranks{};ranks[6]=ranks[33]=ranks[34]=ranks[36]=12;
  casting::Runtime runtime(1);casting::Hooks h;std::uint64_t at=1000;unsigned payments=0,paid=0,calls=0,accepted=0,observed=0;
  std::function<void()> retained;
  h.events=[&](const auto& packets){w.skill_event(owner.actor,packets,at);};
  h.resources=[&](auto actor){return w.resources(actor);};h.target_scope=[&](auto actor){return w.effect_scope(actor);};
  h.consume_energy_at=[&](auto cost,auto now){++payments;paid+=cost;return w.consume_energy(owner.actor,cost,owner,now);};
  h.control_epoch=[&]{return w.control_epoch(owner);};
  h.cast_slot_activity=[&](auto slot,auto begins,auto ends){check(w.cast_activity(owner,slot,begins,ends));};
  h.before_spell_effect=[&](auto id,auto event,auto now){return w.before_spell_effect(owner,id,event,now);};
  h.effect_snapshot=[&](auto now){return w.status_snapshot(owner,now);};
  h.prepare_skill_use=[&](auto slot,auto event,auto now){return w.prepare_diversion_skill_use(owner,slot,event,now);};
  h.finalize_skill_use=[&](auto event,auto now,auto ordinary,auto existing,bool instant){return w.finalize_diversion_skill_use(owner,event,now,ordinary,existing,instant);};
  h.abort_skill_use=[&](auto event){return w.abort_diversion_skill_use(owner,event);};
  h.cooldown_completion_source=[&](auto event){return w.cooldown_completion_stamp(owner,event);};
  h.cooldown_control_source=[&](auto after){return w.cooldown_control_source(owner,after);};
  h.skill_disabled_slot=[&](auto slot,auto now){return w.skill_disabled_slot(owner,slot,now);};
  h.current_skillbar=[&]()->std::optional<Slots>{return bar;};
  h.completed_skill_use=[&](const effects::CooldownCompletion& receipt){const auto actual=w.cooldown_completion_stamp(owner,receipt.skill_use);check(actual&&*actual==receipt);++observed;return true;};
  h.prepare_completion_publication=[&](auto scope,auto now){check(scope==owner);return w.prepare_completion_publication(scope,now);};
  h.completed_action=[&](auto id,auto now)->casting::ActionCompletion{casting::ActionCompletion result;result.accepted=w.completed_skill_action(owner,id,now,nullptr,&result.publication);return result;};
  auto stale=owner;++stale.generation;
  if(skill==793){
   h.weapon_spell_target_valid=[&](auto target,const auto& plan,auto now){return w.valid_weapon_spell_target(owner,target,plan,now);};
   h.weapon_spell=[&](auto target,const auto& plan,auto now)->casting::EffectCompletion{
    ++calls;std::vector<world::Packet> visuals;std::function<void()> publication;
    const bool ok=w.apply_weapon_spell(refuse?stale:owner,target,plan,now,&visuals,&publication);accepted+=ok;retained=publication;
    return {ok,std::move(visuals),{},std::move(publication),true};
   };
  }else{
   h.projectile_target_valid=[&](auto actor,float range){return w.projectile_target_valid(owner,actor,range,at);};
   h.signet_sorrow=[&](auto target,const auto& plan,auto now)->casting::SignetSorrowCompletion{
    ++calls;auto result=w.complete_signet_sorrow(refuse?stale:owner,target,plan,20,now);accepted+=result.accepted;retained=result.publication;
    return {{result.accepted,std::move(result.visuals),{},std::move(result.publication),true},result.recharge_ms};
   };
  }
  box->drain();const auto target=skill==793?ally:npc;
  check(runtime.start(at,{skill,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);box->drain();
  at=1999;runtime.update(at,h);check(calls==0);box->drain();at=2000;runtime.update(at,h);
  const auto packets=box->drain();check(calls==1&&accepted==unsigned(!refuse)&&payments==1&&paid==(skill==793?10u:0u));
  const auto finish=casting::terminal(58,owner.actor),stop=casting::terminal(59,owner.actor);
  if(refuse){
   check(std::count(packets.begin(),packets.end(),finish)==0&&std::count(packets.begin(),packets.end(),stop)==1);
   if(skill==793){const auto status=w.status_snapshot(ally,at);check(status&&status->empty());}
   else check(w.npc_combat_snapshot(npc)->health->current==100);
   check(w.resources(owner.actor)->health==500);
  }else{
   check(observed==1&&std::count(packets.begin(),packets.end(),finish)==1&&bool(retained));
   const auto begin=std::find(packets.begin(),packets.end(),finish);check(begin!=packets.end());
   if(skill==793){
    const auto* row=find_skill(skill);check(row);
    const auto body=std::find(packets.begin(),packets.end(),casting::generic(21,ally.actor,row->target_body_resource));
    const auto heal=std::find(packets.begin(),packets.end(),particles::health_change_2022(owner.actor,owner.actor,vigor.healing,1000));
    check(body!=packets.end()&&heal!=packets.end()&&begin<body&&body<heal);
    check(std::count(packets.begin(),packets.end(),casting::generic(21,ally.actor,1155))==1&&w.resources(owner.actor)->health==500+vigor.healing);
   }else{
    const auto body=std::find(packets.begin(),packets.end(),casting::generic(21,owner.actor,1619));
    const auto capacity=std::find(packets.begin(),packets.end(),combat::detail::integer(42,npc.actor,100));
    const auto damage=std::find(packets.begin(),packets.end(),combat::detail::integer(10,owner.actor,skill));
    check(body!=packets.end()&&capacity!=packets.end()&&damage!=packets.end()&&begin<body&&body<capacity&&capacity<damage);
    check(w.npc_combat_snapshot(npc)->health->current==37);
   }
   retained();retained();check(box->drain().empty());
  }
  runtime.update(at,h);runtime.update(at+1,h);check(calls==1&&payments==1&&box->drain().empty());
 }
 std::cout<<"PASS actual paid Weapon793/Sorrow1363 owner/native prefix, strict deferred queue, Vigor tail, refusal and once replay\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

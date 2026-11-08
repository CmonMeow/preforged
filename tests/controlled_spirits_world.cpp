#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("controlled spirits World line "+std::to_string(at.line()));
}
static unsigned spawns(const std::vector<world::Packet>& packets){
 return unsigned(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=6&&p[0]==32&&p[1]==0;}));
}
static std::vector<world::Instance::Npc> templates(unsigned level){
 std::vector<world::Instance::Npc> result;
 for(unsigned i=0;i<3;++i){
  world::Instance::Npc n;n.definition.id=400+i;n.definition.model_file=1234;
  n.definition.visual_adjustment={0,0,0,100};n.definition.level=std::uint8_t(level);
  n.x=float(i*40);n.y=0;
  combat::npcs::Definition d;d.binding.map=146;d.binding.definition=n.definition.id;
  d.binding.model_file=n.definition.model_file;d.binding.level=n.definition.level;
  d.relation=combat::npcs::Relation::allied_to_players;
  d.spawn_health=combat::Health{30,50};d.damageable=true;d.conditions_allowed=true;
  d.healing_allowed=false;d.healing_evidence="archived spirit mechanics";
  d.fleshy=false;d.fleshy_evidence="archived spirit mechanics";
  n.combat_definition=d;result.push_back(n);
 }
 return result;
}
int main()try{
 {auto d=*templates(1).front().combat_definition;d.binding.actor=400;
  combat::npcs::LiveActor spirit(d,{1,1,1,146,400});const auto life=spirit.lifetime();
  check(!spirit.heal_ally(life,10,100));
  check(spirit.gain_spirit_health(life,10,100));
  check(spirit.actor(life)->health->current==40);
 }
 const auto plan=*skills::effects::spirits::signet_plan(0);
 world::Instance w;auto out=std::make_shared<world::Outbox>();
 w.join(1,"Owner",0,0,out,0,{}, {},100,1);w.set_vitals(1,{100,100,100,100},100);
 const auto owner=*w.effect_scope(1);out->drain();
 auto bad=templates(plan.level);bad[1].combat_definition->spawn_health.reset();
 std::vector<world::Packet> packets;
 check(!w.spawn_controlled_spirits(owner,plan,bad,1000,packets));
 check(packets.empty()&&w.controlled_spirits(owner,1000).empty()&&spawns(w.npc_packets())==0);
 bad=templates(plan.level);bad[1].combat_definition->healing_allowed=true;
 check(!w.spawn_controlled_spirits(owner,plan,bad,1000,packets)&&packets.empty());
 bad=templates(plan.level);bad[1].combat_definition->creature_type="human";
 check(!w.spawn_controlled_spirits(owner,plan,bad,1000,packets)&&packets.empty());
 auto collision=templates(plan.level);collision[1].definition.id=collision[0].definition.id;
 check(!w.spawn_controlled_spirits(owner,plan,collision,1000,packets)&&packets.empty());
 auto forged=plan;++forged.attack_damage;
 check(!w.spawn_controlled_spirits(owner,forged,templates(plan.level),1000,packets));
 forged=plan;++forged.nominal_attack_period;
 check(!w.spawn_controlled_spirits(owner,forged,templates(plan.level),1000,packets));
 forged=plan;++forged.nominal_release_windup;
 check(!w.spawn_controlled_spirits(owner,forged,templates(plan.level),1000,packets));
 const auto created=w.spawn_controlled_spirits(owner,plan,templates(plan.level),1000,packets);
 check(created&&spawns(packets)==3&&spawns(w.npc_packets())==3);
 const auto controlled=w.controlled_spirits(owner,1000);
 check(controlled.size()==3&&out->drain().empty());
 check(w.spirit_light_proximity(owner,1000)==skills::effects::SpiritProximity::present);
 check(w.mend_body_soul_spirit_count(owner,1000)==std::optional<unsigned>(3));
 for(unsigned i=0;i<3;++i)check(controlled[i].attack_skill==skills::effects::spirits::signet_attack_skills[i]);
 for(const auto scope:*created)check(w.effect_scope(scope.actor)==scope);
 check(!w.begin_npc_action((*created)[0],combat::npcs::control::Action::attack,
                           1001,3001,skills::Slot{controlled[0].attack_skill,0}));
 check(!w.begin_npc_action((*created)[0],combat::npcs::control::Action::attack,
                           3002,5002,skills::Slot{1239,0}));
 std::array<navigation::Point,3> landing{{{0,20},{40,20},{80,20}}};
 auto summon=*skills::effects::spirits::summon_plan(0);
 auto altered=summon;++altered.healing;
 check(!w.summon_owned_spirits(owner,altered,landing,1100,packets)&&packets.empty());
 check(!w.summon_owned_spirits(owner,summon,std::span<const navigation::Point>(landing.data(),2),1100,packets));
 check(w.summon_owned_spirits(owner,summon,landing,1100,packets));
 check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=18&&p[0]==159&&p[2]==20;})==3);
 check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==16&&p[0]==44&&p[1]==0;})==3);
 for(const auto scope:*created)check(w.npc_combat_snapshot(scope)->health->current==50);
 auto conflicting=templates(plan.level);conflicting[0].definition.model_file=1235;
 check(!w.spawn_controlled_spirits(owner,plan,conflicting,1200,packets)&&packets.empty());
 const auto second=w.spawn_controlled_spirits(owner,plan,templates(plan.level),1200,packets);
 check(second&&spawns(packets)==3&&w.controlled_spirits(owner,1200).size()==6);
 check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p.size()>=6&&p[1]==0&&(p[0]==85||p[0]==86);}));
 for(const auto scope:*second)check(w.effect_scope(scope.actor)==scope);
 w.tick(60999);check(w.controlled_spirits(owner,60999).size()==6);
 w.tick(61000);check(w.controlled_spirits(owner,61000).size()==3&&spawns(w.npc_packets())==3);
 w.tick(61200);check(w.controlled_spirits(owner,61200).empty()&&spawns(w.npc_packets())==0);
 check(spawns(out->drain())==0);
 const auto again=w.spawn_controlled_spirits(owner,plan,templates(plan.level),62000,packets);
 check(again&&(*again)[0]!=(*created)[0]);
 w.leave(1);check(w.controlled_spirits(owner,62000).empty()&&spawns(w.npc_packets())==0);
 {
  world::Instance battle;
  auto enemy=templates(plan.level).front();enemy.id=100;enemy.definition.id=500;
  enemy.x=400;enemy.combat_definition->binding.actor=100;
  enemy.combat_definition->binding.definition=500;
  enemy.combat_definition->relation=combat::npcs::Relation::hostile_to_players;
  auto far=enemy;far.id=101;far.definition.id=501;far.x=1500;
  far.combat_definition->binding.actor=101;far.combat_definition->binding.definition=501;
  auto other_plane=enemy;other_plane.id=102;other_plane.definition.id=502;
  other_plane.x=100;other_plane.plane=1;
  other_plane.combat_definition->binding.actor=102;other_plane.combat_definition->binding.definition=502;
  battle.configure_npcs({enemy,far,other_plane});
  auto battle_out=std::make_shared<world::Outbox>();
  battle.join(1,"Owner",0,0,battle_out,0,{}, {},100,1);
  auto peer_out=std::make_shared<world::Outbox>();
  battle.join(2,"Peer",0,0,peer_out,0,{}, {},100,1);
  battle.set_vitals(1,{100,100,100,100},100);
  const auto battle_owner=*battle.effect_scope(1);
  const auto target=*battle.effect_scope(100);
  const auto far_target=*battle.effect_scope(101);
  const auto other_plane_target=*battle.effect_scope(102);
  auto battle_packets=std::vector<world::Packet>{};
  const auto spirits=battle.spawn_controlled_spirits(battle_owner,plan,templates(plan.level),1000,battle_packets);
  check(spirits.has_value());
  check(!battle.mend_body_soul_spirit_count(battle_owner,1000));
  battle_out->drain();peer_out->drain();
  const auto started=battle.begin_controlled_spirit_attack((*spirits)[0],target,1001);
  check(started&&started->target==target&&started->packet==
        skills::effects::spirits::attack_start_2022((*spirits)[0].actor,target.actor,3153));
  check(battle_out->drain()==std::vector<world::Packet>{started->packet});
  check(peer_out->drain()==std::vector<world::Packet>{started->packet});
  check(!battle.begin_controlled_spirit_attack((*spirits)[0],target,1002));
  check(!battle.complete_npc_action(started->token,1901));
  check(!battle.release_controlled_spirit_attack(started->token,1900));
  check(battle_out->drain().empty()&&peer_out->drain().empty());
  const auto released=battle.release_controlled_spirit_attack(started->token,1901);
  check(released&&released->size()==29&&(*released)[0]==163);
  check(battle_out->drain()==std::vector<world::Packet>{*released});
  check(peer_out->drain()==std::vector<world::Packet>{*released});
  check(!battle.release_controlled_spirit_attack(started->token,1901));
  check(!battle.begin_controlled_spirit_attack((*spirits)[0],battle_owner,1902));
  auto stale_target=target;++stale_target.generation;
  check(!battle.begin_controlled_spirit_attack((*spirits)[1],stale_target,1001));
  check(!battle.begin_controlled_spirit_attack((*spirits)[1],(*spirits)[0],1001));
  check(!battle.begin_controlled_spirit_attack((*spirits)[1],far_target,1001));
  check(!battle.begin_controlled_spirit_attack((*spirits)[1],other_plane_target,1001));
  check(battle_out->drain().empty()&&peer_out->drain().empty());
  battle.tick(2151);
  check(battle.npc_combat_snapshot(target)->health->current==30);
  battle.tick(2251);
  check(battle.npc_combat_snapshot(target)->health->current==30-plan.attack_damage);
  const auto owner_impact=battle_out->drain(),peer_impact=peer_out->drain();
  const auto is_arrival=[](const auto& packet){return packet.size()==14&&packet[0]==166;};
  const auto owner_damage=[](const auto& packet){
   return packet.size()==18&&packet[0]==162&&packet[2]==55&&packet[6]==100&&packet[10]==1;
  };
  check(std::count_if(owner_impact.begin(),owner_impact.end(),is_arrival)==1);
  check(std::count_if(peer_impact.begin(),peer_impact.end(),is_arrival)==1);
  check(std::count_if(owner_impact.begin(),owner_impact.end(),owner_damage)==1);
  check(std::count_if(peer_impact.begin(),peer_impact.end(),owner_damage)==1);
  battle.tick(2351);
  check(battle.npc_combat_snapshot(target)->health->current==30-plan.attack_damage);
  const auto second_start=battle.begin_controlled_spirit_attack((*spirits)[1],target,4000);
  check(second_start.has_value());battle_out->drain();peer_out->drain();
  check(battle.release_controlled_spirit_attack(second_start->token,4900).has_value());
  battle_out->drain();peer_out->drain();
  check(battle.damage_npc(battle_owner,target,
      {combat::Kind::armor_ignoring,100,20,133},1248,5000).killed);
  battle_out->drain();peer_out->drain();
  battle.tick(5400);
  check(battle.npc_combat_snapshot(target)->health->current==0);
  const auto dead_target_arrival=battle_out->drain();
  check(std::count_if(dead_target_arrival.begin(),dead_target_arrival.end(),is_arrival)==1);
  check(std::count_if(dead_target_arrival.begin(),dead_target_arrival.end(),owner_damage)==0);
  battle.tick(61000);
  check(!battle.begin_controlled_spirit_attack((*spirits)[0],target,61001));
 }
 {
  world::Instance automatic;
  auto enemy=templates(plan.level).front();enemy.id=100;enemy.definition.id=500;
  enemy.x=400;enemy.combat_definition->binding.actor=100;
  enemy.combat_definition->binding.definition=500;
  enemy.combat_definition->relation=combat::npcs::Relation::hostile_to_players;
  automatic.configure_npcs({enemy});
  auto listener=std::make_shared<world::Outbox>();
  automatic.join(1,"Owner",0,0,listener,0,{}, {},100,1);
  automatic.set_vitals(1,{100,100,100,100},100);
  auto scope=*automatic.effect_scope(1);
  std::vector<world::Packet> packets;
  check(automatic.spawn_controlled_spirits(scope,plan,templates(plan.level),1000,packets).has_value());
  listener->drain();
  const auto starts=[](const auto& p){return p.size()>=16&&p[0]==159&&p[2]==50;};
  const auto releases=[](const auto& p){return p.size()==29&&p[0]==163;};
  automatic.drive_controlled_spirits(1001);
  auto published=listener->drain();
  check(std::count_if(published.begin(),published.end(),starts)==3);
  automatic.drive_controlled_spirits(1002);
  check(listener->drain().empty());
  automatic.drive_controlled_spirits(1900);
  check(listener->drain().empty());
  automatic.drive_controlled_spirits(1901);
  published=listener->drain();
  check(std::count_if(published.begin(),published.end(),releases)==3);
  automatic.tick(2251);
  check(automatic.npc_combat_snapshot(*automatic.effect_scope(100))->health->current==
        30-3*plan.attack_damage);
  listener->drain();
  automatic.drive_controlled_spirits(3000);
  check(listener->drain().empty());
  automatic.drive_controlled_spirits(3001);
  published=listener->drain();
  check(std::count_if(published.begin(),published.end(),starts)==3);
 }
 unsigned reversal_cases=0;
 // Completed spirit attacks retain owner credit, but conversion belongs to
 // the recipient's current generation at impact, not at launch.
 const auto conversion_case=[&](unsigned rank,unsigned ordinal,int rr,unsigned hp,unsigned lifecycle,bool wound){
  world::Instance battle;const auto attack=*skills::effects::spirits::signet_plan(rank);
  auto enemy=templates(attack.level).front();enemy.id=100;enemy.definition.id=500;enemy.x=400;
  auto& d=*enemy.combat_definition;d.binding.actor=100;d.binding.definition=500;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(hp),1000};
  d.healing_allowed=d.fleshy=true;d.healing_evidence=d.fleshy_evidence="synthetic living conversion recipient";
  battle.configure_npcs({enemy});auto out=std::make_shared<world::Outbox>();
  battle.join(1,"Owner",0,0,out,0,{}, {},100,1);battle.set_vitals(1,{1000,1000,100,100},100);
  const auto owner=*battle.effect_scope(1),target=*battle.effect_scope(100);
  std::vector<world::Packet> spawn;const auto spirits=battle.spawn_controlled_spirits(owner,attack,templates(attack.level),1000,spawn);
  check(spirits.has_value());
  const std::uint64_t start=lifecycle==2?60000:1001,release=start+900;
  const auto begun=battle.begin_controlled_spirit_attack((*spirits)[ordinal],target,start);check(begun.has_value());
  const auto packet=battle.release_controlled_spirit_attack(begun->token,release);check(packet.has_value());
  const auto from=float(ordinal*40);const auto duration=std::uint64_t(std::ceil(double(*skills::effects::spirits::signet_attack_flight_seconds(from,0,400,0))*1000));
  const auto impact=release+duration;
  if(rr>=0)check(battle.stage_npc_reversal(target,unsigned(rr),release));
  if(wound){
   check(battle.apply_condition(owner,owner,combat::conditions::Kind::deep_wound,release,10000));
   check(battle.condition_transfer(owner,target,*skills::effects::condition_transfer(132,12),release));
  }
  const auto before_impact=battle.npc_combat_snapshot(target)->health->current;
  if(lifecycle==1)check(battle.kill(owner,release));
  if(lifecycle==2){battle.tick(61000);check(battle.controlled_spirits(owner,61000).empty());}
  if(lifecycle==3)battle.leave(1);
  out->drain();battle.tick(impact-100);
  if(battle.npc_combat_snapshot(target)->health->current!=before_impact)
   std::cerr<<"before impact rank="<<rank<<" ordinal="<<ordinal<<" rr="<<rr<<" hp="<<hp<<" lifecycle="<<lifecycle<<" wound="<<wound<<" actual="<<battle.npc_combat_snapshot(target)->health->current<<" impact="<<impact<<'\n';
  check(battle.npc_combat_snapshot(target)->health->current==before_impact);
  battle.tick(impact);
  const auto converted=rr<0?0:std::min(attack.attack_damage,skills::effects::reversal_fortune(unsigned(rr))->maximum_healing);
  const double expected=lifecycle==3?hp:wound?0:std::max(0.,std::min(1000.,double(hp+converted))-double(attack.attack_damage-converted));
  check(battle.npc_combat_snapshot(target)->health->current==expected);
  const auto statuses=*battle.status_snapshot(target,impact);
  const bool retained=std::any_of(statuses.begin(),statuses.end(),[](const auto& e){return e.skill==307;});
  check(retained==(lifecycle==3&&rr>=0));
  if(lifecycle!=3)check(battle.take_npc_deaths(owner).size()==unsigned(expected<=0));
  const auto packets=out->drain();
  const auto arrivals=std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==14&&p[0]==166;});
  check(arrivals==unsigned(lifecycle!=3));
  battle.tick(impact+100);check(battle.npc_combat_snapshot(target)->health->current==expected);
  if(lifecycle!=3)check(battle.take_npc_deaths(owner).empty());
  ++reversal_cases;
 };
 for(unsigned rank:{0u,12u,20u})for(unsigned ordinal=0;ordinal<3;++ordinal)
  for(int rr:{-1,0,12,20})for(unsigned hp:{1u,10u,500u,1000u})conversion_case(rank,ordinal,rr,hp,0,false);
 for(unsigned lifecycle:{1u,2u,3u})for(unsigned ordinal=0;ordinal<3;++ordinal)
  for(int rr:{0,12,20})conversion_case(12,ordinal,rr,500,lifecycle,false);
 for(unsigned ordinal=0;ordinal<3;++ordinal)for(int rr:{0,12,20})conversion_case(12,ordinal,rr,10,0,true);
 std::cout<<"PASS atomic controlled NPC spawn, deferred packets, expiry and departure; "<<reversal_cases
          <<" conversion/survival/healing-death/owner-lifetime cases; no skill binding\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

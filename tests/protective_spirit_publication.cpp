#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <barrier>
#include <thread>
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Protective Spirit publication line "+std::to_string(at.line()));}
int main()try{
 for(unsigned run=0;run<16;++run){world::Instance w;std::array<std::shared_ptr<world::Outbox>,2> boxes;
  for(unsigned id=1;id<=2;++id){boxes[id-1]=std::make_shared<world::Outbox>();w.join(id,"Actor",id*50,0,boxes[id-1],0,{world::Packet{165,0,static_cast<std::uint8_t>(id),0,0,0,3,0}}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);for(auto& b:boxes)b->drain();
  const world::Packet marker{250,0,27,0};std::barrier start(2);std::thread producer([&]{start.arrive_and_wait();for(unsigned i=0;i<100;++i){for(auto& b:boxes)b->enqueue({marker});std::this_thread::yield();}});
  start.arrive_and_wait();const bool ok=w.complete_protective_spirit(caster,target,*effects::protective_spirit(12),12,1000);producer.join();check(ok);
  for(auto& b:boxes){const auto p=b->drain();const auto finish=std::find(p.begin(),p.end(),casting::terminal(58,1));const auto body=std::find(p.begin(),p.end(),*particles::targeted_body_2022(2,1,245));
   const auto added=std::find_if(p.begin(),p.end(),[](const auto& packet){return packet.size()==20&&packet[0]==65&&packet[6]==245;});
   const auto heal=std::find(p.begin(),p.end(),particles::health_change_2022(2,1,38,1000));
   check(finish!=p.end()&&body!=p.end()&&added!=p.end()&&heal!=p.end()&&finish<body&&body<added&&added<heal);
   check(std::find(finish,heal,marker)==heal&&std::count(p.begin(),p.end(),marker)==100);
  }
 }
 // The recipient's owned end queue preserves the actual strip order.
 for(bool spirit_newer:{false,true}){combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;d.cold_armor=60;
  combat::npcs::LiveActor n(d,{1,1,1,146,100});const auto life=n.lifetime();const effects::Scope caster{1,1,1};
  if(!spirit_newer)check(n.apply_protective_spirit(life,caster,*effects::protective_spirit(12),1,1000));
  check(n.apply_watchful_healing(life,*effects::watchful_healing(12),1,2,1000));
  if(spirit_newer)check(n.apply_protective_spirit(life,caster,*effects::protective_spirit(12),3,1000));
  const std::vector<effects::ChilblainsNpcRecipient> inputs{{n,life,0}};const auto changed=effects::prepare_chilblains_npcs(*effects::chilblains(12),caster,20,{},inputs,effects::captured_chilblains_order,1000);check(changed&&changed->recipients.size()==1);
  auto actor=changed->recipients[0].actor;const auto ends=actor.take_enchantment_ends(life);check(ends.size()==2&&ends[0].entry.skill==(spirit_newer?245:1392)&&ends[1].entry.skill==(spirit_newer?1392:245));
 }
 for(bool npc:{false,true}){world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
  if(!npc){w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{500,1000,100,100},100);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));}
  const unsigned id=npc?100:2;const auto source=*w.effect_scope(1),target=*w.effect_scope(id);box->drain();
  const auto effect=[&](const auto& p){for(const auto& packet:p)if(packet.size()==20&&packet[0]==65&&packet[6]==245)return unsigned(packet[12])|(unsigned(packet[13])<<8)|(unsigned(packet[14])<<16)|(unsigned(packet[15])<<24);throw std::runtime_error("missing245effect");};
  check(w.complete_protective_spirit(source,target,*effects::protective_spirit(12),0,1000));const auto first=effect(box->drain());
  check(w.complete_protective_spirit(source,target,*effects::protective_spirit(12),0,5000));const auto renewed=box->drain();const auto second=effect(renewed);check(first!=second&&std::count(renewed.begin(),renewed.end(),casting::effect_remove(id,first))==1);
  w.tick(20000);const auto old_deadline=box->drain();check(std::find(old_deadline.begin(),old_deadline.end(),casting::effect_remove(id,second))==old_deadline.end());
  w.tick(24000);const auto expired=box->drain();check(std::count(expired.begin(),expired.end(),casting::effect_remove(id,second))==1);w.tick(25000);const auto again=box->drain();check(std::find(again.begin(),again.end(),casting::effect_remove(id,second))==again.end());
 }
 {world::Instance w;auto box=std::make_shared<world::Outbox>();for(unsigned id=1;id<=3;++id){w.join(id,"Actor",id*50,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{id==2?55u:1000u,id==2?55u:1000u,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(1),target=*w.effect_scope(2),foe=*w.effect_scope(3);check(w.elemental_armor(target,60,60)&&w.elemental_armor(source,60,60));
  check(w.complete_protective_spirit(source,target,*effects::protective_spirit(12),0,1000));const auto entry=w.status_snapshot(target,1000)->front();box->drain();
  check(w.complete_chilblains(foe,target,*effects::chilblains(12),20,2000));const auto removed=box->drain();check(w.resources(2)->health==50&&std::count(removed.begin(),removed.end(),casting::effect_remove(2,entry.instance))==1);
  w.tick(21000);const auto expired=box->drain();check(std::find(expired.begin(),expired.end(),casting::effect_remove(2,entry.instance))==expired.end());
 }
 std::cout<<"PASS16 atomic Protective Spirit batches plus mixed245/1392 ends,real player/NPC renewal/expiry and player Chilblains removal\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

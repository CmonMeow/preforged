#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <barrier>
#include <thread>
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Shield of Deflection publication line "+std::to_string(at.line()));}
int main()try{
 for(unsigned run=0;run<16;++run){world::Instance w;std::array<std::shared_ptr<world::Outbox>,2> boxes;
  for(unsigned id=1;id<=2;++id){boxes[id-1]=std::make_shared<world::Outbox>();w.join(id,"Actor",id*50,0,boxes[id-1],0,{world::Packet{165,0,static_cast<std::uint8_t>(id),0,0,0,3,0}}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);for(auto& b:boxes)b->drain();
  const world::Packet marker{250,0,27,0};std::barrier start(2);std::thread producer([&]{start.arrive_and_wait();for(unsigned i=0;i<100;++i){for(auto& b:boxes)b->enqueue({marker});std::this_thread::yield();}});
  start.arrive_and_wait();const bool ok=w.complete_shield_deflection(caster,target,*effects::shield_deflection(12),12,1000);producer.join();check(ok);
  for(auto& b:boxes){const auto p=b->drain();const auto finish=std::find(p.begin(),p.end(),casting::terminal(58,1));const auto body=std::find(p.begin(),p.end(),*particles::targeted_body_2022(2,1,259));
   const auto added=std::find_if(p.begin(),p.end(),[](const auto& packet){return packet.size()==20&&packet[0]==65&&packet[6]==3&&packet[7]==1;});
   const auto heal=std::find(p.begin(),p.end(),particles::health_change_2022(2,1,38,1000));
   check(finish!=p.end()&&body!=p.end()&&added!=p.end()&&heal!=p.end()&&finish<body&&body<added&&added<heal);
   check(std::find(finish,heal,marker)==heal&&std::count(p.begin(),p.end(),marker)==100);
  }
 }
 // Both skills share lifecycle bookkeeping but must keep separate credits.
 for(bool deflection_first:{false,true})for(unsigned renewal:{245u,259u}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);const auto source=*w.effect_scope(1),target=*w.effect_scope(100);box->drain();
  auto apply=[&](unsigned skill,unsigned at){return skill==245?w.complete_protective_spirit(source,target,*effects::protective_spirit(12),0,at):w.complete_shield_deflection(source,target,*effects::shield_deflection(12),0,at);};
  auto effect=[&](const auto& packets,unsigned skill){for(const auto& p:packets)if(p.size()==20&&p[0]==65&&(unsigned(p[6])|(unsigned(p[7])<<8))==skill)return unsigned(p[12])|(unsigned(p[13])<<8)|(unsigned(p[14])<<16)|(unsigned(p[15])<<24);throw std::runtime_error("missing effect");};
  const unsigned first=deflection_first?259:245,second=deflection_first?245:259;
  check(apply(first,1000));const auto first_id=effect(box->drain(),first);check(apply(second,1100));const auto second_id=effect(box->drain(),second);
  check(apply(renewal,1200));const auto renewed=box->drain();const auto replacement=effect(renewed,renewal);const auto old=renewal==first?first_id:second_id;
  check(std::count(renewed.begin(),renewed.end(),casting::effect_remove(100,old))==1);
  const auto spirit_id=renewal==245?replacement:first==245?first_id:second_id,deflection_id=renewal==259?replacement:first==259?first_id:second_id;
  w.tick(10200);const auto early=box->drain();check(std::count(early.begin(),early.end(),casting::effect_remove(100,deflection_id))==1);check(std::count(early.begin(),early.end(),casting::effect_remove(100,spirit_id))==0);
  w.tick(20200);const auto late=box->drain();check(std::count(late.begin(),late.end(),casting::effect_remove(100,spirit_id))==1);check(std::count(late.begin(),late.end(),casting::effect_remove(100,deflection_id))==0);
  w.tick(21000);const auto again=box->drain();check(std::count(again.begin(),again.end(),casting::effect_remove(100,spirit_id))==0&&std::count(again.begin(),again.end(),casting::effect_remove(100,deflection_id))==0);
 }
 std::cout<<"PASS16 atomic Shield of Deflection batches and4 mixed245/259 NPC ownership/renewal/expiry cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

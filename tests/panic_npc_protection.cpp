#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Panic NPC protection line "+std::to_string(at.line()));}
unsigned word(const Packet& p,unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);}
int main()try{
 for(unsigned mode=0;mode<7;++mode){warrior_test::Fixture f(340,true,1000,true,true,1,mode==3?std::nullopt:std::optional(combat::npcs::Energy{20,20}));auto& w=f.world;
  w.join(2,"Trigger",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,30,30},100);const auto trigger=*w.effect_scope(2);
  w.join(3,"Protected player",70,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{500,500,30,30},100);const auto player=*w.effect_scope(3);
  check(w.cast_activity(player,281,100,3000)&&w.status_mutation(player,16,123,0,100,10100,true));
  check(w.apply_panic(f.source,trigger,12,100));
  if(mode==1||mode==2||mode==3)check(w.npc_interrupt_stance(f.target,mode==1?16:17,0,10000,100));
  unsigned stance_effect=0;for(const auto& packet:f.box->drain())if(packet.size()==20&&packet[0]==65&&word(packet,2)==f.target.actor&&((packet[6]==16||packet[6]==17)&&packet[7]==0))stance_effect=word(packet,12);
  const auto kind=mode==6?combat::npcs::control::Action::attack:combat::npcs::control::Action::spell;
  std::optional<Slot> slot=(mode==4||mode==6)?std::nullopt:std::optional(Slot{static_cast<std::uint16_t>(mode==5?329:281),0});
  const auto action=w.begin_npc_action(f.target,kind,100,3000,slot);check(action.has_value());
  effects::PanicUse use{trigger,281,1,150,200,true};f.box->drain();
  check(w.panic_skill_used(use)==(mode!=3));auto wire=f.box->drain();const auto stopped=mode==6?combat::weapon::stop_2022(f.target.actor):casting::terminal(mode==5?49:59,f.target.actor);
  if(mode==3){check(std::count(wire.begin(),wire.end(),stopped)==0&&w.casting_skill(player,200)==281);
   const auto snapshot=*w.status_snapshot(player,200);check(std::any_of(snapshot.begin(),snapshot.end(),[](const auto& e){return e.skill==16;}));
   check(stance_effect&&w.end_stance(f.target,stance_effect,200));
   check(w.panic_skill_used(use));
   check(!w.complete_npc_action(*action,3000));continue;}
  check(std::count(wire.begin(),wire.end(),stopped)==unsigned(mode==0||mode==4||mode==5||mode==6));
  if(mode==1||mode==2){const unsigned extra=mode==1?1:3;
   if(mode==2)check(w.npc_energy_snapshot(f.target)->current==10);
   for(unsigned i=0;i<extra;++i){++use.event;++use.used;check(w.panic_skill_used(use));}
   wire=f.box->drain();check(std::count(wire.begin(),wire.end(),stopped)==1);
   if(mode==2)check(w.npc_energy_snapshot(f.target)->current==0);
  }
  check(!w.complete_npc_action(*action,3000));check(f.health()==1000);
 }
 std::cout<<"PASS Panic NPC Concentration/Resolve protection and exact drains,atomic unknown Energy/player preservation/retry,ordinary spell and attack-skill terminal publication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

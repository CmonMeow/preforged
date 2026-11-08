#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <barrier>
#include <thread>
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Chilblains atomic publication line "+std::to_string(at.line()));}
int main()try{
 for(unsigned run=0;run<24;++run){world::Instance w;std::array<std::shared_ptr<world::Outbox>,3> boxes;
  for(unsigned id=1;id<=3;++id){boxes[id-1]=std::make_shared<world::Outbox>();w.join(id,"Actor",id*60,0,boxes[id-1],0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==2?500u:1000u,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(2),healer=*w.effect_scope(3);
  check(w.party_action(3,158,2,100)&&w.party_action(2,154,3,100));
  check(w.protective_enchantment(healer,target,*regeneration::reviewed(1392,12),0,100));
  const auto effect=w.status_snapshot(target,100)->front();for(auto& box:boxes)box->drain();
  const world::Packet marker{250,0,27,0};std::barrier rendezvous(2);
  std::thread producer([&]{rendezvous.arrive_and_wait();for(unsigned i=0;i<200;++i){for(auto& box:boxes)box->enqueue({marker});std::this_thread::yield();}});
  rendezvous.arrive_and_wait();const bool accepted=w.complete_chilblains(caster,target,*effects::chilblains(12),20,1000);producer.join();check(accepted);
  for(auto& box:boxes){const auto packets=box->drain();
   const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
   const auto caster_body=std::find(packets.begin(),packets.end(),combat::detail::integer(21,1,256));
   const auto target_body=std::find(packets.begin(),packets.end(),particles::chilblains_target_2022(2,1));
   const auto gain=std::find(packets.begin(),packets.end(),particles::health_change_2022(2,3,102,1000));
   const auto removed=std::find(packets.begin(),packets.end(),casting::effect_remove(2,effect.instance));
   const auto damage=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=18&&p[0]==162&&p[2]==16&&p[6]==2&&p[10]==1;});
   const auto poison=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=8&&p[0]==65&&p[2]==1&&p[6]==228&&p[7]==1;});
   check(finish!=packets.end()&&caster_body!=packets.end()&&poison!=packets.end()&&damage!=packets.end()&&target_body!=packets.end()&&gain!=packets.end()&&removed!=packets.end());
   check(finish<caster_body&&caster_body<poison&&poison<damage&&damage<target_body&&target_body<gain&&gain<removed);
   check(std::find(finish,removed,marker)==removed);check(std::count(packets.begin(),packets.end(),marker)==200);
   check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1&&std::count(packets.begin(),packets.end(),particles::chilblains_target_2022(2,1))==1);
  }
 }
 std::cout<<"PASS24 atomic production Chilblains batches against concurrent producers;finish/caster/Poison/damage/target/Watchful gain/removal order\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

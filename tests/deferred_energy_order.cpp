#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <cstring>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Deferred Energy order line "+std::to_string(at.line()));}
unsigned read(const world::Packet& p,unsigned at){unsigned v=0;for(unsigned i=0;i<4;++i)v|=unsigned(p[at+i])<<(8*i);return v;}
int main()try{using namespace skills::effects;
 for(bool pay_cost:{false,true}){
 world::Instance w;auto box=std::make_shared<world::Outbox>();
 w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 w.set_vitals(1,{500,500,0,100},100);w.set_vitals(2,{500,500,100,100},100);
 const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);
 check(w.energy_capacity(1,5,0,100,{80,-2}));
 check(w.degeneration_hex(caster,target,*degeneration_hex(36,0),100));
 check(w.wound_spell(target,caster,*wound_spell(44,0),20,100));
 const auto status=*w.status_snapshot(caster,100);check(status.size()==1);const auto effect=status.front().instance;
 box->drain();if(pay_cost)check(w.consume_energy(1,arcane_conundrum(0)->energy,caster,20100));else w.tick(20100);const auto packets=box->drain();
 check(w.resources(1)->energy==(pay_cost?0u:arcane_conundrum(0)->energy));
 std::size_t ended=packets.size(),credit=packets.size();
 for(std::size_t i=0;i<packets.size();++i){const auto& p=packets[i];
  if(p.size()==10&&p[0]==67&&read(p,2)==1&&read(p,6)==effect)ended=std::min(ended,i);
  if(p.size()==14&&p[0]==161&&read(p,2)==33&&read(p,6)==1&&std::bit_cast<float>(read(p,10))>0)credit=std::min(credit,i);
 }
 std::cout<<"order: ended="<<ended<<" credit="<<credit<<" packets="<<packets.size()<<"\n";
 check(ended<packets.size()&&credit<packets.size());
 check(ended<credit);
 w.tick(20200);check(w.resources(1)->energy==(pay_cost?0u:arcane_conundrum(0)->energy));
 }
 std::cout<<"PASS deferred Arcane Energy credit follows caster timed end effects, before same-event cost, without recursion or duplicate payout\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


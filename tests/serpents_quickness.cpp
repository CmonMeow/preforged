#include "gw_cast_timing.h"
#include <iostream>
#include <stdexcept>
#include <source_location>
using namespace fw::gw::skills;using namespace effects;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Serpents Quickness line "+std::to_string(at.line()));
}
int main()try{
 check(serpents_quickness(0)->duration==15000&&serpents_quickness(12)->duration==27000&&
       serpents_quickness(15)->duration==30000&&serpents_quickness(20)->duration==35000&&!serpents_quickness(21));
 Entry e{1,42,456,12,Category::stance,100,27100};
 const auto spell=*find_skill(28),signet=*find_skill(294),attack=*find_skill(399),stance=*find_skill(455);
 for(auto skill:{spell,signet,attack}){
  check(serpents_quickness_reduces(skill,e,100));
  check(timing::recharge(skill,2,0,false,{e},100,10)==7);
  check(timing::recharge(skill,2,0,true,{e},100,15)==10);
  check(timing::recharge(skill,2,0,false,{e},99,10)==10);
  check(timing::recharge(skill,2,0,false,{e},27100,10)==10);
 }
 check(timing::recharge(*find_skill(456),2,0,false,{e},100,45)==45);
 check(timing::recharge(stance,2,0,false,{e},100,20)==20);
 check(timing::recharge(spell,2,0,false,{e},100,0)==0);
 // Shared cap and single-effect exception, with one final rounding.
 check(timing::recharge(spell,2,0,false,{e},100,15,.5)==8);
 check(timing::recharge(spell,2,0,false,{e},100,15,.4)==6);
 auto mesmer=spell;mesmer.profession=5;
 check(timing::recharge(mesmer,5,12,false,{e},100,15)==6);
 check(timing::recharge(mesmer,5,12,true,{e},100,15)==10);
 for(unsigned mode=0;mode<5;++mode){auto bad=e;
  if(mode==0)bad.kind=Category::enchantment;if(mode==1)bad.rank=21;
  if(mode==2)bad.expires.reset();if(mode==3)bad.instance=0;if(mode==4)bad.cause=0;
  check(timing::recharge(spell,2,0,false,{bad},100,10)==10);
 }
 check(!serpents_quickness_ends(250,500)&&serpents_quickness_ends(249.99,500)&&
       !serpents_quickness_ends(251,500)&&!serpents_quickness_ends(0,0));
 std::cout<<"PASS Serpents Quickness canonical duration,completion recharge,stance exclusions,cap/FastCasting and strict Health predicate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

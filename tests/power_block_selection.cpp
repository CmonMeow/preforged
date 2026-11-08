#include "gw_power_block.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills;using namespace effects;
void check(bool ok){if(!ok)throw std::runtime_error("Power Block selection regression");}
int main()try{
 const auto plan=*power_block(12);check(plan.disable==10000&&power_block(0)->disable==1000&&power_block(15)->disable==12000&&power_block(20)->disable==16000&&!power_block(21));
 Slots bar{};bar[0]={23,0};bar[1]={28,0};bar[2]={28,1};bar[3]={14,0};
 const auto selected=power_block_disabled_slots(plan,23,bar);check(selected&&selected->size()==3&&(*selected)[0]==bar[0]&&(*selected)[2]==bar[2]);
 bar[4]=bar[0];check(!power_block_disabled_slots(plan,23,bar));bar[4]={};
 DisableDeadlines current{{{23,0},5000},{{28,0},20000},{{14,0},9000}};
 const auto extended=power_block_deadlines(current,plan,23,bar,1000);
 check(extended&&extended->at({23,0})==11000&&extended->at({28,0})==20000&&extended->at({28,1})==11000&&extended->at({14,0})==9000);
 check(current.size()==3&&current.at({23,0})==5000); // Input remains unchanged until owner commits.
 check(!power_block_deadlines(current,plan,23,bar,UINT64_MAX));
 auto invalid=current;invalid[{99999,0}]=12000;check(!power_block_deadlines(invalid,plan,23,bar,1000));
 auto bad_bar=bar;bad_bar[7]={99999,0};check(!power_block_deadlines(current,plan,23,bad_bar,1000)&&current.size()==3);
 const auto repeated=power_block_deadlines(*extended,plan,23,bar,2000);check(repeated&&repeated->at({23,0})==12000&&repeated->at({28,0})==20000);
 auto forged=plan;forged.disable=1;check(!power_block_disabled_slots(forged,23,bar));
 check(!power_block_disabled_slots(plan,0,bar)&&!power_block_disabled_slots(plan,349,bar));
 for(bool pve:{false,true}){
  unsigned active=0,other=0;
  for(unsigned id=1;id<3433;++id){const auto* row=find_skill(id);
   if(is_spell(*row)&&(pve?bool(row->special&0x80000u):row->attribute==51)&&!active)active=id;
  }
  check(active!=0);
  for(unsigned id=1;id<3433;++id)if(id!=active&&find_skill(id)->attribute==find_skill(active)->attribute){other=id;break;}
  check(other!=0);Slots special{};special[0]={active,0};special[1]={other,0};special[2]={active,1};
  const auto only=power_block_disabled_slots(plan,active,special);check(only&&only->size()==2&&(*only)[0]==special[0]&&(*only)[1]==special[2]);
 }
 std::cout<<"PASS Power Block canonical disable duration,attribute-group slot copies,no-attribute/PvE exceptions and invalid bar rejection;Runtime deadlines/casting pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

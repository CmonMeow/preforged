#include "gw_frenzied_defense.h"
#include "gw_block_attribution.h"
#include "gw_cast_timing.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Frenzied Defense policy line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=frenzied_defense(rank);check(p&&p->rank==rank&&p->energy==5&&p->duration==8000&&p->recharge==10000&&p->block==.75&&p->strike_multiplier==2);check(timing::stance(1700,rank)->duration==8000);}check(!frenzied_defense(21)&&!timing::stance(1700,21));
 check(timing::instant_action(*find_skill(1700)));
 std::vector<Entry> entries{{123,25,1700,0,Category::stance,100,8100}};
 check(frenzied_defense_active(entries,100)==true&&frenzied_defense_active(entries,8099)==true);
 check(frenzied_defense_active(entries,99)==false&&frenzied_defense_active(entries,8100)==false);
 check(reviewed_block_probability(entries,false,100)==.75);
 check(reviewed_block_probability(entries,true,100)==0.);
 const auto sources=reviewed_block_sources(entries,100);check(sources&&sources->size()==1&&sources->front().skill==1700&&sources->front().instance==123&&sources->front().probability==.75);
 entries.push_back({124,25,258,12,Category::enchantment,100,8100});check(reviewed_block_probability(entries,false,100)==.875);
 entries.push_back({125,25,137,0,Category::hex,100,8100});check(reviewed_block_probability(entries,false,100)==0.);
 entries.resize(1);entries.push_back(entries.front());entries.back().instance=124;check(reviewed_block_probability(entries,false,100)==.75&&reviewed_block_sources(entries,100)->size()==1);
 for(unsigned mode=0;mode<5;++mode){auto bad=entries;bad.resize(1);if(mode==0)bad[0].rank=21;if(mode==1)bad[0].instance=0;if(mode==2)bad[0].cause=0;if(mode==3)bad[0].expires.reset();if(mode==4)bad[0].kind=Category::enchantment;check(!frenzied_defense_active(bad,100)&&!reviewed_block_probability(bad,false,100)&&!reviewed_block_sources(bad,100));}
 std::cout<<"PASS Frenzied Defense rank/native plan, instant timing, duration boundaries, block attribution/stacking/bypass and corrupt-entry guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

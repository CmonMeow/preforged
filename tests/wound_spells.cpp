#include "gw_wound_spells.h"
#include "gw_skill_casting.h"
#include <fstream>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("wound spell regression");}
int main(){using namespace fw::gw;using namespace skills::effects;std::ifstream fixture(FIXTURE_FILE);check(bool(fixture));
 const auto compare=[&](combat::Packet packet){std::string line;check(bool(std::getline(fixture,line)));std::vector<unsigned char> expected;for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected);};
 for(unsigned id:{1052u,3184u}){const auto plan=*wound_spell(id,12);check(plan.damage==(id==1052?63u:30u)&&plan.wound_duration==17000&&!plan.phantom);combat::Actor actor{2,1,combat::Health{480,480},{},0,true};const auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,double(plan.damage),20,id},true);for(const auto&p:combat::damage_packets_2022(1,hit))compare(p);}
 const auto phantom=*wound_spell(44,12);check(phantom.phantom&&phantom.degeneration==3&&phantom.hex_duration==10000&&phantom.wound_duration==17000);
 compare(skills::casting::effect_add(2,44,12,0x80000000,10.f));compare(skills::casting::effect_add(2,482,0,0x80000001,17.f));
 check(!wound_spell(0)&&!wound_spell(44,21));
 for(unsigned rank=0;rank<=20;++rank){
  const auto plan=*wound_spell(44,rank);
  const auto deadlines=phantom_pain_deadlines(plan,1000);
  check(deadlines&&deadlines->hex_end==11000&&deadlines->wound_end==11000+plan.wound_duration);
  const auto last=UINT64_MAX-plan.hex_duration-plan.wound_duration;
  const auto edge=phantom_pain_deadlines(plan,last);
  check(edge&&edge->wound_end==UINT64_MAX);
  check(!phantom_pain_deadlines(plan,last+1));
  check(!phantom_pain_deadlines(plan,UINT64_MAX-plan.hex_duration+1));
  auto invalid=plan;++invalid.wound_duration;
  check(!phantom_pain_deadlines(invalid,1000));
 }
 check(!phantom_pain_deadlines(*wound_spell(1052,12),1000));
 check(!phantom_pain_deadlines(*wound_spell(3184,12),1000));
}

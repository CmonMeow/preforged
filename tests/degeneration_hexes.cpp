#include "gw_degeneration_hexes.h"
#include "gw_enduring_toxin.h"
#include "gw_skill_casting.h"
#include <fstream>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("degeneration hex regression");}
int main(){using namespace fw::gw;using namespace skills::effects;std::ifstream fixture(FIXTURE_FILE);check(bool(fixture));
 const auto compare=[&](combat::Packet packet){std::string line;check(bool(std::getline(fixture,line)));std::vector<unsigned char> expected;for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected);};
 unsigned index=0;for(unsigned id:{31u,859u,108u,2054u}){const auto plan=*degeneration_hex(id,12);check(valid_degeneration_hex(plan));check(plan.pips==(id==31?5u:id==859?8u:id==108?2u:3u));check(plan.duration==(id==108?25000u:id==2054?10000u:13000u));check(plan.radius==(id==108?252.f:0.f));
  compare(skills::casting::effect_add(2,id,12,0x80000000+index,plan.duration/1000.f));compare(skills::casting::effect_remove(2,0x80000000+index));++index;
 }
 compare(skills::casting::effect_add(2,2077,0,0x80000004,17.f));
 check(degeneration_hex(108,0)->pips==0&&degeneration_hex(108,0)->duration==6000&&degeneration_hex(108,20)->pips==4);
 check(degeneration_hex(2054,12)->end_condition==combat::conditions::Kind::cracked_armor&&degeneration_hex(2054,12)->end_duration==17000);
 check(!degeneration_hex(0)&&!degeneration_hex(31,21));
 check(degeneration_hex(129,12)->pips==0&&degeneration_hex(129,12)->duration==29000);
 check(degeneration_hex(142,12)->pips==2&&degeneration_hex(142,12)->duration==25000&&degeneration_hex(142,12)->radius==252);
 check(skills::find_skill(129)->duration0==5&&skills::find_skill(129)->duration15==35&&(skills::find_skill(129)->skill_arguments&1));
 const auto toxin=enduring_toxin(12);check(toxin&&toxin->pips==4&&toxin->period==5000);
 check(enduring_toxin(0)->pips==1&&enduring_toxin(15)->pips==5&&!enduring_toxin(21));
 EnduringToxinTimer timer;check(timer.start(1000)&&timer.deadline()==6000);
 check(!timer.start(2000)&&timer.deadline()==6000);
 check(timer.settle(5999,true)==EnduringToxinTimer::Result::none);
 check(timer.settle(6200,std::nullopt)==EnduringToxinTimer::Result::deferred&&timer.deadline()==6000);
 check(timer.settle(6200,true)==EnduringToxinTimer::Result::renewed&&timer.deadline()==11000);
 check(timer.settle(10999,false)==EnduringToxinTimer::Result::none);
 check(timer.settle(11000,false)==EnduringToxinTimer::Result::expired&&!timer.active());
 check(timer.settle(12000,true)==EnduringToxinTimer::Result::none);
 check(timer.start(0));timer.cancel();check(timer.settle(5000,true)==EnduringToxinTimer::Result::none);
 check(!timer.start(UINT64_MAX-4999));
}

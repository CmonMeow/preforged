#include "gw_attack_interrupts.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat::weapon;
void check(bool ok){if(!ok)throw std::runtime_error("Savage Slash policy regression");}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=savage_slash(rank);check(p&&p->bonus==unsigned(std::lround(1.+39.*rank/15.))&&p->windup==500&&p->weapon_type==27);
 check(savage_slash_contact(*p,{AttackTargetAction::idle,{}})->bonus==0&&!savage_slash_contact(*p,{AttackTargetAction::idle,{}})->attempt_interrupt);
 for(unsigned spell:{187u,192u,281u}){const auto hit=savage_slash_contact(*p,{AttackTargetAction::skill,fw::gw::skills::Slot{static_cast<std::uint16_t>(spell),0}});check(hit&&hit->bonus==p->bonus&&hit->attempt_interrupt);}
 for(unsigned other:{361u,362u,393u}){const auto hit=savage_slash_contact(*p,{AttackTargetAction::skill,fw::gw::skills::Slot{static_cast<std::uint16_t>(other),0}});check(hit&&!hit->bonus&&hit->attempt_interrupt);}
 check(savage_slash_contact(*p,{AttackTargetAction::attack,{}})->attempt_interrupt);
 check(!savage_slash_contact(*p,{AttackTargetAction::skill,{}}));
 check(!savage_slash_contact(*p,{AttackTargetAction::idle,fw::gw::skills::Slot{187,0}}));
 check(!savage_slash_contact(*p,{AttackTargetAction::attack,fw::gw::skills::Slot{187,0}}));
 auto forged=*p;++forged.bonus;check(!savage_slash_contact(forged,{AttackTargetAction::idle,{}}));
 }
 check(!savage_slash(21));
 std::cout<<"PASS Savage Slash native plan,impact action classification,spell-only bonus,signet/attack distinction and unknown NPC action rejection;World/Runtime unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

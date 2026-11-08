#include "gw_dazed_interrupts.h"
#include <iostream>
#include <stdexcept>
namespace conditions=fw::gw::combat::conditions;
using namespace fw::gw;using namespace combat::weapon;
void check(bool ok){if(!ok)throw std::runtime_error("Dazed/Concussion policy regression");}
int main()try{
 conditions::State clear,dazed;check(dazed.apply(conditions::Kind::dazed,2,100,5000));
 const AttackActionFacts spell{AttackTargetAction::skill,skills::Slot{281,0}},signet{AttackTargetAction::skill,skills::Slot{1,0}},idle{},unknown{AttackTargetAction::skill,{}};
 for(unsigned rank=0;rank<=20;++rank){const auto p=concussion_shot(rank);check(p&&p->damage==rank+1&&p->energy==25&&p->dazed_ms==(5+rank)*1000ull);
 for(bool eligible:{false,true})for(bool existing:{false,true}){const auto proposal=concussion_contact(*p,spell,existing?dazed:clear,1000,eligible);check(proposal&&proposal->damage==1+rank&&proposal->apply_dazed==(eligible&&!existing)&&proposal->duration==((eligible&&!existing)?p->dazed_ms:0));}
 check(!concussion_contact(*p,signet,clear,1000,true)->apply_dazed&&!concussion_contact(*p,idle,clear,1000,true)->apply_dazed);
 check(!concussion_contact(*p,unknown,clear,1000,true));auto bad=*p;++bad.damage;check(!concussion_contact(bad,spell,clear,1000,true));
 }
 check(newly_dazed_interrupt(clear,dazed,spell,1000)==true);
 check(newly_dazed_interrupt(clear,dazed,signet,1000)==false);
 check(newly_dazed_interrupt(dazed,dazed,spell,1000)==false);
 check(newly_dazed_interrupt(clear,clear,spell,1000)==false); // redirected/rejected application
 check(!newly_dazed_interrupt(clear,dazed,unknown,1000));
 for(auto outcome:{AttackOutcome::hit,AttackOutcome::miss,AttackOutcome::block,AttackOutcome::dodge,AttackOutcome::cancelled,AttackOutcome::obstruction})check(dazed_hit_interrupt(dazed,spell,outcome,1000)==(outcome==AttackOutcome::hit));
 check(dazed_hit_interrupt(dazed,spell,AttackOutcome::hit,5100)==false);
 std::cout<<"PASS Concussion native damage/cost/duration,nonrenewal,immunity/redirect,new Dazed versus landed-hit spell interruption;World/Runtime unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

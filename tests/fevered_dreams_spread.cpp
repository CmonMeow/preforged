#include "gw_fevered_dreams.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using Kind=combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Fevered spread line "+std::to_string(at.line()));}
int main()try{const Scope caster{7,9,1},a{7,1,2},b{7,2,3};Registry cr(caster),ar(a),br(b);auto aa=*prepare_fevered_dreams(ar,cr,caster,a,1,*fevered_dreams(12),100);auto bb=*prepare_fevered_dreams(br,cr,caster,b,2,*fevered_dreams(12),100);combat::conditions::State clean,poison;check(poison.apply(Kind::poison,5,200,5000));auto first=*fevered_condition_reaction(aa.registry,aa.owner,{a,1,200,true},clean,poison,FeveredDazedSpan::attempted_duration);check(first.pulses.size()==1);
 auto longer=clean;check(longer.apply(Kind::poison,6,100,10000));std::vector<FeveredContact> contacts{{a,0,true,true,poison},{b,322,true,true,longer},{{7,3,4},323,true,true,clean},{{7,4,5},1,true,false,clean},{{7,5,6},1,false,true,clean},{caster,1,true,true,clean}};auto recipients=fevered_recipients(first,contacts);check(recipients&&recipients->size()==1&&recipients->front().owner==b&&recipients->front().after.lookup(Kind::poison)->expires==10100&&!contacts[1].conditions.has(Kind::dazed,200));
 auto bad=contacts;bad.push_back({{7,2,99},1,true,true,clean});check(!fevered_recipients(first,bad));bad=contacts;bad[1].distance=-1;check(!fevered_recipients(first,bad));bad=contacts;bad[0].owner.generation++;check(!fevered_recipients(first,bad));
 // A real chain can return a NEW Dazed condition to the first target. Give
 // each committed condition event its own ID; stop once only renewals remain.
 auto bleed=clean;check(bleed.apply(Kind::bleeding,6,100,10000));auto to_b=*fevered_recipients(first,{{b,100,true,true,bleed}});auto second=*fevered_condition_reaction(bb.registry,bb.owner,{b,2,200,true},bleed,to_b[0].after,FeveredDazedSpan::attempted_duration);check(second.dazed_applied&&second.pulses.size()==2);
 auto to_a=*fevered_recipients(second,{{a,100,true,true,poison}});auto third=*fevered_condition_reaction(aa.registry,aa.owner,{a,3,200,true},poison,to_a[0].after,FeveredDazedSpan::attempted_duration);check(third.dazed_applied&&third.pulses.size()==2);
 auto again=*fevered_recipients(third,{{b,100,true,true,second.target}});auto end=*fevered_condition_reaction(bb.registry,bb.owner,{b,4,200,true},second.target,again[0].after,FeveredDazedSpan::attempted_duration);check(end.pulses.empty()&&!end.dazed_applied);
 FeveredEvents ledger_a(a),ledger_b(b);check(ledger_a.accept(first.event)&&ledger_b.accept(second.event)&&ledger_a.accept(third.event)&&ledger_b.accept(end.event)&&!ledger_a.accept(first.event));
 auto forged=first;forged.hex.skill=99;check(!fevered_recipients(forged,contacts));forged=first;forged.pulses[0].duration=0;check(!fevered_recipients(forged,contacts));
 std::cout<<"PASS Fevered322 boundary, alive/foe/self filtering, longer recipient timers, atomic invalid contacts and multi-hex new-condition chain termination\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

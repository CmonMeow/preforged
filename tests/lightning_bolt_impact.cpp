#include "gw_lightning_bolt.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>

using namespace fw::gw;
using skills::effects::LightningBoltImpact;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Lightning Bolt impact line "+std::to_string(at.line()));
}
combat::Actor target(double health=1000){
 combat::Actor a;a.id=2;a.generation=5;a.health=combat::Health{health,1000};a.damageable=true;a.lightning_armor=80;return a;
}
int main()try{
 check(!LightningBoltImpact::begin(21,false)&&!LightningBoltImpact::begin(12,std::nullopt));
 for(unsigned rank:{0u,12u,20u})for(bool moving:{false,true}){
  auto impact=*LightningBoltImpact::begin(rank,moving);auto a=target();
  const auto expected=double(5+3*rank); // AR80 with25% penetration resolves to AR60 at level20.
  const auto first=impact.strike(a,20,true);
  check(first.reason==combat::Reason::accepted&&first.damage==expected&&impact.remaining()==unsigned(moving));
  const auto second=impact.strike(a,20,true);
  check(moving?(second.reason==combat::Reason::accepted&&second.damage==expected):(second.reason==combat::Reason::invalid));
  check(a.health->current==1000-expected*(moving?2:1)&&!impact.remaining());
 }
 {auto impact=*LightningBoltImpact::begin(12,true);auto a=target();a.incoming_reduction=10;
  check(impact.strike(a,20,true).damage==31);
  // A reaction can change protection after the first hit. The second strike
  // reads current authority, rather than committing an earlier Actor proposal.
  a.incoming_reduction=20;check(impact.strike(a,20,true).damage==21&&a.health->current==948);}
 {auto impact=*LightningBoltImpact::begin(12,true);auto a=target(20);
  check(impact.strike(a,20,true).killed&&!impact.remaining());
  check(impact.strike(a,20,true).reason==combat::Reason::invalid&&a.health->current==0);}
 {auto impact=*LightningBoltImpact::begin(12,true);auto a=target();a.lightning_armor.reset();
  check(impact.strike(a,20,true).reason==combat::Reason::unknown_armor&&a.health->current==1000&&impact.remaining()==2);
  a.lightning_armor=80;check(impact.strike(a,20,false).reason==combat::Reason::not_hostile&&a.health->current==1000);
  check(impact.strike(a,0,true).reason==combat::Reason::invalid&&impact.remaining()==2);}
 std::cout<<"PASS Lightning Bolt distinct armored strikes,unknown motion/armor,per-hit reactions,lethal suppression; World integration remains pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

#include "gw_signet_judgment.h"
#include <iostream>
#include <source_location>
#include <string>
#include <limits>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Signet Judgment line "+std::to_string(at.line()));}
SignetJudgmentRecipient recipient(unsigned actor,double distance,unsigned plane=0){SignetJudgmentRecipient r;r.owner={1,actor,2};r.actor.id=actor;r.actor.generation=2;r.actor.health=combat::Health{1000,1000};r.actor.damageable=true;r.traits.light_sensitive=false;r.distance=distance;r.plane=plane;return r;}
int main()try{const Scope caster{1,1,1},selected{1,100,2};unsigned cases=0;
 for(bool pvp:{false,true})for(unsigned rank=0;rank<=20;++rank)for(unsigned mode=0;mode<3;++mode){++cases;const auto plan=*signet_judgment(rank,pvp);check(plan.damage==(pvp?5+3*rank:15+4*rank)&&plan.range==(pvp?624:1248)&&plan.radius==166&&plan.energy==0);
  auto primary=recipient(100,0);primary.knockdown_allowed=mode!=1;if(mode==2)primary.actor.health->current=1;
  auto nearby=recipient(101,166);nearby.traits.light_sensitive=true;auto outside=recipient(102,166.01);outside.actor.health.reset();outside.traits={};auto other_plane=recipient(103,0,1);other_plane.actor.health.reset();
  const std::vector<SignetJudgmentRecipient> inputs{primary,nearby,outside,other_plane};const auto result=prepare_signet_judgment(plan,caster,selected,20,plan.range,0,inputs);check(result&&result->size()==2);
  check(result->at(0).hit.damage==plan.damage&&result->at(1).hit.damage==2*plan.damage);check(result->at(0).knockdown_ms==(mode==0?2000:0)&&result->at(1).knockdown_ms==0);
  check(inputs[0].actor.health->current==(mode==2?1:1000)&&inputs[1].actor.health->current==1000);
 }
 const auto plan=*signet_judgment(12);auto primary=recipient(100,0);primary.knockdown_allowed=true;
 for(unsigned mode=0;mode<14;++mode){auto p=plan;auto one=primary;auto target=selected;auto source=caster;double range=100;unsigned plane=0;std::vector<SignetJudgmentRecipient> inputs;
  if(mode==0)++p.damage;if(mode==1)++p.radius;if(mode==2)one.knockdown_allowed.reset();if(mode==3)one.traits.light_sensitive.reset();if(mode==4)one.actor.health.reset();if(mode==5)one.actor.generation=3;
  if(mode==6)++target.generation;if(mode==7)source.generation=0;if(mode==8)one.distance=1;if(mode==9)range=1248.01;if(mode==10)range=std::numeric_limits<double>::quiet_NaN();if(mode==11)plane=1;if(mode==12)one.actor.damageable=false;
  inputs.push_back(one);if(mode==13)inputs.push_back(one);check(!prepare_signet_judgment(p,source,target,20,range,plane,inputs));
 }
 {auto one=primary;one.actor.incoming_damage_cap=20;auto result=prepare_signet_judgment(plan,caster,selected,20,100,0,{one});check(result&&result->front().hit.damage==20);one.already_down=true;one.knockdown_allowed.reset();result=prepare_signet_judgment(plan,caster,selected,20,100,0,{one});check(result&&result->front().knockdown_ms==0);}
 {auto unknown=recipient(101,10);unknown.actor.health=combat::Health{0,0};check(!prepare_signet_judgment(plan,caster,selected,20,100,0,{primary,unknown}));}
 check(!signet_judgment(21));check(!prepare_signet_judgment(*signet_judgment(12,true),caster,selected,20,625,0,{primary}));
 // Re-evaluating a copy-only proposal must not consume a caller-owned
 // stateful projection (for example a private one-use Reversal registry).
 {auto one=primary;one.actor.health->current=1;
  one.projection=[available=true](combat::Actor& actor,const combat::Damage& damage)mutable->std::optional<combat::DamageStep>{
   const auto raw=combat::apply(actor,damage,true);if(raw.reason!=combat::Reason::accepted)return {};
   if(available){actor.health->current=50;available=false;}
   return combat::DamageStep{raw,{}};
  };
  const std::vector<SignetJudgmentRecipient> inputs{one};
  for(unsigned repeat=0;repeat<2;++repeat){const auto result=prepare_signet_judgment(plan,caster,selected,20,100,0,inputs);check(result&&result->front().knockdown_ms==2000);}
  check(inputs.front().actor.health->current==1);
 }
 std::cout<<"PASS126 Signet Judgment PvE/PvP rank,crowd,holy-trait and primary-only control cases plus14 rejection cases and protection/range guards;World integration pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

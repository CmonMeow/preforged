#include "gw_ward_against_melee.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ward176 policy line "+std::to_string(at.line()));
}
int main()try{
 const Scope caster{1,2,3};const auto make=[&](unsigned rank=12){return ward_against_melee(caster,99,rank,0,100,200,100);};
 for(unsigned rank=0;rank<=20;++rank){const auto ward=make(rank);check(ward&&valid_ward_against_melee(*ward)&&ward->expires==5100+rank*1000);}
 check(!make(21)&&!ward_against_melee({},99,12,0,0,0,100)&&!ward_against_melee(caster,0,12,0,0,0,100));
 check(!ward_against_melee(caster,99,12,0,0,0,UINT64_MAX));
 check(!ward_against_melee(caster,99,12,0,std::numeric_limits<float>::infinity(),0,100));
 const auto ward=*make();WardMeleeRecipient recipient{{1,5,7},0,100,200,true,false,true};
 check(ward_melee_covers(ward,recipient,100)&&ward_melee_covers(ward,recipient,17099)&&!ward_melee_covers(ward,recipient,17100));
 check(!ward_melee_covers(ward,recipient,99));recipient.x=422;check(ward_melee_covers(ward,recipient,100));
 recipient.x=422.01f;check(!ward_melee_covers(ward,recipient,100));recipient.x=100;
 auto denied=recipient;denied.spirit=true;check(!ward_melee_covers(ward,denied,100));
 denied=recipient;denied.authoritative_ally=false;check(!ward_melee_covers(ward,denied,100));
 denied=recipient;denied.living=false;check(!ward_melee_covers(ward,denied,100));
 denied=recipient;++denied.scope.instance;check(!ward_melee_covers(ward,denied,100));
 denied=recipient;++denied.plane;check(!ward_melee_covers(ward,denied,100));
 denied=recipient;denied.scope.generation=0;check(!ward_melee_covers(ward,denied,100));
 auto forged=ward;++forged.expires;check(!valid_ward_against_melee(forged)&&!ward_melee_covers(forged,recipient,100));
 // The current caster may benefit, but no implicit ally inference is made.
 denied=recipient;denied.scope=caster;check(ward_melee_covers(ward,denied,100));
 using Delivery=combat::weapon::AttackDelivery;using Outcome=combat::weapon::AttackOutcome;
 const auto source=ward_melee_block_source(ward,recipient,Delivery::melee,100);check(source&&source->skill==176&&source->instance==99&&source->probability==.5);
 check(!ward_melee_block_source(ward,recipient,Delivery::ranged,100)&&!ward_melee_block_source(ward,recipient,Delivery::unknown,100));
 check(!ward_melee_block_source(ward,recipient,static_cast<Delivery>(99),100));
 const BlockSource sources[]{*source,{258,100,.5}};
 unsigned draws=0;const auto draw=[&](const BlockSource&){++draws;return 0.;};
 auto contact=resolve_attributed_block(Outcome::hit,sources,false,{},100,draw);
 check(contact&&contact->outcome==Outcome::block&&contact->source->skill==176&&draws==1);
 for(const auto prior:{Outcome::miss,Outcome::dodge,Outcome::obstruction,Outcome::cancelled}){
  draws=0;contact=resolve_attributed_block(prior,sources,false,{},100,draw);check(contact&&contact->outcome==prior&&!contact->source&&!draws);}
 draws=0;contact=resolve_attributed_block(Outcome::hit,sources,true,{},100,draw);check(contact&&contact->outcome==Outcome::hit&&!draws);
 const Entry rigor{1,3,137,12,Category::hex,100,18100};
 contact=resolve_attributed_block(Outcome::hit,sources,false,std::span(&rigor,1),100,draw);check(contact&&contact->outcome==Outcome::hit&&!draws);
 // Environmental effect IDs belong to caster lifetime, not the recipient's
 // weapon-spell registry; equal numeric IDs still yield distinct attribution.
 const BlockSource collision[]{{793,99,.5},*source};
 for(double value:{.1,.6,.9}){const auto result=resolve_attributed_block_draw(collision,value);
  check(result&&result->outcome==(value<.75?Outcome::block:Outcome::hit));
  if(value<.75)check(result->source->skill==(value<.5?793u:176u));}
 auto other=*source;other.owner.actor=9;const BlockSource distinct[]{*source,other};
 check(bool(resolve_attributed_block_draw(distinct,.6)));
 const BlockSource duplicate[]{*source,*source};check(!resolve_attributed_block_draw(duplicate,.1));
 const double chances[]{.5,.5};check(attack_block_probability(chances,false,{},100)==.75);
 check(combat::weapon::weapon_delivery(27,false)==Delivery::melee&&combat::weapon::weapon_delivery(5,false)==Delivery::ranged);
 std::cout<<"Ward176 canonical lifetime, area/ally/spirit eligibility, melee-only block attribution and existing bypass composition passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

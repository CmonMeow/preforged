#include "gw_weapon_attack_skills.h"
#include <stdexcept>
#include <string>
#include <source_location>
#include <limits>
using namespace fw::gw::combat::weapon;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Earth Shaker line "+std::to_string(at.line()));
}
int main(){
 const auto low=earth_shaker(4),high=earth_shaker(5);
 check(low&&high&&!earth_shaker(21));
 check(*earth_shaker_fails(*low,0)&&*earth_shaker_fails(*low,.499999));
 check(!*earth_shaker_fails(*low,.5)&&!*earth_shaker_fails(*high,0));
 check(!earth_shaker_fails(*high,1)&&!earth_shaker_fails(*high,-.1));
 check(!earth_shaker_fails(*high,std::numeric_limits<double>::quiet_NaN()));
 using Scope=fw::gw::skills::effects::Scope;
 const Scope source{1,1,1},primary{1,2,1},edge{1,3,1};
 std::vector<EarthShakerRecipient> actors{{source,-166,0,true},{primary,0,0,true},
  {edge,166,0,true},{{1,4,1},166.001,0,true},{{1,5,1},0,0,false},{{1,6,1},-167,0,true}};
 auto chosen=earth_shaker_recipients(*high,source,primary,AttackOutcome::hit,false,actors);
 check(chosen&&*chosen==std::vector<Scope>({primary,edge}));
 for(auto outcome:{AttackOutcome::miss,AttackOutcome::block,AttackOutcome::dodge,AttackOutcome::obstruction,
                   AttackOutcome::target_retired,AttackOutcome::cancelled,AttackOutcome::replaced}){
  chosen=earth_shaker_recipients(*high,source,primary,outcome,false,actors);check(chosen&&chosen->empty());
 }
 chosen=earth_shaker_recipients(*low,source,primary,AttackOutcome::hit,true,actors);check(chosen&&chosen->empty());
 actors[1].eligible=false; // lethal primary hit still centers the splash
 chosen=earth_shaker_recipients(*high,source,primary,AttackOutcome::hit,false,actors);
 check(chosen&&*chosen==std::vector<Scope>({edge}));actors[1].eligible=true;
 auto corrupt=*high;corrupt.adjacent=1000;
 check(!earth_shaker_recipients(corrupt,source,primary,AttackOutcome::hit,false,actors));
 check(!earth_shaker_recipients(*high,source,{1,2,2},AttackOutcome::hit,false,actors));
 actors.push_back(actors[2]);check(!earth_shaker_recipients(*high,source,primary,AttackOutcome::hit,false,actors));actors.pop_back();
 actors[2].scope.instance=2;check(!earth_shaker_recipients(*high,source,primary,AttackOutcome::hit,false,actors));actors[2].scope.instance=1;
 actors[2].x=std::numeric_limits<double>::infinity();check(!earth_shaker_recipients(*high,source,primary,AttackOutcome::hit,false,actors));
 // Canonical weapon integration: no bonus damage, one hammer primary, no
 // generic fallback when the owning World has not supplied failure facts.
 const auto attack=earth_shaker_attack_plan(12),weak_attack=earth_shaker_attack_plan(4);
 check(attack&&weak_attack&&valid(*attack)&&attack->required_type==15&&attack->bonus==0&&attack->knockdown_on_hit);
 check(attack_skill(354,12)&&attack_skill(354,0)); // World now owns failure and area completion.
 check(!earth_shaker_attack_plan(21)&&!earth_shaker_attack_plan(12,21));
 Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=30;strike.location_armor=60;
 check(!resolve_attack_skill(strike,*attack,false));
 ConditionContact contact;contact.earth_shaker_failure_roll=.5;
 auto hit=resolve_attack_skill(strike,*attack,false,false,false,false,contact);
 check(hit&&!hit->missed&&!hit->skill_failed&&hit->damage==30&&hit->refund==0&&hit->adrenaline==0);
 check(attack_knockdown(*attack,false,contact)==2000&&!attack_condition(*attack,false,contact)&&condition_duration(*attack)==0);
 check(attack_knockdown(*attack,false)==0&&!blocked_attack_effect(*attack,AttackOutcome::block));
 contact.earth_shaker_failure_roll=.499;
 hit=resolve_attack_skill(strike,*weak_attack,false,false,false,false,contact);
 check(hit&&hit->missed&&hit->skill_failed&&hit->damage==0&&hit->refund==0&&hit->adrenaline==0);
 check(attack_knockdown(*weak_attack,false,contact)==0);
 contact.earth_shaker_failure_roll=.5;strike.blind=true;strike.miss_roll=0;
 hit=resolve_attack_skill(strike,*weak_attack,false,false,false,false,contact);
 check(hit&&hit->missed&&!hit->skill_failed&&hit->damage==0);
 contact.earth_shaker_failure_roll=std::numeric_limits<double>::quiet_NaN();
 check(!resolve_attack_skill(strike,*attack,false,false,false,false,contact));
 actors[2].x=166;contact.earth_shaker_failure_roll=.5;
 chosen=earth_shaker_attack_recipients(*attack,source,primary,AttackOutcome::hit,contact,actors);
 check(chosen&&*chosen==std::vector<Scope>({primary,edge}));
 for(auto outcome:{AttackOutcome::block,AttackOutcome::miss,AttackOutcome::replaced}){
  chosen=earth_shaker_attack_recipients(*attack,source,primary,outcome,contact,actors);check(chosen&&chosen->empty());
 }
 contact.earth_shaker_failure_roll=0;
 chosen=earth_shaker_attack_recipients(*weak_attack,source,primary,AttackOutcome::hit,contact,actors);check(chosen&&chosen->empty());
 contact.earth_shaker_failure_roll.reset();check(!earth_shaker_attack_recipients(*attack,source,primary,AttackOutcome::hit,contact,actors));
 auto forged=*attack;forged.bonus=50;
 check(!valid(forged)&&!resolve_attack_skill(strike,forged,false,false,false,false,contact));
}


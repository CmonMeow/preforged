#pragma once
#include "gw_weapon_attack.h"
#include "gw_dagger_attack_policy.h"
#include "gw_dagger_family_conditions_detached.h"
#include "gw_wild_blow.h"
#include "gw_skull_crack.h"
#include "gw_conditions.h"
#include "gw_attack_outcomes.h"
#include "gw_attack_interrupts.h"
#include "gw_dazed_interrupts.h"
#include "gw_precision_shot.h"
#include "gw_debilitating_shot.h"
#include "gw_determined_shot.h"
#include "gw_barrage.h"
#include "gw_distracting_shot.h"
#include "gw_earth_shaker.h"
#include "gw_bow_self_disable.h"
#include "gw_screaming_shot.h"
#include "gw_arcing_forked_arrows.h"
#include <cstdint>
namespace fw::gw::combat::weapon {
struct AttackPlan {unsigned skill=0,rank=0,bonus=0,refund=0;std::uint64_t windup_ms=500;bool attacking_refund=false;bool moving_bonus=false;unsigned strength=0,required_type=0;unsigned arrows=1,base_percent=100,fixed_damage=0;bool knockdown_on_hit=false;unsigned projectile_speed_percent=100,armor_penetration=0,health_advantage_adrenaline=0,hit_adrenaline=0,blocked_adrenaline=0;unsigned stance_bonus=0,copy=0;bool hit_refund=false,weakness_unblockable=false,no_stance_unblockable=false,shout_unblockable=false;unsigned critical_bonus=0;bool critical_interrupt=false,requires_critical_authority=false;bool operator==(const AttackPlan&)const=default;};
// Only actual scoped duration records qualify. An instant shout use has no
// implied lifetime; unimplemented chant casting is not inferred from this query.
inline bool active_shout_or_chant(const skills::effects::Registry& registry,skills::effects::Scope owner,skills::effects::Time now){
 for(const auto& e:registry.active(owner,now)){const auto* row=skills::find_skill(e.skill);
  if(row&&(row->type==15||row->type==27)&&e.kind==skills::effects::category(*row))return true;
 }return false;
}
inline std::optional<AttackPlan> attack_skill(unsigned id,unsigned rank,unsigned strength=UINT32_MAX,unsigned copy=0,bool dagger_owner=false){
 if((dagger_role(id)!=0)&&!dagger_owner)return {};
 // World owns Earth Shaker's independent failure draw and area completion.
 if(rank>20||copy>65535)return {};
 if(strength==UINT32_MAX)strength=(id==322||id==326||id==332||id==1693)?rank:0;
 if(strength>20)return {};
 std::optional<AttackPlan> p;
 if(dagger_role(id)&&dagger_role(id)!=780&&dagger_role(id)!=782){
  const auto family=detached_dagger_family_plan(id,rank);if(!family)return {};
  p=AttackPlan{id,rank,family->bonus,0,500};
 }
 if(dagger_role(id)==780||dagger_role(id)==782){const auto dagger=dagger_attack_policy(id,rank);if(!dagger)return {};
  p=AttackPlan{id,rank,dagger->bonus,0,dagger->windup_ms};
 }
 if(id==1720||id==3147||id==1402){const auto* r=skills::find_skill(id);const bool chop=id==1402,pvp=id==3147;
  if(!r||r->profession!=(chop?1u:2u)||r->attribute!=(chop?18u:25u)||r->type!=14||r->special!=(pvp?4194304u:0u)||r->energy_cost()!=5||r->adrenaline||r->activation!=(chop?1.f:0.f)||r->aftercast||r->recharge!=(chop?15u:pvp?12u:6u)||r->weapon_requirement!=(chop?1u:2u)||r->scale0!=(pvp?1u:5u)||r->scale15!=(pvp?5u:20u)||r->bonus0!=(chop?0u:5u)||r->bonus15!=(chop?0u:pvp?30u:25u)||r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
  p=AttackPlan{id,rank,static_cast<unsigned>(std::lround((pvp?1.:5.)+(pvp?4.:15.)*rank/15.)),0,chop?1000u:500u};p->requires_critical_authority=true;p->critical_interrupt=chop;p->critical_bonus=chop?0:static_cast<unsigned>(std::lround(5.+(pvp?25.:20.)*rank/15.));
 }
 if(id==904||id==905||id==1409){const auto* r=skills::find_skill(id);const bool axe=id==904;
  if(!r||r->profession!=1||r->type!=14||r->attribute!=(axe?18u:19u)||r->special||r->health_sacrifice||
     r->activation!=0||r->aftercast!=0||r->energy_cost()!=(id==905?0u:5u)||r->adrenaline!=(id==905?120u:0u)||
     r->recharge!=(id==904?6u:id==1409?12u:0u)||r->weapon_requirement!=(axe?1u:16u)||
     r->scale0!=5||r->scale15!=(axe?35u:20u)||r->bonus0!=(axe?75u:id==905?3u:2u)||
     r->bonus15!=(axe?75u:id==905?8u:2u)||r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
  p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(5.+(axe?30.:15.)*rank/15.)),0,500};
  if(id==905){p->refund=static_cast<unsigned>(std::lround(3.+5.*rank/15.));p->hit_refund=true;p->weakness_unblockable=true;}
  if(id==1409)p->hit_adrenaline=2;if(id==904)p->blocked_adrenaline=3;
 }
 if(id==996||id==1135){const auto* r=skills::find_skill(id);const bool standing=id==996;
  if(!r||r->profession!=1||r->type!=14||r->attribute!=20||r->special||r->health_sacrifice||
     r->activation!=0||r->aftercast!=0||r->energy_cost()!=(standing?0u:5u)||r->adrenaline!=(standing?140u:0u)||
     r->recharge!=(standing?0u:8u)||r->weapon_requirement!=128||r->scale0!=(standing?5u:1u)||
     r->scale15!=(standing?20u:30u)||r->bonus0!=(standing?5u:0u)||r->bonus15!=(standing?20u:0u)||
     r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
  p=AttackPlan{id,rank,standing?5+rank:static_cast<unsigned>(std::lround(1.+29.*rank/15.)),0,500};
  if(standing)p->stance_bonus=5+rank;else p->no_stance_unblockable=true;
 }
 if(id==1695){const auto* r=skills::find_skill(id);
  if(!r||r->profession!=1||r->type!=14||r->attribute!=21||r->special||r->health_sacrifice||
     r->activation!=0||r->aftercast!=0||r->energy_cost()!=5||r->adrenaline||r->recharge!=4||
     r->weapon_requirement!=185||r->scale0!=10||r->scale15!=40||r->bonus0||r->bonus15||
     r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
  p=AttackPlan{id,rank,10+2*rank,0,500};p->shout_unblockable=true;
 }
 if(id==908||id==909){const auto shot=bow_self_disable(id,rank,copy);if(!shot)return {};p=AttackPlan{id,rank,shot->bonus,0,500};}
 if(id==1191||id==2864){const auto* r=skills::find_skill(id);const bool pvp=id==2864;
  if(!r||r->profession!=2||r->attribute!=25||r->type!=14||r->special!=(pvp?4194304u:0u)||
     r->energy_cost()!=10||r->adrenaline||r->health_sacrifice||r->overcast!=5||r->recharge!=4||
     r->activation!=(pvp?1.f:.75f)||r->aftercast!=(pvp?1.f:.6000000238418579f)||
     r->pvp_skill!=(pvp?1191u:2864u)||r->weapon_requirement!=2||r->target!=5||
     r->scale0!=(pvp?3u:5u)||r->scale15!=(pvp?10u:25u)||
     r->bonus0!=10||r->bonus15!=10||r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
  p=AttackPlan{id,rank,static_cast<unsigned>(std::lround((pvp?3.:5.)+(pvp?7.:20.)*rank/15.)),0,pvp?1000u:750u};
  p->armor_penetration=10;
 }
 if(id==1719){const auto shot=screaming_shot(rank);if(!shot)return {};p=AttackPlan{id,rank,shot->bonus,0,500};}
 if(id==1467){const auto shot=arcing_shot(rank);if(!shot)return {};p=AttackPlan{id,rank,shot->bonus,0,500};p->projectile_speed_percent=shot->speed_percent;}
 if(id==1722){if(!forked_arrow())return {};p=AttackPlan{id,rank,0,0,500};p->arrows=2;}
 if(id==329){if(!skull_crack())return {};p=AttackPlan{id,rank,0,0,500};}
 if(id==321){if(!wild_blow())return {};p=AttackPlan{id,rank,0,0,500};}
 if(id==331)p=AttackPlan{id,rank,0,0,500};
 if(id==328)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(1.+29.*rank/15.)),0,500};
 if(id==327||id==356||id==1134)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(5.+15.*rank/15.)),0,500};
 if(id==341||id==386)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(1.+19.*rank/15.)),0,500};
 if(id==330)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(4.+8.*rank/15.)),0,500};
 if(id==394)p=AttackPlan{id,rank,0,0,500};
 if(id==399){const auto shot=distracting_shot(rank);if(!shot)return {};p=AttackPlan{id,rank,0,0,shot->release};p->fixed_damage=shot->damage;}
 if(id==396)p=AttackPlan{id,rank,0,0,500};
 if(id==1953||id==2096){
  const auto* row=skills::find_skill(id);
  if(!row||rank>12||row->profession!=2||row->attribute!=51||row->type!=14||
     row->special!=524288||row->energy_cost()!=10||row->adrenaline||row->activation||
     row->aftercast||row->recharge!=10||row->weapon_requirement!=2||
     row->scale0!=40||row->scale15!=25||row->caster_body_resource!=2077||
     row->target_body_resource!=2077)return {};
  p=AttackPlan{id,rank,0,0,500};p->required_type=5;p->arrows=3;
  // The live skill's base attack damage is ten percentage points below its
  // displayed reduction. Bonus damage is handled after this multiplier.
  p->base_percent=50u+static_cast<unsigned>(std::lround(15.*std::min(rank,6u)/6.));
 }
 if(id==397)p=AttackPlan{id,rank,0,0,1000};
 if(id==403)p=AttackPlan{id,rank,0,0,500}; // Called Shot uses ordinary bow activation.
 if(id==391)p=AttackPlan{id,rank,0,0,1000}; // Pinned Hunter Shot activation1s, not ordinary500ms windup.
 if(id==392)p=AttackPlan{id,rank,0,0,500};
 if(id==395||id==2144){const auto shot=barrage(rank,id);if(!shot)return {};p=AttackPlan{id,rank,shot->bonus,0,500};}
 if(id==402){const auto shot=determined_shot(rank);if(!shot)return {};p=AttackPlan{id,rank,shot->bonus,0,500};}
 if(id==406){if(!debilitating_shot(rank))return {};p=AttackPlan{id,rank,0,0,500};}
 if(id==400){const auto p400=precision_shot(rank);if(!p400)return {};p=AttackPlan{id,rank,p400->bonus,0,p400->windup};}
 if(id==408){const auto c=concussion_shot(rank);if(!c)return {};p=AttackPlan{id,rank,0,0,c->windup};p->fixed_damage=c->damage;}
 if(id==409){const auto s=punishing_shot(rank);if(!s)return {};p=AttackPlan{id,rank,s->bonus,0,s->windup};}
 if(id==426){const auto s=savage_shot(rank);if(!s)return {};p=AttackPlan{id,rank,s->bonus,0,s->windup};}
 if(id==390){const auto s=savage_slash(rank);if(!s)return {};p=AttackPlan{id,rank,s->bonus,0,s->windup};}
 if(id==393){const auto* r=skills::find_skill(393);if(!r||r->type!=14||r->attribute!=25||r->energy_cost()!=10||r->recharge!=4||r->bonus0!=1||r->bonus15!=11)return {};p=AttackPlan{id,rank,0,0,500};}
 if(id==404)p=AttackPlan{id,rank,0,0,500};
 if(id==407)p=AttackPlan{id,rank,10+2*rank,0,500};
 if(id==322)p=AttackPlan{id,rank,10+2*rank,0,500,false};
 if(id==326)p=AttackPlan{id,rank,10+2*rank,0,500,false,true};
 if(id==340){const auto* r=skills::find_skill(id);
  if(!r||r->type!=14||r->attribute!=18||r->adrenaline!=150||r->activation!=0||r->recharge!=0||r->scale0!=20||r->scale15!=20)return {};
  p=AttackPlan{id,rank,0,0,500};p->required_type=2;
 }
 if(id==324){const auto* r=skills::find_skill(id);
  if(!r||r->type!=14||r->attribute!=21||r->energy_cost()!=5||r->recharge!=8||r->activation!=0||r->scale0!=20||r->scale15!=40||r->bonus0!=1||r->bonus15!=2||r->weapon_requirement!=185)return {};
  p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(20.+20.*rank/15.)),0,500};p->health_advantage_adrenaline=static_cast<unsigned>(std::lround(1.+rank/15.));
 }
 if(id==332){const auto* row=skills::find_skill(id);
  if(!row||row->type!=14||row->attribute!=17||row->energy_cost()!=5||row->recharge!=10||row->activation!=0||
     row->scale0!=5||row->scale15!=30||row->duration0!=2||row->duration15!=2||row->weapon_requirement!=185||
     row->caster_body_resource!=2077||row->target_body_resource!=2077)return {};
  p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(5.+25.*rank/15.)),0,500,false,true};p->knockdown_on_hit=true;
 }
 if(id==1693)p=AttackPlan{id,rank,5+2*rank,static_cast<unsigned>(std::lround(2.+4.*rank/15.)),500,true};
 if(id==335)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(10.+20.*rank/15.)),0,500};
 if(id==336||id==351)p=AttackPlan{id,rank,10+2*rank,0,500};
 if(id==337||id==338)p=AttackPlan{id,rank,id==338?1+2*rank:0u,0,500};
 if(id==339||id==1136)p=AttackPlan{id,rank,5+rank,0,500};
 if(id==383||id==1144)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(1.+39.*rank/15.)),0,500};
 if(id==385)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(1.+39.*rank/15.)),0,500};
 if(id==382)p=AttackPlan{id,rank,0,0,500};
 if(id==384)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(5.+rank)),0,500};
 // Seven reviewed condition attacks. Damage/condition endpoints are pinned
 // 2022 rows; predicates are from the historical individual skill articles.
 if(id==320)p=AttackPlan{id,rank,0,0,500};
 if(id==334)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(1.+9.*rank/15.)),0,500};
 if(id==342||id==352)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(1.+19.*rank/15.)),0,500};
 if(id==849||id==850)p=AttackPlan{id,rank,5+rank,0,500};
 if(id==1416)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(5.+25.*rank/15.)),0,500};
 if(id==354){if(!earth_shaker(rank))return {};p=AttackPlan{id,rank,0,0,500};}
 if(id==355||id==357)p=AttackPlan{id,rank,0,0,500};
 if(id==358)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(1.+19.*rank/15.)),0,500};
 if(id==359)p=AttackPlan{id,rank,static_cast<unsigned>(std::lround(1.+29.*rank/15.)),0,500};
 if(id==360)p=AttackPlan{id,rank,0,0,1000}; // Native fixed1s attack activation.
 if(p){p->copy=copy;p->strength=strength;p->required_type=(id==1191||id==2864||id==1720||id==3147||id==1467||id==1722||id==1719||id==908||id==909||id==399||id==2144||id==395||id==402||id==406||id==400||id==408||id==409||id==426||id==394||id==396||id==407||id==404||id==391||id==392||id==393||id==397||id==403)?5:(id==1402||id==904||id==340||id==341||id==330||id==334||id==335||id==336||id==337||id==338||id==339||id==342||id==849||id==1136)?2:(id==905||id==1409||id==356||id==331||id==351||id==352||id==354||id==355||id==357||id==358||id==359||id==360||id==850)?15:(id==996||id==1135||id==390||id==386||id==328||id==320||id==382||id==384||id==385||id==383||id==1144||id==1416)?27:0;if(id==397)p->projectile_speed_percent=200;if(id==403)p->projectile_speed_percent=300;if(id==339||id==1136)p->armor_penetration=20;if(id==331||id==354||id==355||id==357||id==358||id==359)p->knockdown_on_hit=true;if(id==396){p->arrows=2;p->base_percent=75;}if(id==394)p->fixed_damage=static_cast<unsigned>(std::lround(25.+25.*rank/15.));if(id==1953||id==2096)p->required_type=5;if(dagger_role(id)!=0)p->required_type=32;}return p;
}
inline unsigned condition_duration(const AttackPlan& p){return !attack_skill(p.skill,p.rank,p.strength,p.copy,true)?0:(p.skill==776||p.skill==778||p.skill==782)?(5+p.rank)*1000:p.skill==1719?(5+p.rank)*1000:p.skill==334?15000:p.skill==342?20000:p.skill==850?static_cast<unsigned>(std::lround(1.+7.*p.rank/15.))*1000:p.skill==1416?static_cast<unsigned>(std::lround(5.+10.*p.rank/15.))*1000:p.skill==393?static_cast<unsigned>(std::lround(1.+10.*p.rank/15.))*1000:p.skill==391?static_cast<unsigned>(std::lround(3.+22.*p.rank/15.))*1000:(p.skill==392||p.skill==320)?static_cast<unsigned>(std::lround(3.+12.*p.rank/15.))*1000:p.skill==404?(5+p.rank)*1000:p.skill==382?static_cast<unsigned>(std::lround(5.+20.*p.rank/15.))*1000:(p.skill==337||p.skill==338||p.skill==384||p.skill==352||p.skill==355||p.skill==360||p.skill==849)?(5+p.rank)*1000:0;}
inline std::optional<AttackPlan> earth_shaker_attack_plan(unsigned rank,unsigned strength=0){return attack_skill(354,rank,strength);}
inline bool valid(const AttackPlan& p,bool dagger_owner=false){const auto actual=attack_skill(p.skill,p.rank,p.strength,p.copy,dagger_owner);return actual&&*actual==p;}
// Facts sampled by World from the authoritative actor at weapon contact.
// Input flags, the selected client target and the state at cast start are not
// proof that a condition/knockdown/stance still exists when this attack hits.
struct ConditionContact {
 bool deep_wound=false,weakness=false,knocked_down=false,attacker_stance=false;
 std::optional<bool> weakness_at_start;
 bool target_spell=false;
 bool target_moving=false;
 std::optional<bool> attacker_health_greater;
 // One authoritative independent roll retained for this Earth Shaker use.
 // Required even above rank4 so old single-target paths cannot silently admit
 // this area-control family without its owner supplying the new contact facts.
 std::optional<double> earth_shaker_failure_roll;
};
inline std::optional<std::vector<skills::effects::Scope>> earth_shaker_attack_recipients(
 const AttackPlan& attack,skills::effects::Scope source,skills::effects::Scope primary,
 AttackOutcome outcome,const ConditionContact& contact,std::span<const EarthShakerRecipient> candidates){
 if(!valid(attack)||attack.skill!=354||!contact.earth_shaker_failure_roll)return {};
 const auto p=earth_shaker(attack.rank);if(!p)return {};
 const auto failed=earth_shaker_fails(*p,*contact.earth_shaker_failure_roll);if(!failed)return {};
 return earth_shaker_recipients(*p,source,primary,outcome,*failed,candidates);
}
struct BlockEffect {unsigned damage=0;std::uint64_t knockdown_ms=0,condition_ms=0;std::optional<conditions::Kind> condition;};
inline std::optional<BlockEffect> blocked_attack_effect(const AttackPlan& p,AttackOutcome outcome){
 if(!valid(p)||outcome!=AttackOutcome::block)return {};
 if(p.skill==327||p.skill==1134)return BlockEffect{static_cast<unsigned>(std::lround(10.+24.*p.rank/15.)),2000,0,{}};
 if(p.skill==356)return BlockEffect{p.bonus,2000,0,{}};
 if(p.skill==341)return BlockEffect{p.bonus,0,20000,conditions::Kind::deep_wound};
 if(p.skill==386)return BlockEffect{p.bonus,0,25000,conditions::Kind::bleeding};
 return {};
}
inline std::uint64_t attack_knockdown(const AttackPlan& p,bool attacking,ConditionContact contact={}){
 if(!valid(p)||!p.knockdown_on_hit)return 0;
 if(p.skill==354){
  if(!contact.earth_shaker_failure_roll)return 0;
  const auto failed=earth_shaker_fails(*earth_shaker(p.rank),*contact.earth_shaker_failure_roll);
  if(!failed||*failed)return 0;
 }
 if(p.skill==357&&!attacking)return 0;
 if(p.skill==359&&!contact.weakness)return 0;
 if(p.skill==332&&!contact.target_moving)return 0;
 return p.skill==358&&p.strength>=8?4000:2000;
}
inline std::optional<conditions::Kind> attack_condition(const AttackPlan& p,bool bleeding,ConditionContact contact={}){
 if(!valid(p))return {};
 if(p.skill==320||(p.skill==334&&contact.deep_wound))return conditions::Kind::crippled;
 if((p.skill==342&&contact.deep_wound)||p.skill==355||p.skill==360)return conditions::Kind::weakness;
 if(p.skill==382||(p.skill==849&&contact.knocked_down)||(p.skill==1416&&!contact.attacker_stance))return conditions::Kind::bleeding;
 if(p.skill==337||p.skill==338||(p.skill==384&&bleeding)||(p.skill==352&&contact.knocked_down)||(p.skill==850&&contact.weakness))return conditions::Kind::deep_wound;
 return {};
}
struct SkillHit {bool missed=false;double damage=0;unsigned refund=0,adrenaline=0;bool skill_failed=false;bool critical=false,critical_energy=false;};
inline std::optional<SkillHit> resolve_attack_skill(Strike base,const AttackPlan& plan,bool target_attacking,bool target_moving=false,bool target_bleeding=false,bool target_below_half=false,ConditionContact contact={},bool dagger_owner=false){
 if(!valid(plan,dagger_owner)||base.caster_weapon||(plan.skill==359&&!contact.weakness_at_start)||(plan.health_advantage_adrenaline&&!contact.attacker_health_greater))return {};
 if(plan.skill==354){
  if(!contact.earth_shaker_failure_roll)return {};
  const auto failed=earth_shaker_fails(*earth_shaker(plan.rank),*contact.earth_shaker_failure_roll);
  if(!failed)return {};
  if(*failed)return SkillHit{true,0,0,0,true};
 }
 // Power Shot replaces weapon damage with fixed armor-ignoring damage. Local
 // Power_Shot/Talk_Power_Shot pre-2022 records; no armor/mastery/Weakness factor.
 if(plan.fixed_damage){
  if(!std::isfinite(base.miss_roll)||base.miss_roll<0||base.miss_roll>=1)return {};
  const auto missed=attack_misses(base.miss_roll,base.blind,base.additional_miss_chance);if(!missed)return {};
  return *missed?SkillHit{true,0,0}:SkillHit{false,double(plan.fixed_damage),0};
 }
 // Strength's inherent armor penetration applies to attack-skill weapon damage;
 // the separate bonus remains armor ignoring and unaffected by Weakness.
 // Skill base penetration and Strength do not stack: only the highest applies.
 const auto base_penetration=std::max(plan.strength,plan.armor_penetration);
 const auto penetration=base.location_armor>0?std::exp2(base.location_armor*base_penetration*.01/40.):1.;
 if(plan.skill==321)base.critical=true;
 const auto hit=resolve(base);if(!hit)return {};if(hit->missed)return SkillHit{true,0,0};
 return SkillHit{false,hit->damage*penetration*plan.base_percent/100.+(((plan.skill!=390&&plan.skill!=426)||contact.target_spell)&&(!plan.moving_bonus||target_moving)&&(plan.skill!=384||target_bleeding)&&((plan.skill!=334&&plan.skill!=342)||contact.deep_wound)&&(plan.skill!=359||*contact.weakness_at_start)?plan.bonus*(plan.skill==385&&target_below_half?2u:1u):0)+(contact.attacker_stance?plan.stance_bonus:0)+(hit->critical?plan.critical_bonus:0),(plan.hit_refund||(plan.attacking_refund&&target_attacking))?plan.refund:0,plan.hit_adrenaline+(plan.health_advantage_adrenaline&&*contact.attacker_health_greater?plan.health_advantage_adrenaline:0),false,hit->critical,hit->critical_energy};
}
}



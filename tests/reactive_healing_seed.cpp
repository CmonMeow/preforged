#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Reactive Seed line "+std::to_string(at.line()));}
constexpr unsigned families[]={327,341,356,386,1134,10,19,28,100,43,47};
struct Fixture{
 world::Instance w;effects::Scope foe,target,monk;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();unsigned seed_rank=12;
 Fixture(unsigned hp=500,unsigned rank=12,bool far=false,bool unknown_npc=false):seed_rank(rank){if(unknown_npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=80;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;d.healing_allowed=std::nullopt;n.combat_definition=d;w.configure_npcs({n});}for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:far?700:100,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==1?1000u:id==2?hp:500u,1000,100,100},100);check(w.physical_armor(*w.effect_scope(id),60));}foe=*w.effect_scope(1);target=*w.effect_scope(2);monk=*w.effect_scope(3);check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));check(w.complete_healing_seed(monk,target,*effects::healing_seed(rank),0,100));}
 void rr(int rank){if(rank>=0)check(w.status_mutation(target,307,900,rank,100,8100,true));}
 void setup(unsigned family){
  if(family==10){check(w.hex_breaker(foe,*effects::hex_breaker(12),20,100));return;}
  if(family==19||family==43||family==47){check(w.degeneration_hex(foe,target,*effects::degeneration_hex(family,12),100));return;}
  if(family==28){check(w.backfire(foe,target,*effects::backfire(28,12),20,100));return;}
  if(family==100){check(w.soul_barbs(foe,target,*effects::soul_barbs(12),20,100));return;}
  check(w.apply_health_regeneration(target,2,*skills::regeneration::reviewed(258,12),100,6100,1248,true,target));w.configure_attack_block_draws([]{return 0.;});
 }
 bool fire(unsigned family){
  if(family==10)return w.backfire(target,foe,*effects::backfire(28,0),20,200);
  if(family==19)return w.apply_condition(foe,target,combat::conditions::Kind::blind,200,1000);
  if(family==28){w.before_spell_effect(target,281,1,200);return true;}
  if(family==100)return w.backfire(foe,target,*effects::backfire(28,0),20,200);
  if(family==43||family==47){w.attack_activity(target,1000,200);return true;}
  const auto plan=*combat::weapon::attack_skill(family,12);combat::weapon::Owned arm{12,1,{20,20,2},12,false,false,plan.required_type?plan.required_type:27u};check(w.ordinary_weapon(foe,arm,20));return w.melee_attack(foe,target,arm,plan,200);
 }
 unsigned hp(){return w.resources(2)->health;}
};
unsigned damage(unsigned family){if(family==10)return effects::hex_breaker(12)->damage;if(family==19)return effects::fragility(19,12)->damage;if(family==28)return effects::backfire(28,12)->damage;if(family==100)return effects::soul_barbs(12)->damage;if(family==43||family==47)return effects::attack_hex(family,12)->damage;return combat::weapon::blocked_attack_effect(*combat::weapon::attack_skill(family,12),combat::weapon::AttackOutcome::block)->damage;}
unsigned word(const world::Packet& p,unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);}
#include "reactive_seed_deadline_cases.inc"
int main()try{
 unsigned cases=0;
 for(unsigned family:families)for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u})for(unsigned rank:{0u,12u,20u}){
  ++cases;Fixture f(hp,rank);f.rr(rr);f.setup(family);f.box->drain();check(f.fire(family));const auto raw=damage(family);const auto converted=rr<0?0u:std::min(raw,effects::reversal_fortune(rr)->maximum_healing);const auto healed=std::min(1000.,hp+double(converted));const auto loss=std::min(healed,double(raw-converted));const bool triggered=loss>0;const auto heal=effects::healing_seed(rank)->healing;double wanted=healed-loss;if(wanted>0&&triggered)wanted=std::min(1000.,wanted+heal);if(family==341&&wanted>0){combat::deep_wound::State wound;wound.reset(1000);wound.capacity(wanted,1000,true);}
  if(std::abs(double(f.hp())-wanted)>=1.01)throw std::runtime_error("family="+std::to_string(family)+" rr="+std::to_string(rr)+" hp="+std::to_string(hp)+" actual="+std::to_string(f.hp())+" expected="+std::to_string(wanted));
  check(f.w.resources(3)->health==500+(triggered?heal:0));const auto packets=f.box->drain();unsigned losses=0,gains=0,first_loss=unsigned(packets.size()),first_gain=first_loss;
  for(unsigned i=0;i<packets.size();++i){const auto& p=packets[i];if(p.size()!=18||p[0]!=162)continue;const auto value=std::bit_cast<float>(word(p,14));if(word(p,6)==2&&value<0){++losses;first_loss=std::min(first_loss,i);}if(word(p,6)==3&&value>0){++gains;first_gain=std::min(first_gain,i);}}
  check(gains==unsigned(triggered));if(triggered)check(losses>=1&&first_loss<first_gain);if(!wanted)check(f.w.player_death_times({2,3})[0]==200);
 }
 // Natural expiry may re-enter Health settlement through another party member.
 for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u}){
  ++cases;Fixture f(500,rank);f.rr(rr);f.setup(19);check(f.fire(19));const auto before=f.hp(),ally=f.w.resources(3)->health;f.w.tick(1200);const auto heal=effects::healing_seed(rank)->healing;check(f.hp()==before-17+heal&&f.w.resources(3)->health==ally+heal);f.w.tick(1300);check(f.hp()==before-17+heal);
 }
 for(unsigned family:families)for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(10);check(f.w.apply_condition(f.foe,f.target,combat::conditions::Kind::deep_wound,100,10000));f.rr(rr);f.setup(family);check(f.fire(family));check(f.hp()==0&&f.w.resources(3)->health==500);check(f.w.player_death_times({2,3})[0]==200);
 }
 for(unsigned family:families)for(int rr:{-1,0,12,20}){
  ++cases;Fixture f(500,12,true);f.rr(rr);f.setup(family);check(f.fire(family));check(f.w.resources(3)->health==500);
 }
 // A failed recipient preflight must not spend Hex Breaker or conversion.
 for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(500,12,false,true);f.rr(rr);f.setup(10);const bool loss=damage(10)>effects::reversal_fortune(rr)->maximum_healing;check(f.fire(10)==!loss);const auto ward=*f.w.status_snapshot(f.foe,200);check(std::any_of(ward.begin(),ward.end(),[](const auto& e){return e.skill==10;})==loss);const auto status=*f.w.status_snapshot(f.target,200);check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;})==loss);check(f.hp()==(loss?500u:500u+damage(10))&&f.w.resources(3)->health==500);
 }
 // Degeneration is Health loss, not a direct damage event or Seed trigger.
 for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u}){
  ++cases;Fixture f(500,rank);f.rr(rr);check(f.w.apply_condition(f.foe,f.target,combat::conditions::Kind::poison,200,3000));f.w.tick(1200);check(f.hp()==492&&f.w.resources(3)->health==500);
 }
 // Fully reduced Fragility damage triggers neither conversion nor Seed.
 for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u}){
  ++cases;Fixture f(500,rank);f.rr(rr);check(f.w.protective_enchantment(f.target,f.target,*skills::regeneration::reviewed(299,20),0,100));f.setup(19);check(f.fire(19));check(f.hp()==500&&f.w.resources(3)->health==500);
 }
 cases+=reactive_seed_deadline_cases();check(cases==564);std::cout<<"PASS "<<cases<<" reactive Healing Seed family/conversion/survival/expiry/range/failure cases;NPC damaged recipients and rendered timing remain open\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

// NEW unregistered.32 paired actualWorld cases, not a throughput benchmark.
#include "ordinary_dagger_stress_peer_fixture.h"
#include <tuple>
#include <string>
struct Observation {
 unsigned source_health,source_energy,target_health,target_energy,selections;
 // Instance IDs differ between independent Worlds; assert exact local Scopes
 // below, then compare all owned terminal fields with those Scopes normalized.
 using Event=std::tuple<unsigned,std::uint64_t,unsigned,std::uint64_t,std::uint64_t,std::uint64_t,std::uint64_t,
  combat::weapon::AttackOutcome,std::optional<std::uint64_t>,combat::weapon::AttackDelivery,bool>;
 std::vector<Event> events;
 bool operator==(const Observation&)const=default;
};
static Observation run(unsigned mask,unsigned mode,bool dense){
 Fixture f(false,500,true,true);f.vigor();
 if(mask&1)check(f.w.status_mutation(f.source,346,123,0,100,8100,true));
 if(mask&2)check(f.w.faintheartedness(f.hexer,f.source,*shadow_of_fear(12),100));
 // Independent declared integer rounding oracle; does not query private phase.
 const std::uint64_t multiplier=(mask&1?67u:100u)*(mask&2?150u:100u);
 const auto gap=(500*multiplier+5000)/10000,first_at=1000+gap,second_at=first_at+gap;
 f.start();const auto first=f.first(first_at);check(first.size()==1&&first[0].at==first_at&&first[0].outcome==combat::weapon::AttackOutcome::hit);
 check(f.health()==980&&f.w.resources(1)->health==500+vigorous_spirit(12)->healing&&f.w.resources(1)->energy==100);
 std::vector<combat::weapon::ResolvedAttack> collected=first;
 if(dense){f.w.tick(first_at+gap/4);check(f.outcomes().empty());f.w.tick(first_at+gap/2-1);check(f.outcomes().empty());}
 const auto changed_at=first_at+gap/2;
 if(mode==1)f.w.cancel_ordinary_attack(f.source);
 if(mode==2||mode==6){auto changed=f.weapon;if(mode==2)++changed.revision;else changed.damage.maximum=21;check(f.w.ordinary_weapon(f.source,changed,20,changed_at,combat::weapon::CriticalProfile{1,0,20}));}
 if(mode==3){const auto old=f.target;f.w.leave(old.actor);f.w.join(old.actor,"New stress target",80,0,std::make_shared<world::Outbox>(),0,{}, {},changed_at,2);f.w.set_vitals(old.actor,{1000,1000,100,100},100);check(f.w.effect_scope(old.actor)!=std::optional(old));}
 if(mode==4){const auto old=f.source;f.w.leave(old.actor);f.w.join(old.actor,"New stress owner",0,0,f.box,0,{}, {},changed_at,1);f.w.set_vitals(old.actor,{500,1000,100,100},100);check(f.w.effect_scope(old.actor)!=std::optional(old));}
 if(mode==5)check(f.w.gale(f.hexer,f.source,12,.5,changed_at));
 if(mode==7)check(f.w.ordinary_attack(f.source,f.target.actor,changed_at));
 if(dense){f.w.tick(first_at+3*gap/4);check(f.outcomes().empty());f.w.tick(second_at-1);check(f.outcomes().empty());f.w.tick(second_at);const auto out=f.outcomes();collected.insert(collected.end(),out.begin(),out.end());}
 // Coarse path jumps from first/midpoint command to due+1. The canonical
 // effect frontier must publish actual second at due, never at coarse now.
 f.w.tick(second_at+1);const auto out=f.outcomes();collected.insert(collected.end(),out.begin(),out.end());
 const unsigned contacts=mode==0?2:1;check(collected.size()==contacts);
 Observation result{f.w.resources(1)->health,f.w.resources(1)->energy,f.w.resources(2)->health,f.w.resources(2)->energy,f.selections,{}};
 check(result.source_energy==100&&result.target_energy==100&&result.selections==1);
 check(result.target_health==(mode==3?1000u:mode==0?960u:980u));
 check(result.source_health==(mode==4?500u:500u+contacts*vigorous_spirit(12)->healing));
 for(unsigned i=0;i<collected.size();++i){const auto& e=collected[i];check(e.attacker==f.source&&e.target==f.target&&e.outcome==combat::weapon::AttackOutcome::hit&&e.at==(i==0?first_at:second_at)&&e.released==e.at&&e.before_damage==std::optional(e.at)&&!e.skill_failed);
  result.events.emplace_back(e.attacker.actor,e.attacker.generation,e.target.actor,e.target.generation,e.event,e.released,e.at,e.outcome,e.before_damage,e.delivery,e.skill_failed);
 }
 f.w.tick(second_at+1);check(f.outcomes().empty()&&f.w.resources(1)->health==result.source_health&&f.w.resources(2)->health==result.target_health);
 if(const auto scope=f.w.effect_scope(1))f.w.cancel_ordinary_attack(*scope);
 return result;
}
int main()try{
 for(unsigned mask=0;mask<4;++mask)for(unsigned mode=0;mode<8;++mode){
  const auto dense=run(mask,mode,true),coarse=run(mask,mode,false);
  if(!(dense==coarse))throw std::runtime_error("dense/coarse mismatch mask="+std::to_string(mask)+" mode="+std::to_string(mode));
 }
 std::cout<<"PASS32 paired actualWorld delayed contacts dense/coarse, IAS/slow, due+-1, cancel/gear/control/Scopes/Intent; first anchored\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

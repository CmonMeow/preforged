#include "gw_deathly_swarm.h"
#include "gw_skill_casting.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace skills;using namespace skills::effects;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto plan=deathly_swarm(12);check(plan&&plan->damage==78&&plan->radius==322,"reviewed plan");
    check(deathly_swarm(0)->damage==30&&deathly_swarm(15)->damage==90&&!deathly_swarm(21),"rank bounds");
    Scope caster{1,42,1},primary{1,43,1},second{1,44,1},third{1,45,1};
    std::vector<SwarmCandidate> foes{{primary,100,0,true,true},{second,422,0,true,true},{third,744,0,true,true}};
    DeathlySwarm chain;check(chain.launch(caster,primary,*plan,2000),"launch");
    check(!chain.advance(caster,2999,foes),"early first hit");
    auto hit=chain.advance(caster,3000,foes);check(hit&&hit->target==primary&&hit->ordinal==1,"first target");
    // A killing strike does not erase the previous victim as the chain center.
    foes[0].alive=false;
    check(!chain.advance(caster,3999,foes),"early second hit");
    hit=chain.advance(caster,4000,foes);check(hit&&hit->target==second&&hit->ordinal==2,"inclusive area edge");
    hit=chain.advance(caster,5000,foes);check(hit&&hit->target==third&&hit->ordinal==3&&!chain.pending(),"third chains from second");
    check(!chain.advance(caster,6000,foes),"fourth hit");
    foes[0].alive=true;foes[1].x=423;
    check(chain.launch(caster,primary,*plan,0)&&chain.advance(caster,1000,foes).has_value(),"range setup");
    check(!chain.advance(caster,2000,foes)&&!chain.pending(),"out of area chain continued");
    check(chain.launch(caster,primary,*plan,0),"lifetime setup");++foes[0].scope.generation;
    check(!chain.advance(caster,1000,foes),"recycled primary hit");--foes[0].scope.generation;
    check(chain.launch(caster,primary,*plan,0),"caster setup");auto replaced=caster;++replaced.generation;
    check(!chain.advance(replaced,1000,foes)&&!chain.pending(),"recycled caster survived");
    check(!chain.launch(caster,primary,*plan,UINT64_MAX-2999),"deadline overflow");
    auto tampered=*plan;tampered.damage=1000;check(!chain.launch(caster,primary,tampered,0),"forged damage");
    combat::Actor victim{43,1,combat::Health{500,500},60,0,true};victim.cold_armor=60;
    combat::conditions::State conditions;
    auto damage=deathly_swarm_target(*plan,victim,conditions,20,0);
    check(damage&&damage->damage==78&&damage->kind==combat::Kind::cold&&damage->after.current==422,"cold armor damage");
    victim.cold_armor=100;damage=deathly_swarm_target(*plan,victim,conditions,20,0);
    check(damage&&damage->damage==39,"cold armor mitigation");
    victim.cold_armor.reset();check(!deathly_swarm_target(*plan,victim,conditions,20,0),"invented cold armor");
    const auto particle=particles::target_body_2022(43,105);
    check(particle&&particle->size()==14&&(*particle)[10]==201,"pinned target particle");

    using namespace casting;Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.learned={{105,1}};
    Slots bar{};bar[0]={105,0};attributes::Ranks ranks{};ranks[5]=12;
    vitals::Resources self{100,100,30,30};Hooks h;unsigned launches=0;std::vector<Packet> events;
    h.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());};
    h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):std::nullopt;};
    h.consume_energy=[&](unsigned cost){if(cost>self.energy)return false;self.energy-=cost;return true;};
    h.target_valid=[](auto,float,bool){return false;};
    h.projectile_target_valid=[](unsigned id,float range){return id==43&&range==1248;};
    h.target_scope=[&](unsigned id)->std::optional<Scope>{return id==43?std::optional(primary):std::nullopt;};
    Runtime cast(42);
    check(cast.start(0,{105,0,43},bar,policy,ranks,true,h)==casting::Reason::unsupported&&self.energy==30,"missing world hook spent energy");
    h.deathly_swarm=[&](Scope scope,const DeathlySwarmPlan& p,std::uint64_t now){check(scope==primary&&p==*plan&&now==2000,"cast launch identity/time");++launches;return true;};
    check(cast.start(0,{105,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==20,"cast energy");
    cast.update(1999,h);check(!launches,"early activation");cast.update(2000,h);check(launches==1,"missing launch without synthetic NPC resources");
    check(cast.start(2750,{105,0,43},bar,policy,ranks,true,h)==casting::Reason::recharge,"six second recharge");
    check(!events.empty(),"missing native activation events");
    Runtime stale(42);check(stale.start(0,{105,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"stale setup");
    ++primary.generation;stale.update(2000,h);check(launches==1,"recycled cast target");
    std::cout<<"Deathly Swarm timings, chain area, lifetimes, and cast lifecycle verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


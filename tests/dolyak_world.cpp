#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Dolyak line "+std::to_string(at.line()));}
struct Fixture{world::Instance w;Scope source,target;
 Fixture(){for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);check(w.combat_armor(*w.effect_scope(id),60)&&w.elemental_armor(*w.effect_scope(id),60,60));}source=*w.effect_scope(1);target=*w.effect_scope(2);}
 bool apply(unsigned rank=15,std::uint64_t now=1000,unsigned effect=100){return w.status_mutation(target,361,effect,rank,now,now+dolyak_signet(rank)->duration,true);}
};
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=dolyak_signet(rank);check(p&&p->armor==10+2*rank&&p->duration==1000ull*interpolate(8,20,rank)&&p->movement==.25f);Registry r({1,2,3});check(r.apply({1,2,3},{1,2,361,rank,Category::other,100,100+p->duration},Replace::same_skill).accepted);check(active_dolyak(r,{1,2,3},100).has_value()&&!active_dolyak(r,{1,2,3},100+p->duration));}
 check(!dolyak_signet(21)&&stacked_skill_armor(24,10)==25&&stacked_skill_armor(60,40)==60);
 for(unsigned rank:{0u,12u,20u}){Fixture f;check(f.apply(rank));const auto epoch=f.w.control_epoch(f.target);check(f.w.gale(f.source,f.target,12,{},1100)&&f.w.control_epoch(f.target)==epoch);const auto end=1000+dolyak_signet(rank)->duration;check(f.w.gale(f.source,f.target,12,{},end)&&f.w.control_epoch(f.target)!=epoch);}
 for(bool earth:{false,true}){Fixture f;check(f.apply(0));if(earth)check(f.w.armor_earth(f.target,0,1000));const auto plan=*elemental(191,0);check(f.w.elemental(f.source,f.target,plan,20,1100));const unsigned expected=unsigned(std::ceil(1000-20*std::exp2((60.-(earth?85:70))/40.)));check(f.w.resources(2)->health==expected);}
 {Fixture f;check(f.apply());f.w.move(2,10000,0,1000);f.w.tick(2000);check(std::abs(f.w.position(2,2000).first-132)<.3);check(f.w.status_mutation(f.target,361,100,15,2000,{},false));f.w.tick(3000);check(std::abs(f.w.position(2,3000).first-420)<.3);}
 {Fixture f;check(!f.w.status_mutation(f.target,361,100,15,1000,9999,true));check(f.apply());check(f.w.kill(f.target,1100));check(f.w.status_snapshot(f.target,1100)->empty());}
 {Fixture f;check(f.w.dolyak_available(f.target,1000));check(f.w.cast_activity(f.target,281,1000,2000));check(!f.w.dolyak_available(f.target,1000)&&!f.apply());check(f.w.dolyak_available(f.target,2000));check(f.w.cast_activity(f.target,281,0,0));check(f.w.attack_activity(f.target,1500,1000));check(!f.w.dolyak_available(f.target,1000)&&!f.apply());check(f.w.dolyak_available(f.target,1500));}
 for(bool absent:{false,true}){Fixture f;casting::Hooks h;h.resources=[&](unsigned id){return f.w.resources(id);};h.consume_energy=[](unsigned cost){return cost==0;};h.events=[](const auto&){};h.target_valid=[](unsigned id,float,bool self){return id==2&&self;};h.effect_snapshot=[&](auto now){return *f.w.status_snapshot(f.target,now);};if(!absent)h.status_mutation=[&](unsigned skill,unsigned effect,unsigned rank,auto now,auto end,bool active){return f.w.status_mutation(f.target,skill,effect,rank,now,end,active);};Policy p;p.primary=1;p.campaigns=255;p.learned[361]=1;Slots bar{};bar[0]={361,0};attributes::Ranks rank{};rank[17]=15;casting::Runtime runtime(2);check(runtime.start(1000,{361,0,1},bar,p,rank,true,h)==(absent?casting::Reason::unsupported:casting::Reason::accepted));if(!absent){runtime.update(1000,h);const auto epoch=f.w.control_epoch(f.target);check(f.w.gale(f.source,f.target,12,{},1100)&&f.w.control_epoch(f.target)==epoch);runtime.update(21000,h);check(f.w.status_snapshot(f.target,21000)->empty());}}
 std::cout<<"PASS Dolyak owned armor stacking,movement75percent penalty,knockdown immunity,expiry/removal/death and zero-cost Runtime self-target\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>

int main()try{
    using namespace fw::gw;using namespace world;using namespace skills;using namespace casting;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto setup=[](Instance& w){
        w.join(1,"Elementalist",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        w.join(2,"Monk",180,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{480,480,40,40},100);w.set_vitals(2,{480,480,40,40},100);
        w.combat_armor(*w.effect_scope(2),60);
    };
    const auto hooks_for=[](Instance& w,effects::Scope scope,unsigned& commits){
        Hooks h;h.events=[](const auto&){};h.resources=[&w](unsigned id){return w.resources(id);};
        h.control_epoch=[&w,scope]{return w.control_epoch(scope);};
        h.control_event_time=[&w,scope](auto epoch){return w.control_event_time(scope,epoch);};
        h.target_valid=[scope](unsigned target,float,bool){return target==scope.actor;};
        h.consume_energy=[&w,scope](unsigned n){return w.effect_scope(scope.actor)==scope&&w.consume_energy(scope.actor,n);};
        h.heal=[&commits](auto,auto,float,bool){++commits;return true;};return h;
    };
    Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[281]=1;
    Slots bar{};bar[0]={281,0};attributes::Ranks ranks{};
    {
        Instance w;setup(w);const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        const auto hawk=*effects::earth_projectile(1374,12,12),stone=*effects::earth_projectile(171,12,12);
        check(w.launch_projectile(source,2,hawk,20,100),"Weakness projectile");w.tick(200);
        check(w.has_condition(target,combat::conditions::Kind::weakness,200),"Weakness impact");
        unsigned commits=0;auto h=hooks_for(w,target,commits);Runtime cast(2);
        check(cast.start(250,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted,"spell windup before knockdown");
        check(w.resources(2)->energy==35,"spell did not pay activation cost");
        check(w.launch_projectile(source,2,stone,20,250),"Stoning projectile");w.tick(350);
        check(w.knocked_down(target,350)&&w.control_epoch(target)==1,"knockdown advances control epoch");
        check(w.launch_projectile(source,2,stone,20,1000),"repeat Stoning");w.tick(1100);
        check(w.control_epoch(target)==1,"rejected repeated knockdown advanced epoch");
        w.tick(2400);check(!w.knocked_down(target,2400),"knockdown recovery");
        const auto energy_before_cancel=w.resources(2)->energy;
        cast.update(2400,h);cast.update(2400,h);
        check(!cast.active()&&commits==0,"delayed spell survived knockdown and recovery");
        check(w.resources(2)->energy==energy_before_cancel,"interruption refunded paid cost");
        const auto ready=std::max<std::uint64_t>(2400,350+std::uint64_t(find_skill(281)->recharge)*1000);
        check(w.control_event_time(target,0)==350,"control event lost its original timestamp");
        cast.update(ready,h);
        check(cast.start(ready,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted,"new post-recharge activation");
        cast.update(ready+1000,h);check(commits==1,"post-recharge spell failed");
    }
    {
        Instance w;setup(w);const auto scope=*w.effect_scope(2);w.set_vitals(2,{1,480,40,40},100);
        unsigned commits=0;auto h=hooks_for(w,scope,commits);Runtime cast(2);
        check(cast.start(100,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted,"death gap spell");
        const auto loss=w.health_sacrifice(scope,115,200);check(loss&&loss->killed,"death between casting polls");
        check(w.resurrect(scope,300),"authorized resurrection between casting polls");cast.update(1200,h);
        check(!cast.active()&&commits==0,"resurrection revived an old pending spell");
    }
    {
        check(!Runtime::knockdown_restricted(1043)&&!Runtime::knockdown_restricted(346)&&
              Runtime::knockdown_restricted(281)&&Runtime::knockdown_restricted(335),"instant stances vs interruptible activations");
        std::uint64_t epoch=0;unsigned applies=0;Hooks h;h.events=[](const auto&){};
        h.control_epoch=[&]()->std::optional<std::uint64_t>{return epoch;};
        h.resources=[](auto)->std::optional<vitals::Resources>{return vitals::Resources{480,480,40,40};};
        h.consume_energy=[](auto){return true;};h.target_valid=[](auto,float,bool){return true;};
        h.speed_multiplier=[&](double speed){if(speed==1.5)++applies;return true;};
        Policy instant;instant.primary=7;instant.campaigns=0x1e;instant.learned[1043]=1;Slots stance{};stance[0]={1043,0};
        Runtime cast(2);check(cast.start(100,{1043,0,2},stance,instant,ranks,true,h)==casting::Reason::accepted,"instant stance begin");
        ++epoch;cast.interrupt_control(100,h);cast.update(100,h);
        check(applies==1,"control incorrectly cancelled an instant no-aftercast stance");
    }
    {
        Instance w;setup(w);const auto scope=*w.effect_scope(2);unsigned commits=0;auto h=hooks_for(w,scope,commits);
        h.control_epoch=[]()->std::optional<std::uint64_t>{return {};};Runtime cast(2);
        check(cast.start(100,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::policy&&w.resources(2)->energy==40,"missing lifetime epoch spent energy");
    }
    // NPC targets have authoritative scoped World state, not player Resources.
    // All reviewed interrupt skills must reach their scoped completion hook.
    for(unsigned skill:{5u,23u,25u,61u,24u,931u,860u})for(unsigned mode=0;mode<3;++mode){
        Runtime cast(1);Hooks h;unsigned paid=0,commits=0;bool completed=false;
        const effects::Scope owner{1,1,1},npc{1,100,1};
        h.events=[](const auto&){};
        h.resources=[](unsigned id)->std::optional<vitals::Resources>{return id==1?std::optional(vitals::Resources{480,480,100,100}):std::nullopt;};
        h.consume_energy=[&](unsigned n){paid+=n;return true;};
        h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{if(id==1)return owner;if(id==100){auto result=npc;if(completed&&mode==1)++result.generation;return result;}return {};};
        h.projectile_target_valid=[&](unsigned id,float range){return id==100&&range==1248&&!(completed&&mode==2);};
        h.interrupt_skill=[&](effects::Scope target,const effects::InterruptPlan& plan,auto){check(target==npc&&plan.skill==skill,"NPC interrupt scoped hook changed target");++commits;return true;};
        Policy p;p.primary=5;p.campaigns=255;p.learned[skill]=1;Slots interrupt_bar{};interrupt_bar[0]={skill,0};
        attributes::Ranks r{};const auto* row=find_skill(skill);if(row->attribute<51)r[row->attribute]=12;
        check(cast.start(1000,{skill,0,100},interrupt_bar,p,r,true,h)==casting::Reason::accepted,"NPC interrupt Runtime admission failed");
        completed=true;cast.update(10000,h);
        check(paid==row->energy_cost()&&commits==unsigned(mode==0),"NPC interrupt skipped valid target or accepted stale/range failure");
    }
    std::cout<<"Control interruption survives delayed polling, recovery and death/resurrection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


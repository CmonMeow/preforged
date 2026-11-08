#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace world;using namespace skills;using namespace effects;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    auto population=[](){
        const std::string hash(64,'a');
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto result=load_npc_catalog(appearance);
        std::istringstream metadata("gw-npc-combat 1 "+hash+"\nsource test authored "+hash+" tests/knockdown_skills.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test\n");
        const auto catalog=combat::npcs::load_catalog(metadata,hash);bind_combat_catalog(result,catalog,hash);
        auto npcs=result.for_map(146);auto& d=*npcs.front().combat_definition;d.physical_armor=60;d.holy_traits={false,{}};d.knockdown_allowed=true;d.knockdown_evidence="test";return npcs;
    };
    auto join=[](Instance& w,std::shared_ptr<Outbox> box){w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{200,200,40,40},100);return *w.effect_scope(1);};
    using combat::npcs::control::Action;using combat::npcs::control::Knockdown;
    for(unsigned variant=0;variant<4;++variant){
        auto npcs=population();if(variant==1)npcs.front().combat_definition->knockdown_allowed=false;
        if(variant==2)npcs.front().combat_definition->knockdown_allowed.reset();
        Instance w;w.configure_npcs(npcs);auto box=std::make_shared<Outbox>();const auto caster=join(w,box),target=*w.effect_scope(100);
        const auto token=variant==3?std::optional<combat::npcs::control::Token>{}:w.begin_npc_action(target,Action::attack,100,2000);
        if(variant!=3)check(token.has_value(),"Bane action fixture");box->drain();
        const bool applied=w.holy_damage(caster,target,*holy_damage(296,12),20,1100);
        check(applied==(variant!=2),"Bane unknown susceptibility preflight");
        check(w.npc_combat_snapshot(target)->health->current==(variant==2?500:450),"Bane ignores summoned trait and uses reviewed holy scale");
        check(w.knocked_down(target,1100)==(variant==0),"Bane pre-control attacking snapshot");
        if(variant==0){
            const auto packets=box->drain();const auto stop=std::find(packets.begin(),packets.end(),combat::weapon::stop_2022(100));
            const auto kd=std::find(packets.begin(),packets.end(),combat::knockdown_2022(100,2));
            const auto damage=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>2&&p[0]==162&&p[1]==0;});
            check(stop!=packets.end()&&kd!=packets.end()&&damage!=packets.end()&&stop<kd&&kd<damage,"Bane captured stop/KD/damage ordering");
            check(!w.complete_npc_action(*token,5000),"Bane interrupted action returned after recovery");
        }else if(token)check(w.complete_npc_action(*token,2100),"immune or rejected Bane canceled action");
    }
    {
        auto npcs=population();auto guardian=npcs.front();guardian.x=0;guardian.combat_definition->relation=combat::npcs::Relation::neutral;
        Instance w;w.configure_npcs({guardian});auto box=std::make_shared<Outbox>();const auto caster=join(w,box),guard=*w.effect_scope(100);npcs.front().x=500;
        check(w.begin_wave_objective(caster,guard,73,1,3,1)&&w.spawn_quest_wave(caster,73,1,npcs,[]{return true;}),"route fixture");const auto enemy=*w.effect_scope(60000);
        check(w.chase_quest_guardian(enemy,100,100)&&w.movement_hex(caster,enemy,*movement_hex(76,0),100),"slowed route");
        check(w.knockdown_npc(enemy,2000,1100)==Knockdown::accepted,"route pause");
        check(!w.chase_quest_guardian(enemy,100,2100)&&w.knocked_down(enemy,3099),"retarget while paused");box->drain();
        w.tick(4100);const auto packets=box->drain();bool found=false;
        for(const auto& p:packets)if(p.size()==16&&p[0]==44&&p[1]==0){std::uint32_t id=0;float x=0;std::memcpy(&id,p.data()+2,4);std::memcpy(&x,p.data()+6,4);if(id==60000){check(x==400,"delayed pause/slow integral");found=true;}}
        check(found&&!w.knocked_down(enemy,4100),"route resume publication");
    }
    {
        Instance w;w.configure_npcs(population());auto box=std::make_shared<Outbox>();const auto caster=join(w,box),target=*w.effect_scope(100);
        combat::weapon::Owned hammer{10,1,{20,20,2},12,false,false,15};check(w.ordinary_weapon(caster,hammer,20),"owned hammer");
        const auto plan=*combat::weapon::attack_skill(331,12);check(plan.required_type==15&&plan.knockdown_on_hit&&plan.bonus==0,"Hammer Bash plan");
        check(w.melee_target(caster,100,100,331)&&w.melee_attack(caster,target,hammer,plan,100),"Hammer Bash hit");
        check(w.knocked_down(target,100)&&w.npc_combat_snapshot(target)->health->current==480,"Hammer Bash damage and knockdown");
        auto bad=plan;bad.knockdown_on_hit=false;check(!w.melee_attack(caster,target,hammer,bad,200),"forged attack plan");
        combat::weapon::Strike strike;strike.blind=true;strike.miss_roll=.5;check(combat::weapon::resolve_attack_skill(strike,plan,false)->missed,"blind Hammer Bash");
    }
    for(unsigned variant=0;variant<3;++variant){
        // Exercise the same Runtime -> world cost/effect hooks as production,
        // including other pools and a miss after paying the full cost.
        using namespace casting;auto npcs=population();if(variant==2)npcs.front().combat_definition->knockdown_allowed.reset();
        Instance w;w.configure_npcs(npcs);auto box=std::make_shared<Outbox>();const auto caster=join(w,box),target=*w.effect_scope(100);
        combat::weapon::Owned hammer{10,1,{20,20,2},12,false,false,15};check(w.ordinary_weapon(caster,hammer,20),"runtime owned hammer");
        Slots bar{};bar[0]={331,0};bar[1]={335,0};bar[2]={336,0};check(w.configure_adrenaline(caster,bar,100),"runtime adrenaline bar");
        const auto power=*combat::weapon::attack_skill(322,12);
        for(unsigned n=0;n<6;++n)check(w.melee_attack(caster,target,hammer,power,100+n),"charge through real weapon hits");
        const auto charged=*w.adrenaline(caster);check(charged[0].units==130&&charged[1].units==80&&charged[2].units==150,"all pools charged");
        if(variant==1)check(w.apply_condition(caster,caster,combat::conditions::Kind::blind,150,5000),"blind fixture");
        Policy policy;policy.primary=1;policy.campaigns=0x1e;policy.learned={{331,1},{335,1},{336,1}};attributes::Ranks ranks{};ranks[20]=12;
        Hooks h;std::uint64_t now=200;h.events=[&](const auto& ps){box->enqueue(ps);};
        h.resources=[&](unsigned id){return w.resources(id);};h.consume_energy=[&](unsigned amount){return w.consume_energy(1,amount);};
        h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(caster,at);};
        h.weapon_snapshot=[&]{return w.attack_weapon(caster);};h.target_scope=[&](unsigned id){return w.effect_scope(id);};
        h.melee_target=[&](unsigned id,auto at){return w.melee_target(caster,id,at,331);};
        h.melee_attack=[&](auto enemy,const auto& weapon,const auto& plan,auto at){return w.melee_attack(caster,enemy,weapon,plan,at);};
        h.attack_activity=[&](auto until){w.attack_activity(caster,until,now);};
        h.consume_adrenaline=[&](auto slot,auto at){return w.consume_adrenaline(caster,slot,at);};
        h.reconcile_adrenaline=[&](auto at){w.reconcile_adrenaline(caster,at);};
        h.control_epoch=[&]{return w.control_epoch(caster);};
        Runtime runtime(1);const auto result=runtime.start(now,{331,0,100},bar,policy,ranks,true,h);
        if(variant==2){const auto after=*w.adrenaline(caster);check(result==casting::Reason::target&&after.size()==charged.size(),"unknown target preflight");for(std::size_t i=0;i<after.size();++i)check(after[i].units==charged[i].units,"unknown target spent adrenaline");continue;}
        check(result==casting::Reason::accepted,"Hammer Bash native runtime start");
        for(const auto& pool:*w.adrenaline(caster))check(pool.units==0,"Hammer Bash left another pool charged");
        const auto before=w.npc_combat_snapshot(target)->health->current;now=700;runtime.update(now,h);
        check(w.knocked_down(target,now)==(variant==0),"miss applied knockdown or hit failed knockdown");
        check(w.npc_combat_snapshot(target)->health->current==before-(variant==0?20:0),"runtime hit/miss damage");
        for(const auto& pool:*w.adrenaline(caster))check(pool.units==(variant==0?25:0),"hit gain/miss cost disagrees with world");
    }
    {
        using namespace casting;Hooks h;unsigned hits=0,cost=0;Scope target{1,100,1};h.events=[](const auto&){};
        h.resources=[](unsigned id)->std::optional<vitals::Resources>{return id==1?std::optional<vitals::Resources>{{200,200,40,40}}:std::nullopt;};h.consume_energy=[&](unsigned c){cost+=c;return true;};
        h.target_valid=[](auto,float,bool){return false;};h.target_scope=[&](unsigned id)->std::optional<Scope>{return id==100?std::optional(target):std::nullopt;};h.projectile_target_valid=[](auto id,float){return id==100;};
        h.holy_damage=[&](auto t,const auto& p,auto now){check(t==target&&p.skill==296&&p.damage==50&&now==1100,"Bane cast parameters");++hits;return true;};
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[296]=1;Slots bar{};bar[0]={296,0};attributes::Ranks ranks{};ranks[14]=12;
        Runtime runtime(1);check(runtime.start(100,{296,0,100},bar,policy,ranks,true,h)==casting::Reason::accepted&&cost==0,"Bane no energy cost");runtime.update(1099,h);check(!hits,"early Bane");runtime.update(1100,h);check(hits==1,"Bane completed");
    }
    std::cout<<"Knockdown skills tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


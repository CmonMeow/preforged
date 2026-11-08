#include "gw_putrid_bile.h"
#include <iostream>
#include <stdexcept>

using namespace fw::gw::skills::effects;
static void check(bool yes){if(!yes)throw std::runtime_error("Putrid Bile plan/death batch");}
int main()try{
    check(putrid_bile(0)->duration_ms==5000&&putrid_bile(0)->death_damage==25);
    check(putrid_bile(15)->duration_ms==17000&&putrid_bile(15)->death_damage==73);
    check(putrid_bile(20)->duration_ms==20000&&putrid_bile(20)->death_damage==85);
    check(putrid_bile(15)->degeneration_pips==3&&!putrid_bile(21));
    auto plan=*putrid_bile(15);
    fw::gw::combat::Actor a,b;
    a.id=10;a.generation=1;a.health=fw::gw::combat::Health{200,200};a.damageable=true;
    b=a;b.id=11;b.health=fw::gw::combat::Health{50,200};
    const auto hit=putrid_bile_death(plan,{a,b},20);
    check(hit&&hit->hits.size()==2&&hit->hits[0].applied==73&&hit->hits[1].killed);
    check(hit->targets[0].health->current==127&&hit->targets[1].health->current==0);
    check(a.health->current==200&&b.health->current==50);
    auto bad=b;bad.health.reset();
    check(!putrid_bile_death(plan,{a,bad},20));
    auto forged=plan;forged.death_damage=999;
    check(!putrid_bile_death(forged,{a},20));
    check(!putrid_bile_death(plan,{a},0));
    const Scope caster{7,3,1},victim{7,4,2};
    const PutridBileLink link{caster,victim,9,15,20,1000,18000};
    const std::vector<PutridBileRecipient> nearby={
        {{7,10,1},a,1,0,0,true},
        {{7,11,1},b,1,200,0,true},
        {{7,12,1},a,1,20,0,false},
        {{7,13,1},a,2,20,0,true},
        {{7,14,1},a,1,400,0,true}};
    auto selected=nearby;
    selected[2].actor.id=12;selected[3].actor.id=13;selected[4].actor.id=14;
    const auto explosion=putrid_bile_explosion(link,victim,1,0,0,1500,selected);
    check(explosion&&explosion->recipient_ids==std::vector<std::uint32_t>({10,11})&&
          explosion->hits.size()==2&&explosion->hits[0].applied==73);
    check(!putrid_bile_explosion(link,{7,4,3},1,0,0,1500,selected));
    check(!putrid_bile_explosion(link,victim,1,0,0,18000,selected));
    selected[1].actor.health.reset();
    check(!putrid_bile_explosion(link,victim,1,0,0,1500,selected));
    PutridBileLinks links;
    check(links.assign(link)&&links.size()==1);
    check(links.lookup(victim)==link);
    check(!links.remove(victim,8));
    check(links.take_death(victim,1500)==link);
    check(!links.take_death(victim,1500));
    check(links.assign(link)&&links.remove(victim,9));
    check(!links.take_death(victim,1500));
    check(links.assign(link)&&!links.take_death(victim,18000));
    check(!links.assign(PutridBileLink{caster,victim,0,15,20,1000,18000}));
    std::cout<<"PASS Putrid Bile catalog plan and atomic copy-only death damage; World binding pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
static void check(bool good,std::source_location at=std::source_location::current()){
 if(!good)throw std::runtime_error("Wary cast line "+std::to_string(at.line()));
}
int main()try{
 warrior_test::Fixture fixture(340,false);auto& world=fixture.world;
 Slots bar{};bar[0]={377,0};bar[1]={340,0};check(world.configure_adrenaline(fixture.source,bar,100));
 Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[377]=policy.learned[340]=1;
 attributes::Ranks ranks{};ranks[21]=12;
 casting::Runtime runtime(1);casting::Hooks hooks;std::vector<Packet> packets;unsigned paid=0,starts=0;
 hooks.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
 hooks.resources=[&](auto actor){return world.resources(actor);};
 hooks.target_valid=[](auto actor,float,bool self){return actor==1&&self;};
 hooks.consume_energy_at=[&](auto amount,auto now){paid+=amount;return world.consume_energy(1,amount,fixture.source,now);};
 hooks.effect_snapshot=[&](auto now){return *world.status_snapshot(fixture.source,now);};
 hooks.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return world.status_mutation(fixture.source,skill,effect,rank,now,until,active);};
 hooks.begin_action=[&](auto slot,auto now){++starts;check(world.accepted_skill_start(fixture.source,slot,now));};
 // Production omits this capability until the observed per-block adrenaline
 // reward has an authoritative World consumer.
 check(runtime.start(1000,{377,0,1},bar,policy,ranks,true,hooks)==casting::Reason::unsupported);
 check(paid==0&&starts==0&&world.status_snapshot(fixture.source,1000)->empty());
 hooks.wary_stance_reward_ready=[](){return false;};
 check(runtime.start(1000,{377,0,1},bar,policy,ranks,true,hooks)==casting::Reason::unsupported);
 check(paid==0&&starts==0);
 hooks.wary_stance_reward_ready=[](){return true;};
 check(runtime.start(1000,{377,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
 const auto plan=wary_stance(12);check(plan&&paid==10&&starts==1);
 const auto active=*world.status_snapshot(fixture.source,1000);
 check(active.size()==1&&active[0].skill==377&&active[0].rank==12&&active[0].expires==1000+plan->duration);
 check(std::count(packets.begin(),packets.end(),casting::generic(48,1,377))==1);
 check(std::count(packets.begin(),packets.end(),casting::generic(21,1,636))==1);
 check(runtime.start(1001,{377,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
 check(paid==10&&starts==1);
 runtime.update(1000+plan->duration,hooks);
 check(world.status_snapshot(fixture.source,1000+plan->duration)->empty());
 std::cout<<"PASS Wary gated instant cast,10 Energy,ranked status,body636,10s recharge and exact expiry\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

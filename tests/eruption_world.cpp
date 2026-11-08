#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;using combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Eruption line "+std::to_string(at.line()));}
struct Fixture{
 world::Instance w;Scope caster,target;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool unknown=false){std::vector<world::Instance::Npc> population;for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?352:100;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=id==100;if(!unknown){d.earth_armor=60;}n.combat_definition=d;population.push_back(n);}w.configure_npcs(population);
 for(unsigned id:{1u,2u,3u,4u,5u}){w.join(id,"Player",id==1?0:id==2||id==5?100:id==3?352:353,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);w.set_vitals(id,{1000,1000,100,100},100);check(w.combat_armor(*w.effect_scope(id),60));}caster=*w.effect_scope(1);target=*w.effect_scope(2);box->drain();}
 Scope scope(unsigned id){return *w.effect_scope(id);}
 bool cast(unsigned rank=15,std::uint64_t now=1000){return w.fire_storm(caster,target,*fire_storm(rank,167),20,now);}
 double health(unsigned id){return id<100?w.resources(id)->health:w.npc_combat_snapshot(scope(id))->health->current;}
};
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=damage_field(167,rank);check(p&&p->kind==FieldDamage::earth&&p->damage==10+2*rank&&p->pulses==5&&p->radius==252&&p->blind_ms==10000);check(!damage_field(167,rank,true));}check(!damage_field(167,21));
 for(unsigned rank:{0u,12u,20u}){Fixture f;check(f.cast(rank));f.w.tick(1900);check(f.health(2)==1000);f.w.tick(2000);for(unsigned id:{2u,3u,100u,101u}){check(f.health(id)==1000-(10+2*rank));check(f.w.has_condition(f.scope(id),Kind::blind,2000)==(id!=101));}for(unsigned id:{1u,4u,5u})check(f.health(id)==1000&&!f.w.has_condition(f.scope(id),Kind::blind,2000));f.w.tick(6000);check(f.health(2)==1000-5*(10+2*rank));f.w.tick(15000);check(f.w.has_condition(f.target,Kind::blind,15000));f.w.tick(16000);check(!f.w.has_condition(f.target,Kind::blind,16000));auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),particles::eruption_ground_2022(1,100,0,0))==1);}
 {Fixture f(true);check(f.cast());f.w.tick(6000);check(f.health(2)==800&&f.health(100)==1000&&!f.w.has_condition(f.scope(100),Kind::blind,6000));}
 {Fixture f;check(f.cast());f.w.leave(1);f.w.tick(6000);check(f.health(2)==1000&&!f.w.has_condition(f.target,Kind::blind,6000));}
 for(unsigned mode=0;mode<4;++mode){Fixture f;casting::Hooks h;unsigned calls=0,paid=0;h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[](const auto&){};h.consume_energy=[&](unsigned cost){++paid;check(cost==25);return f.w.consume_energy(1,cost,f.caster);};h.target_scope=[&](unsigned id){return f.w.effect_scope(id);};h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};if(mode!=3)h.complete_fire_storm=[&](auto target,const auto& p,auto now)->std::optional<std::vector<world::Packet>>{++calls;std::vector<world::Packet> out;if(!f.w.fire_storm(f.caster,target,p,20,now,&out))return {};return out;};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[167]=1;Slots bar{};bar[0]={167,0};attributes::Ranks ranks{};ranks[9]=15;casting::Runtime runtime(1);check(runtime.start(1000,{167,0,2},bar,policy,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1500,h);runtime.update(3000,h);check(calls==(mode==0?1:0)&&paid==(mode<3?1:0));if(mode==0){f.w.tick(4000);check(f.health(2)==960&&f.w.has_condition(f.target,Kind::blind,4000));}}
 std::cout<<"PASS Eruption fixed five earth pulses,nearby boundary,Blindness renewal,immune NPCs,unknown armor and Runtime lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

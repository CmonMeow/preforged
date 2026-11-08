#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Maelstrom line "+std::to_string(at.line()));}
struct Fixture{
 world::Instance w;Scope caster,target,npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(){world::Instance::Npc n;n.id=n.definition.id=100;n.x=266;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;d.cold_armor=60;d.cold_armor_evidence="test";n.combat_definition=d;w.configure_npcs({n});
 for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Player",id==1?0:id==2?100:id==3?266:267,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{1000,1000,100,100},100);
 check(w.elemental_armor(*w.effect_scope(id),{},60));}caster=*w.effect_scope(1);target=*w.effect_scope(2);npc=*w.effect_scope(100);box->drain();}
 bool cast(unsigned rank=15,std::uint64_t now=1000){return w.fire_storm(caster,target,*fire_storm(rank,215),20,now);}
};
int main()try{
 for(unsigned rank=0;rank<=20;++rank){auto p=damage_field(215,rank);
 check(p&&p->damage==10+rank&&p->kind==FieldDamage::cold&&p->pulses==10&&p->radius==166);
 check(!damage_field(215,rank,true));}
 check(!damage_field(215,21));
 for(unsigned active:{281u,1u})for(bool protection:{false,true}){Fixture f;
 check(f.cast());
 check(f.w.cast_activity(f.target,active,1500,3500));if(protection)check(f.w.status_mutation(f.target,16,123,15,1500,10000,true));
 const auto epoch=f.w.control_epoch(f.target);
 f.w.tick(1900);
 check(f.w.resources(2)->health==1000);
 f.w.tick(2000);
 check(f.w.resources(2)->health==975&&f.w.resources(3)->health==975&&f.w.resources(4)->health==1000);
 check(f.w.npc_combat_snapshot(f.npc)->health->current==975);
 check((f.w.control_epoch(f.target)!=epoch)==(active==281&&!protection));
 f.w.tick(11000);
 check(f.w.resources(2)->health==750);auto packets=f.box->drain();
 check(std::count(packets.begin(),packets.end(),particles::maelstrom_ground_2022(100,0,0))==10);
 f.w.tick(12000);
 check(f.w.resources(2)->health==750);}
 {Fixture f;
 check(f.cast());
 check(f.w.begin_npc_action(f.npc,combat::npcs::control::Action::spell,1500,3500,Slot{281,0}).has_value());
 f.w.tick(2000);
 check(f.w.begin_npc_action(f.npc,combat::npcs::control::Action::spell,2001,2400,Slot{1,0}).has_value());}
 {Fixture f;
 check(f.cast());f.w.leave(1);
 f.w.tick(11000);
 check(f.w.resources(2)->health==1000);}
 for(unsigned mode=0;mode<5;++mode){Fixture f;casting::Hooks h;unsigned calls=0,payments=0;h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[](const auto&){};h.target_scope=[&](unsigned id){return f.w.effect_scope(id);};h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};if(mode!=4)h.consume_overcast_energy=[&](unsigned skill,unsigned cost,auto event,auto now){++payments;
 check(skill==215&&cost==25);return f.w.activate_overcast_energy(f.caster,skill,cost,event,now);};if(mode!=3)h.complete_fire_storm=[&](auto target,const auto& p,auto now)->std::optional<std::vector<world::Packet>>{++calls;std::vector<world::Packet> out;if(!f.w.fire_storm(f.caster,target,p,20,now,&out))return {};return out;};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[215]=1;Slots bar{};bar[0]={215,0};attributes::Ranks ranks{};ranks[11]=15;casting::Runtime runtime(1);
 check(runtime.start(1000,{215,0,2},bar,policy,ranks,true,h)==(mode>=3?casting::Reason::unsupported:casting::Reason::accepted));if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1500,h);runtime.update(3000,h);
 check(calls==(mode==0?1:0)&&payments==(mode<3?1:0));if(mode==0){check(f.w.resources(2)->health==1000);
 f.w.tick(4000);
 check(f.w.resources(2)->health==975);}}
 std::cout<<"PASS Maelstrom ten fixed cold pulses,spell-only interruption,protection,scope and Runtime Overcast lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

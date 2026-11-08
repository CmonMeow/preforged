#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Balanced line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=balanced_stance(rank);
 check(p&&p->duration==1000ull*interpolate(8,20,rank));}
 for(bool immune:{false,true}){combat::weapon::Strike s;s.level=20;s.mastery=12;s.minimum=10;s.maximum=20;s.location_armor=60;s.base_roll=s.miss_roll=.5;s.critical=true;s.critical_immune=immune;const auto hit=combat::weapon::resolve(s);
 check(hit&&!hit->missed&&std::abs(hit->damage-(immune?15:20*std::sqrt(2.)))<1e-9);}
 for(unsigned mode=0;mode<8;++mode){const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto population=world::load_npc_catalog(appearance);std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/balanced_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 0 test\n");const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(population,catalog,hash);auto enemies=population.for_map(146);auto guardian=enemies.front();guardian.combat_definition->relation=combat::npcs::Relation::neutral;world::Instance w;w.configure_npcs({guardian});w.join(2,"Owner",100,0,std::make_shared<world::Outbox>());w.set_vitals(2,{1000,1000,100,100},100);const auto owner=*w.effect_scope(2),guard=*w.effect_scope(100);
 check(w.physical_armor(owner,60));
 check(w.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}));const auto attacker=*w.effect_scope(60000);w.configure_attack_miss_draws([]{return .9;});
 if(mode&&mode<5)check(w.status_mutation(owner,371,123,15,100,20100,true));
 if(mode==3)check(w.status_mutation(owner,349,124,0,200,8200,true));
 if(mode==4)check(w.status_mutation(owner,371,123,15,200,{},false));combat::weapon::Strike s;s.level=enemies.front().definition.level;s.mastery=12;s.minimum=10;s.maximum=20;s.base_roll=s.miss_roll=.5;s.critical=true;s.location_armor=60;const auto at=mode==2?20100:1000;
 if(mode>=5){check(w.apply_condition(owner,owner,combat::conditions::Kind::dazed,100,5000));
 check(w.cast_activity(owner,Slot{mode==6?1u:281u,17},200,3000));
 if(mode==7)check(w.status_mutation(owner,17,125,12,200,200+timing::stance(17,12)->duration,true));}
 const auto before_epoch=w.control_epoch(owner);
 check(w.damage_quest_player(attacker,owner,128,at,s,1000));auto expected=s;expected.critical_immune=mode==1;const double damage=combat::weapon::resolve(expected)->damage;
 check(w.resources(2)->health==unsigned(std::ceil(1000-damage)));
 if(mode>=5){check((w.control_epoch(owner)!=before_epoch)==(mode==5));
 check(bool(w.casting_slot(owner,1000))==(mode!=5));if(mode==7)check(w.resources(2)->energy==95);}}

 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
 check(w.status_mutation(b,371,123,15,100,20100,true));const auto epoch=w.control_epoch(b);
 check(w.gale(a,b,12,{},1000)&&w.control_epoch(b)==epoch);
 check(w.status_mutation(b,349,124,0,1100,9100,true));
 check(w.gale(a,b,12,{},1200)&&w.control_epoch(b)!=epoch);
 check(w.status_snapshot(b,1200)->size()==1);}
 for(bool missing:{false,true}){world::Instance w;w.join(1,"Player",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto owner=*w.effect_scope(1);casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy=[&](unsigned n){check(n==5);return w.consume_energy(1,n,owner);};h.effect_snapshot=[&](auto now){return *w.status_snapshot(owner,now);};if(!missing)h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return w.status_mutation(owner,skill,effect,rank,now,until,active);};Policy p;p.primary=1;p.campaigns=255;p.learned[371]=1;Slots bar{};bar[0]={371,0};attributes::Ranks ranks{};ranks[21]=15;casting::Runtime r(1);
 check(r.start(1000,{371,0,2},bar,p,ranks,true,h)==(missing?casting::Reason::unsupported:casting::Reason::accepted));if(!missing){r.update(1000,h);
 check(w.status_snapshot(owner,1000)->size()==1&&w.resources(1)->energy==95);
 check(w.kill(owner,1100));
 check(w.status_snapshot(owner,1100)->empty());}}
 std::cout<<"PASS Balanced Stance trusted NPC critical suppression,knockdown immunity,stance replacement,expiry,removal and Runtime/death\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

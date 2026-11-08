#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Rigor NPC casting regression line "+std::to_string(at.line()));}
int main()try{using namespace skills;
 const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto population=world::load_npc_catalog(appearance);
 std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/rigor_npc_cast.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 0 test\n");
 const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(population,catalog,hash);
 world::Instance w;w.configure_npcs(population.for_map(146));auto box=std::make_shared<world::Outbox>();
 w.join(1,"Necromancer",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
 const auto source=*w.effect_scope(1),target=*w.effect_scope(100);casting::Hooks hooks;unsigned commits=0;
 hooks.events=[](const auto&){};hooks.resources=[&](unsigned id){return w.resources(id);};
 hooks.consume_energy=[&](unsigned amount){return w.consume_energy(1,amount,source);};
 hooks.target_scope=[&](unsigned id){return w.effect_scope(id);};hooks.projectile_target_valid=[](unsigned id,float range){return id==100&&range==1248;};
 hooks.degeneration_hex=[&](auto selected,const auto& plan,auto now){++commits;return w.degeneration_hex(source,selected,plan,now);};
 Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[137]=1;Slots slots{};slots[0]={137,0};attributes::Ranks ranks{};ranks[7]=12;casting::Runtime cast(1);
 check(cast.start(100,{137,0,100},slots,policy,ranks,true,hooks)==casting::Reason::accepted&&w.resources(1)->energy==90);
 cast.update(1099,hooks);check(!commits&&!(w.npc_combat_snapshot(target)->effects&0x800));
 box->drain();cast.update(1100,hooks);cast.update(1101,hooks);check(commits==1&&(w.npc_combat_snapshot(target)->effects&0x800));
 const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(100,1,137))==1);
 w.tick(19000);check(w.npc_combat_snapshot(target)->effects&0x800); // Respect World's100ms tick cadence.
 w.tick(19100);check(!(w.npc_combat_snapshot(target)->effects&0x800)&&w.npc_combat_snapshot(target)->health->current==500);
 std::cout<<"PASS Rigor actual cast to NPC without playerResources,target248owned hex and exact expiry;NPC defensive block sources remain pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

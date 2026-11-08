#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Bile Profane frontier line "+std::to_string(at.line()));
}
std::shared_ptr<world::Outbox> add_player(world::Instance& w,unsigned id,float x,unsigned hp,unsigned team){
    auto box=std::make_shared<world::Outbox>();
    w.join(id,"Profane "+std::to_string(id),x,0,box,0,{}, {},100,team);
    w.set_vitals(id,{hp,1000,100,100},100);return box;
}
int main()try{
    std::optional<std::vector<world::Packet>> wire_reference;
    for(unsigned breeze_rank:{0u,12u})for(bool dense:{false,true}){
        world::Instance w;world::Instance::Npc corpse;corpse.id=corpse.definition.id=100;
        corpse.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
        d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={1,1};d.damageable=true;
        d.fleshy=true;d.fleshy_evidence="synthetic corpse";corpse.combat_definition=d;w.configure_npcs({corpse});
        auto box=add_player(w,1,0,1000,1);add_player(w,2,100,200,2);add_player(w,3,500,1000,2);
        add_player(w,10,100,1000,2);add_player(w,20,500,2,2);
        const auto source=*w.effect_scope(1),healer=*w.effect_scope(3),moving=*w.effect_scope(20);
        check(w.party_action(3,158,20,100)&&w.party_action(20,154,3,100));
        check(w.protective_enchantment(healer,moving,*skills::regeneration::reviewed(288,breeze_rank),0,100));
        const auto initial=*w.status_snapshot(moving,100);check(initial.size()==1&&initial[0].skill==288);
        const auto breeze_effect=initial[0].instance;
        check(w.degeneration_hex(source,*w.effect_scope(10),*skills::effects::degeneration_hex(2058,0),100,20));
        check(w.degeneration_hex(source,moving,*skills::effects::degeneration_hex(2058,15),100,20));
        check(w.damage_npc(source,*w.effect_scope(100),{combat::Kind::armor_ignoring,1000,20,194},1248,100).killed);
        check(w.claim_well_corpse(source,*skills::effects::well_cast_plan(94,12),1,100,252,[](const auto&){return true;}));
        w.move(20,0,0,100);box->drain();
        if(dense)for(unsigned at=200;at<10000;at+=100)w.tick(at);
        check(w.settle_resources(*w.effect_scope(2),10000));w.tick(10000);
        check(w.resources(20)->health==0&&w.resources(2)->health==127&&w.resources(10)->health==917);
        check(w.pending_putrid_deaths()==0);
        const auto status=w.status_snapshot(moving,10000);check(status&&status->empty());
        const auto packets=box->drain();std::vector<world::Packet> filtered;
        auto removed=packets.end();unsigned removal_count=0;
        const auto damage_marker=combat::detail::integer(10,1,2058);
        for(auto it=packets.begin();it!=packets.end();++it){
            bool removal=false;
            if(it->size()==10&&(*it)[0]==67&&(*it)[1]==0){
                std::uint32_t actor=0,effect=0;std::memcpy(&actor,it->data()+2,4);std::memcpy(&effect,it->data()+6,4);
                removal=actor==20&&effect==breeze_effect;
            }
            if(removal){removed=it;++removal_count;}
            if(removal||*it==damage_marker||(it->size()>2&&(*it)[0]==162&&(*it)[1]==0))filtered.push_back(*it);
        }
        const auto first_damage=std::find(packets.begin(),packets.end(),damage_marker);
        check(removal_count==1&&removed<first_damage&&first_damage!=packets.end());
        check(std::count(packets.begin(),packets.end(),damage_marker)==2);
        if(wire_reference)check(*wire_reference==filtered);else wire_reference=filtered;

    }
    std::cout<<"4 Profane cases remove Breeze at entry frontier before Bile forecasting; coarse/fine damage and no retained event\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

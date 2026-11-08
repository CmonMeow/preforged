#include "gw_npc_catalog.h"
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("escort owner readiness line "+std::to_string(at.line()));
}
int main()try{
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"data/gw/quests";
    std::ifstream appearance(root/"actors/monk-test-givers.txt"),metadata(root/"monk-test-giver-combat.txt");
    check(bool(appearance)&&bool(metadata));auto catalog=world::load_npc_catalog(appearance);
    const std::string hash="14b6cc1b3b97e5ad5b4bcb922486890c0675036440f23e93ff38d6ccb2001b1f";
    world::bind_combat_catalog(catalog,combat::npcs::load_catalog(metadata,hash),hash);
    auto participants=catalog.for_map(146);check(participants.size()==2);
    // Retain captured identities and unknown Health, use synthetic geometry.
    for(auto& npc:participants){npc.x=npc.id==20051?500:0;npc.y=0;}
    std::ifstream enemies_file(root/"actors/monk-test-skale.txt"),enemy_metadata(root/"monk-test-enemy-combat.txt");
    auto enemies=world::load_npc_catalog(enemies_file);
    const std::string enemy_hash="faa268470eba50f6fe4ba66517c51c6823d5ffc045afe51f0ee6665a5c02a5ec";
    world::bind_combat_catalog(enemies,combat::npcs::load_catalog(enemy_metadata,enemy_hash),enemy_hash);
    for(bool published:{false,true}){
        world::Instance w;w.configure_npcs(participants);auto output=std::make_shared<world::Outbox>();
        w.join(2,"Escort owner",0,0,output,0,{}, {},100,1,published);
        w.set_vitals(2,{published?0u:100u,100,20,20},100);
        const auto owner=*w.effect_scope(2),gwen=*w.effect_scope(20051),ciglo=*w.effect_scope(20050);
        check(w.spawn_escort_encounter(owner,gwen,ciglo,1,enemies.for_map(146),[]{return true;}));
        check(!w.discover_escort(owner,1,600,100));
        check(w.escort_objective_state(owner,1)==quests::EscortObjective::State::searching);
        check(w.clear_kill_objective(owner,84,1,true));
        check(w.restore_completed_escort_objective(owner,gwen,ciglo,146,1));output->drain();
        check(!w.follow_escort(owner,1,100,10,100));
        check(!w.arrive_escort(owner,1,600,100)&&!w.escort_reward_ready(owner,1));
        if(!published)w.npc_entry_packets(2);
        w.set_vitals(2,{100,100,20,20},100);
        check(w.follow_escort(owner,1,100,10,101));output->drain();
        // A route accepted while alive must stop when the owner dies.
        w.set_vitals(2,{0,100,20,20},100);w.tick(1101);
        const auto stopped=output->drain();
        const auto gwen_stop=[](const auto& packet){return packet.size()==6&&packet[0]==40&&packet[1]==0&&packet[2]==0x53&&packet[3]==0x4e;};
        check(std::count_if(stopped.begin(),stopped.end(),gwen_stop)==1);
        check(std::none_of(stopped.begin(),stopped.end(),[](const auto& packet){return packet.size()==16&&packet[0]==44&&packet[1]==0&&packet[2]==0x53&&packet[3]==0x4e;}));
        w.tick(1201);const auto idle=output->drain();check(std::none_of(idle.begin(),idle.end(),gwen_stop));
        check(!w.follow_escort(owner,1,100,10,1202));
        // Readiness resumes the existing attempt, without resetting progress.
        w.set_vitals(2,{100,100,20,20},100);
        check(w.follow_escort(owner,1,100,10,1203));w.tick(7203);
        check(w.arrive_escort(owner,1,100,7204)&&w.escort_reward_ready(owner,1));
    }
    std::cout<<"Escort owner readiness passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}


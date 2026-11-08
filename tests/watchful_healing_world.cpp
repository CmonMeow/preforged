#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Watchful Healing line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned rank=0;rank<=20;++rank){const auto p=effects::watchful_healing(rank);check(bool(p));
        check(p->pips==effects::interpolate(1,4,rank)&&p->gain==30+6*rank&&p->duration==10000);
        check(regeneration::end_healing(1392,rank)==0); // Health gain has its own end path.
        effects::Entry entry{1,1,1392,rank,effects::Category::enchantment,100,10100};
        check(effects::watchful_early_gain(entry,100,true)==p->gain);
        check(effects::watchful_early_gain(entry,10099,true)==p->gain);
        check(effects::watchful_early_gain(entry,10100,true)==0&&effects::watchful_early_gain(entry,20000,true)==0);
        check(effects::watchful_early_gain(entry,200,false)==0&&!effects::watchful_early_gain(entry,99,true));
        entry.expires.reset();check(!effects::watchful_early_gain(entry,200,true));}
    check(!effects::watchful_healing(21));
    for(unsigned mode=0;mode<9;++mode){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id*100,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);
            w.set_vitals(id,{id==2?500u:1000u,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
        check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2),foe=*w.effect_scope(3);
        const auto plan=*regeneration::reviewed(1392,12);
        check(!w.status_mutation(target,1392,77,12,100,10100,true));
        auto stale=target;++stale.generation;check(!w.protective_enchantment(source,stale,plan,0,100));
        auto forged=plan;++forged.pips;check(!w.protective_enchantment(source,target,forged,0,100));
        check(w.protective_enchantment(source,target,plan,0,100));
        const auto entry=w.status_snapshot(target,100)->front();check(entry.skill==1392&&entry.cause==1);
        check(!w.status_mutation(target,180,entry.instance,12,100,60100,true));
        box->drain();
        if(mode==0||mode==1){
            if(mode==1)check(w.degeneration_hex(foe,target,*effects::degeneration_hex(129,0),100));
            box->drain();check(w.enchantment_removal(foe,target,*effects::enchantment_removal(68,0),20,100));
            check(w.resources(2)->health==602);const auto packets=box->drain();
            const auto gain=skills::particles::health_change_2022(2,1,102,1000);
            world::Packet removed{67,0};world::u32(removed,2);world::u32(removed,entry.instance);
            check(std::count(packets.begin(),packets.end(),gain)==1);
            check(std::find(packets.begin(),packets.end(),gain)<std::find(packets.begin(),packets.end(),removed));
            check(w.enchantment_removal(foe,target,*effects::enchantment_removal(68,0),20,100)&&w.resources(2)->health==602);
        }else if(mode==2){w.tick(10100);check(w.resources(2)->health==560);w.tick(20100);check(w.resources(2)->health==560);}
        else if(mode==3){check(w.protective_enchantment(source,target,plan,0,1100));check(w.resources(2)->health==506);
            w.tick(11100);check(w.resources(2)->health==566);}
        else if(mode==4){check(w.kill(target,200));w.tick(20100);check(w.resources(2)->health==0);
            const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(2,1,102,1000))==0);}
        else if(mode==5){w.leave(1);w.join(1,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            box->drain();check(w.enchantment_removal(foe,target,*effects::enchantment_removal(68,0),20,100));check(w.resources(2)->health==602);
            const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(2,1,102,1000))==0);}
        else if(mode==6){
            std::vector<world::Packet> visuals;
            check(!w.stage_chilblains(foe,target,*effects::chilblains(12),effects::ChilblainsOrder::remove_then_damage,20,100,&visuals));
            check(w.resources(1)->health==1000&&w.resources(2)->health==500&&!w.has_condition(foe,combat::conditions::Kind::poison,100));
            check(w.stage_chilblains(foe,target,*effects::chilblains(12),20,100,&visuals));check(w.resources(2)->health==565);
            const auto packets=box->drain();const auto gain=skills::particles::health_change_2022(2,1,102,1000);
            const auto hit=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=18&&p[0]==162&&p[2]==16&&p[6]==2&&p[10]==3;});
            world::Packet removed{67,0};world::u32(removed,2);world::u32(removed,entry.instance);
            check(hit!=packets.end()&&hit<std::find(packets.begin(),packets.end(),gain));
            check(std::find(packets.begin(),packets.end(),gain)<std::find(packets.begin(),packets.end(),removed));
        }
        else if(mode==7){w.set_vitals(2,{950,1000,100,100},100);box->drain();
            check(w.enchantment_removal(foe,target,*effects::enchantment_removal(68,0),20,100)&&w.resources(2)->health==1000);
            const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(2,1,50,1000))==1);}
        else{check(!w.enchantment_removal(foe,target,*effects::enchantment_removal(68,0),20,99));
            check(w.resources(2)->health==500&&w.status_snapshot(target,100)->size()==1);
            check(w.enchantment_removal(foe,target,*effects::enchantment_removal(68,0),20,100)&&w.resources(2)->health==602);}
    }
    std::cout<<"PASS staged Watchful Healing ranks,regeneration,early Health gain,healing-reduction bypass,expiry,renewal,death and source generation guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


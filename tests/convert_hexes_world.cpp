#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Convert Hexes World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;Scope source,target,foe;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(){
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,id==1?box:std::make_shared<world::Outbox>(),0,
            id==1?std::vector<world::Packet>{{165,0,1,0,0,0,3,0}}:std::vector<world::Packet>{},{},100,id==3?2:1);w.set_vitals(id,{id==3?100u:500u,1000,100,100},100);}
        check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));source=*w.effect_scope(1);target=*w.effect_scope(2);foe=*w.effect_scope(3);
    }
};
int main()try{
    for(unsigned rank=0;rank<=20;++rank){Fixture f;const auto plan=*convert_hexes(rank);
        check(f.w.apply_parasitic_bond(f.foe,f.target,0,100));
        check(f.w.status_mutation(f.target,100,1001,0,100,30000,true));
        check(f.w.status_mutation(f.target,19,1002,0,100,30000,true));
        const auto before=*f.w.status_snapshot(f.target,100);f.box->drain();
        check(f.w.complete_convert_hexes(f.source,f.target,plan,0,100));
        const auto after=*f.w.status_snapshot(f.target,100);check(after.size()==1&&after.front().skill==303&&after.front().rank==rank);
        check(f.w.convert_hexes_armor(f.target,100)==10&&f.w.resources(3)->health==130); // Actual Parasitic Bond end return.
        const auto packets=f.box->drain();const auto body=*skills::particles::targeted_body_2022(2,1,303);
        check(std::count(packets.begin(),packets.end(),body)==1&&std::count(packets.begin(),packets.end(),combat::detail::integer(58,1,0))==1);
        for(const auto& entry:before){world::Packet end{67,0};combat::detail::put(end,2);combat::detail::put(end,entry.instance);check(std::count(packets.begin(),packets.end(),end)==1);}
        check(f.w.convert_hexes_armor(f.target,100+plan.duration-1)==10);
        check(f.w.convert_hexes_armor(f.target,100+plan.duration)==0&&f.w.status_snapshot(f.target,100+plan.duration)->empty());
    }
    {Fixture f;check(f.w.status_mutation(f.target,100,1001,0,100,30000,true));check(f.w.complete_convert_hexes(f.source,f.target,*convert_hexes(0),0,100));
        check(f.w.status_mutation(f.target,19,1003,0,101,30000,true));
        check(!f.w.complete_convert_hexes(f.source,f.target,*convert_hexes(0),0,101)); // Unknown zero-count refresh is explicitly staged.
        check(f.w.status_snapshot(f.target,101)->size()==2&&f.w.convert_hexes_armor(f.target,101)==10); // No partial removal on rejection.
        check(f.w.status_mutation(f.target,101,1002,0,101,30000,true));check(f.w.complete_convert_hexes(f.source,f.target,*convert_hexes(12),0,101));
        check(f.w.convert_hexes_armor(f.target,101)==10&&f.w.status_snapshot(f.target,101)->size()==1);
        check(f.w.kill(f.target,102)&&f.w.convert_hexes_armor(f.target,102)==0);}
    {Fixture f;check(f.w.status_mutation(f.target,19,1002,0,100,30000,true));check(f.w.complete_convert_hexes(f.source,f.target,*convert_hexes(12),12,100));
        check(f.w.convert_hexes_armor(f.target,100)==0&&f.w.resources(2)->health==538&&f.w.status_snapshot(f.target,100)->empty());}
    {Fixture f;auto stale=f.target;++stale.generation;check(!f.w.complete_convert_hexes(f.source,stale,*convert_hexes(0),0,100));
        check(!f.w.complete_convert_hexes(f.source,f.source,*convert_hexes(0),0,100));
        check(!f.w.complete_convert_hexes(f.source,f.foe,*convert_hexes(0),0,100));
        check(!f.w.status_mutation(f.target,303,123,0,100,1000,true));}
    for(unsigned base:{80u,500u}){Fixture f;check(f.w.status_mutation(f.target,100,1001,0,100,30000,true));check(f.w.complete_convert_hexes(f.source,f.target,*convert_hexes(0),0,100));
        check(f.w.lightning_armor(f.target,base)&&!f.w.lightning_armor(f.target,501));const auto before=f.w.resources(2)->health;
        check(f.w.lightning(f.foe,f.target,*lightning(224,12),20,100));
        const auto loss=before-f.w.resources(2)->health;check(std::abs(double(loss)-45*std::exp2(-.75*(double(base)-80+10)/40))<1.1);}
    {Fixture f;check(f.w.status_mutation(f.target,100,1001,0,100,30000,true));check(f.w.complete_convert_hexes(f.source,f.target,*convert_hexes(0),0,100));
        f.w.leave(2);f.w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},101,1);f.w.set_vitals(2,{500,1000,100,100},100);
        check(!f.w.convert_hexes_armor(f.target,101));check(f.w.convert_hexes_armor(*f.w.effect_scope(2),101)==0);}
    std::cout<<"PASS21 Convert Hexes World ranks,owned hex ends/Parasitic return,timed armor and real damage,refresh/death/DF/publication/identity guards;normal casting pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


#include "gw_world_simulation.h"
#include <bit>
#include <cmath>
#include <iostream>
using namespace fw::gw;
static void check(bool ok){if(!ok)throw std::runtime_error("captured player speed regression");}
static float scalar(const agent::Packet& p,unsigned n){return std::bit_cast<float>(unsigned(p[n])|(unsigned(p[n+1])<<8)|(unsigned(p[n+2])<<16)|(unsigned(p[n+3])<<24));}
int main()try{
    for(bool left:{false,true})for(unsigned elapsed:{100u,2200u,5000u}){
        world::Instance turn;auto box=std::make_shared<world::Outbox>();turn.join(1,"Turn",0,0,box);
        turn.steer(1,1,0,100);turn.rotate(1,left?std::numeric_limits<float>::infinity():-std::numeric_limits<float>::infinity(),1,100);box->drain();
        const auto full=turn.maintained_range_path(*turn.effect_scope(1),100,100+elapsed);
        const auto suffix=turn.maintained_range_path(*turn.effect_scope(1),150,100+elapsed);
        check(full&&suffix&&full->size()>2&&box->drain().empty());
        check(full->back().x==suffix->back().x&&full->back().y==suffix->back().y&&suffix->front().at==150);
        turn.tick(100+elapsed);const auto actual=turn.position(1,100+elapsed);
        check(std::abs(full->back().x-actual.first)<.001&&std::abs(full->back().y-actual.second)<.001);
    }
    // Independent capture assertions; do not derive expected values from production constants.
    check(scalar(agent::spawn_2022(1,0x30000001,0,0),31)==288.f);
    check(scalar(agent::spawn_2022(2,0x20000001,0,0),31)==220.f);
    for(unsigned gait=1;gait<=8;++gait){
        world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Runner",0,0,box);
        w.steer(1,1,0,100,gait);w.tick(1100);
        check(std::abs(w.position(1,1100).first-288.f*(gait<=3?1.f:gait<=6?.66f:.75f))<.05f);
    }
    world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Runner",0,0,box);
    w.move(1,10000,0,100);w.tick(1100);check(std::abs(w.position(1,1100).first-288)<.1);
    check(w.movement_multiplier(1,1.33f,1100));w.tick(2100);
    check(std::abs(w.position(1,2100).first-671.04f)<.2);
    auto peer=std::make_shared<world::Outbox>();const auto snapshot=w.join(2,"Peer",0,0,peer);
    bool published=false;for(const auto& p:snapshot.at(0).presentation)if(p.size()==10&&p[0]==39&&std::abs(scalar(p,6)-383.04f)<.01f)published=true;
    check(published);check(w.movement_multiplier(1,.5f,2100));w.tick(3100);
    check(std::abs(w.position(1,3100).first-815.04f)<.3);
    world::Instance reports;auto report_box=std::make_shared<world::Outbox>();reports.join(1,"Reports",0,0,report_box);
    using S=world::Instance::CheckpointStatus;using R=world::Instance::SteeringReport;
    check(reports.steer(1,1,0,100,1,R{0,0,0}).status==S::baseline);
    check(reports.steer(1,0,0,100,1,R{28.8f,0,0}).status==S::accepted);
    reports.steer(1,1,0,100,1,R{28.8f,0,0});
    check(reports.steer(1,0,0,100,1,R{57.6f,0,0}).status==S::over_budget);
    std::cout<<"PASS captured288 baseline, gaits, boost/snare integration, peer speed, bounded prediction lead\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

#include "gw_world_simulation.h"
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("pet layered World line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& p,unsigned at){return unsigned(p.at(at))|unsigned(p.at(at+1))<<8|unsigned(p.at(at+2))<<16|unsigned(p.at(at+3))<<24;}
int main(int argc,char** argv)try{
    if(argc!=2)throw std::runtime_error("build fixture path required");
    const std::filesystem::path file=argv[1];std::ofstream out(file,std::ios::binary);
    auto u32=[&](unsigned value){for(unsigned i=0;i<4;++i)out.put(char(value>>(8*i)));};
    auto f32=[&](float value){u32(std::bit_cast<unsigned>(value));};
    out.write("FWNAV001",8);for(unsigned value:{1u,7u,0xeefe704cu,12u,1u,1u})u32(value);
    f32(0);f32(0);f32(3000);f32(100);u32(2);
    auto cell=[&](float left,float right){for(unsigned i=0;i<5;++i)u32(0xffffffff);f32(0);f32(100);f32(left);f32(right);f32(left);f32(right);};
    u32(2);cell(0,1000);cell(2000,3000);u32(1);cell(1000,2000);out.close();
    const auto links=std::filesystem::path(file.string()+".links");
    {std::ofstream stream(links);stream<<"FWLINK01 7 2 4\n0 0 1 0\n1 0 0 0\n1 0 0 1\n0 1 1 0\n";}
    auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(file));
    world::Instance w;w.configure_navigation(mesh);auto box=std::make_shared<world::Outbox>();
    w.join(1,"Layered owner",500,50,box,0,{}, {},100,1);w.set_vitals(1,{100,100,30,30},100);
    const auto owner=*w.effect_scope(1);check(w.restore_pet_profile(owner,1,pets::Profile{pets::Pet{}},1));
    check(w.pet_owner_combat(owner,1,0,1000)&&w.pet_instance_mode(owner,false,1000));
    check(w.reconcile_pet_bar_mode(owner,162,true,false,std::array{436u},1000));const auto pet=*w.owned_pet_scope(owner);box->drain();
    w.move(1,2500,50,1000);bool bridge=false,north=false,layered_comfort=false;unsigned pet_plane=0;float last_x=500;
    for(unsigned now=1000;now<=12000;now+=100){w.tick(now);
        for(const auto& packet:box->drain())if(packet.size()==16&&packet[0]==44&&word(packet,2)==pet.actor){
            last_x=std::bit_cast<float>(word(packet,6));const unsigned plane=unsigned(packet[14])|unsigned(packet[15])<<8;
            pet_plane=plane;bridge|=plane==1;north|=plane==0&&last_x>2000;
        }
        if(!layered_comfort&&w.plane(1)!=pet_plane){
            check(w.pet_comfort_ready(owner,pet,now));
            const auto comfort=w.complete_pet_comfort(owner,pet,0,123,true,now,true);
            check(comfort.accepted&&!comfort.revived&&bool(comfort.publication));comfort.publication();layered_comfort=true;
        }
    }
    check(layered_comfort&&bridge&&north&&last_x>=2399&&last_x<=2501);
    // Preserve current owned scope and resume planar following after BOTH layer
    // crossings; there is no teleport to the owner or guessed plane change.
    check(w.owned_pet_scope(owner)==pet);
    w.move(1,500,50,12000);bool bridge_back=false,south=false;
    for(unsigned now=12000;now<=23000;now+=100){w.tick(now);
        for(const auto& packet:box->drain())if(packet.size()==16&&packet[0]==44&&word(packet,2)==pet.actor){
            last_x=std::bit_cast<float>(word(packet,6));const unsigned plane=unsigned(packet[14])|unsigned(packet[15])<<8;
            bridge_back|=plane==1;south|=plane==0&&last_x<1000;
        }
    }
    check(bridge_back&&south&&last_x>=499&&last_x<=601);
    std::filesystem::remove(links);std::filesystem::remove(file);
    std::cout<<"Default restored pet follows over reviewed layered links in both directions without actor replacement\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

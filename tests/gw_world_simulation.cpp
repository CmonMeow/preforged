#include "gw_world_simulation.h"
#include "gw_spawn.h"
#include "gw_outgoing_frame.h"
#include "gw_item_wire.h"
#include "gw_equipment_wire.h"
#include "gw_equipment_assignments.h"
#include "gw_character_profile.h"
#include "gw_skill_casting.h"
#include "gw_basic_armor_energy.h"

#include <cmath>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <bit>
#include <array>
#include <thread>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
bool close(float left, float right) { return std::abs(left - right) < 0.01f; }
float packet_float(const fw::gw::world::Packet& packet,std::size_t offset){
    std::uint32_t bits=0;for(unsigned i=0;i<4;++i)bits|=std::uint32_t(packet.at(offset+i))<<(8*i);
    return std::bit_cast<float>(bits);
}
std::uint32_t packet_u32(const fw::gw::world::Packet& packet,std::size_t offset){
    std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::uint32_t(packet.at(offset+i))<<(8*i);return value;
}
unsigned packet_count(const std::vector<fw::gw::world::Packet>& packets,unsigned opcode){
    return static_cast<unsigned>(std::count_if(packets.begin(),packets.end(),[&](const auto& p){return p.size()>=2&&
        (unsigned(p[0])|(unsigned(p[1])<<8))==opcode;}));
}
double health_from_packets(const std::vector<fw::gw::world::Packet>& packets,std::uint32_t actor,unsigned maximum){
    for(auto it=packets.rbegin();it!=packets.rend();++it)if(it->size()==14&&it->at(0)==161&&it->at(1)==0&&
        packet_u32(*it,2)==34&&packet_u32(*it,6)==actor)return double(packet_float(*it,10))*maximum;
    throw std::runtime_error("missing authoritative health fraction");
}
void write_u32(std::ostream& out, std::uint32_t value) {
    const std::array<char,4> b{char(value),char(value>>8),char(value>>16),char(value>>24)};
    out.write(b.data(),b.size());
}
void write_f32(std::ostream& out, float value) { write_u32(out,std::bit_cast<std::uint32_t>(value)); }
std::shared_ptr<const fw::gw::navigation::Mesh> fixture_mesh(float cell_width=10) {
    auto path=std::filesystem::temp_directory_path()/"fw-gw-world-nav-test.fwnav";
    std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write("FWNAV001",8);
    write_u32(out,1);write_u32(out,7);write_u32(out,0xEEFE704c);write_u32(out,12);write_u32(out,1);
    write_u32(out,1);write_f32(out,0);write_f32(out,0);write_f32(out,3*cell_width);write_f32(out,10);
    write_u32(out,1);write_u32(out,3);
    for(int i=0;i<3;++i){write_u32(out,std::bit_cast<std::uint32_t>(i?i-1:-1));
        write_u32(out,std::bit_cast<std::uint32_t>(i<2?i+1:-1));write_u32(out,0xffffffff);write_u32(out,0xffffffff);
        write_u32(out,0xffffffff);write_f32(out,0);write_f32(out,10);write_f32(out,i*cell_width);
        write_f32(out,(i+1)*cell_width);write_f32(out,i*cell_width);write_f32(out,(i+1)*cell_width);}
    out.close();return std::make_shared<const fw::gw::navigation::Mesh>(fw::gw::navigation::Mesh::load(path));
}
std::shared_ptr<const fw::gw::navigation::Mesh> wall_tip_mesh() {
    const auto path=std::filesystem::temp_directory_path()/"fw-gw-world-wall-tip-test.fwnav";
    std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write("FWNAV001",8);
    for(auto value:{1u,7u,0xEEFE704cu,12u,1u,1u})write_u32(out,value);
    for(float value:{0.f,0.f,1000.f,1000.f})write_f32(out,value);
    write_u32(out,1);
    write_u32(out,3);
    for(unsigned i=0;i<3;++i){
        write_u32(out,i?i-1:0xffffffff);write_u32(out,i<2?i+1:0xffffffff);
        for(unsigned n=0;n<3;++n)write_u32(out,0xffffffff);
        const float left=i==0?0.f:i==1?100.f:200.f,right=i==0?100.f:i==1?200.f:1000.f;
        for(float value:{0.f,i==0?10.f:1000.f,left,right,left,right})write_f32(out,value);
    }
    out.close();return std::make_shared<const fw::gw::navigation::Mesh>(fw::gw::navigation::Mesh::load(path));
}
std::shared_ptr<const fw::gw::navigation::Mesh> party_healing_mesh(){
    const auto path=std::filesystem::temp_directory_path()/"fw-gw-party-healing-test.fwnav";
    std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write("FWNAV001",8);
    for(auto value:{1u,7u,0xEEFE704cu,12u,1u,1u})write_u32(out,value);
    for(float value:{0,0,1000,400})write_f32(out,value);write_u32(out,2);
    // Two disconnected rooms on floor0 and an overlapping floor1. Party
    // area is not a navigation route, single-target plane or LoS restriction.
    write_u32(out,2);
    for(unsigned i=0;i<2;++i){for(unsigned n=0;n<5;++n)write_u32(out,0xffffffffu);
        for(float value:{0.f,400.f,i?220.f:0.f,i?1000.f:200.f,i?220.f:0.f,i?1000.f:200.f})write_f32(out,value);}
    write_u32(out,1);for(unsigned n=0;n<5;++n)write_u32(out,0xffffffffu);
    for(float value:{0,400,0,1000,0,1000})write_f32(out,value);
    out.close();return std::make_shared<const fw::gw::navigation::Mesh>(fw::gw::navigation::Mesh::load(path));
}
void fails_with(const auto& action, const char* expected) {
    try { action(); }
    catch (const std::runtime_error& error) {
        require(std::string(error.what()) == expected, "unexpected simulation rejection");
        return;
    }
    throw std::runtime_error("simulation accepted an invalid operation");
}
}

int main() try {
    {
        using namespace fw::gw::world;
        Instance world;auto owner=std::make_shared<Outbox>(),peer=std::make_shared<Outbox>();
        world.join(1,"Owner",0,0,owner);world.join(2,"Peer",10,0,peer);
        owner->drain();peer->drain();const auto scope=*world.effect_scope(1);
        require(world.equipment_visual(scope,1,544,12),"owned equipment visual publishes");
        const auto first=peer->drain();require(first.size()==2&&first[0][0]==159,"peer receives cosmetic pair without private item IDs");
        owner->drain();
        require(world.equipment_color(scope,1,8),"dye updates existing public armor");
        fw::gw::equipment::Assignments owned{};owned[2]=123;owned[4]=124;
        owner->enqueue(fw::gw::equipment::restore_assignments(1,owned));
        const auto dyed=owner->drain();
        require(dyed.size()==3&&dyed[0][0]==159&&dyed[1][0]==110&&dyed[2][0]==110,
                "owner associations follow dye rebuild, including unchanged pieces");
        const auto public_dye=peer->drain();
        require(public_dye.size()==1&&public_dye[0][0]==159,"dye peer does not receive owned item assignments");
        require(world.equipment_visual(scope,1,0,0),"removed equipment clears public appearance");
        require(peer->drain().size()==2,"removal reaches existing peer");
        const auto existing=world.join(3,"Late Peer",20,0,std::make_shared<Outbox>());
        const auto actor=std::find_if(existing.begin(),existing.end(),[](const auto& p){return p.id==1;});
        require(actor!=existing.end()&&actor->presentation.size()==2&&packet_u32(actor->presentation[0],14)==0,"late peer sees removed equipment");
        auto stale=scope;++stale.generation;
        require(!world.equipment_visual(stale,1,544,12),"obsolete world scope cannot change appearance");
    }
    {
        using namespace fw::gw::world;using namespace fw::gw::skills::effects;using fw::gw::combat::conditions::Kind;
        Instance searing;auto out=std::make_shared<Outbox>();searing.join(1,"Caster",0,0,out,0,{}, {},100,1);
        searing.join(2,"Burned Foe",10,0,std::make_shared<Outbox>(),0,{}, {},100,2);searing.join(3,"Clean Foe",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        searing.join(4,"Far Foe",263,0,std::make_shared<Outbox>(),0,{}, {},100,2);searing.join(5,"Ally",20,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        for(unsigned id=1;id<=5;++id)searing.set_vitals(id,{480,480,40,40},100);const auto caster=*searing.effect_scope(1),target=*searing.effect_scope(2);
        require(searing.elemental_armor(target,60,60)&&searing.apply_condition(caster,target,Kind::burning,100,20000),"Searing pre-existing burn and actual armor");out->drain();
        require(searing.elemental(caster,target,*elemental(884,12,12),20,100),"Searing conditional per-recipient split");const auto packets=out->drain();
        require(searing.resources(2)->health==398&&searing.resources(3)->health==480&&searing.resources(4)->health==480&&searing.resources(5)->health==480,"Searing damages only already-Burning foe and respects radius/team");
        require(searing.has_condition(*searing.effect_scope(3),Kind::burning,100)&&!searing.has_condition(*searing.effect_scope(4),Kind::burning,100),"Searing clean adjacent foe gets Burning without fabricated initial damage");
        require(std::find(packets.begin(),packets.end(),*fw::gw::skills::particles::target_body_2022(2,884))!=packets.end()&&std::find(packets.begin(),packets.end(),*fw::gw::skills::particles::target_body_2022(3,884))!=packets.end(),"Searing original victim resources reach both branches");
        searing.tick(1100);require(searing.resources(2)->health==384&&searing.resources(3)->health==466,"Searing actual burning degeneration");
        require(!searing.elemental(caster,{target.instance,target.actor,target.generation+1},*elemental(884,12,12),20,1100),"Searing recycled victim rejected");
        searing.tick(6100);const auto finished=searing.resources(3)->health;searing.tick(7100);require(searing.resources(3)->health==finished&&finished==396,"Searing six-second new burn ends without extending pre-existing timer");
    }
    {
        using namespace fw::gw::world;using namespace fw::gw::skills::effects;
        Instance glowing;auto out=std::make_shared<Outbox>();glowing.join(1,"Caster",0,0,out,0,{}, {},100,1);glowing.join(2,"Foe",10,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        glowing.set_vitals(1,{480,480,10,40},100);glowing.set_vitals(2,{480,480,40,40},100);const auto caster=*glowing.effect_scope(1),target=*glowing.effect_scope(2);glowing.elemental_armor(target,60,60);
        require(glowing.status_mutation(target,31,77,12,100,2100,true),"owned non-Water hex prerequisite fixture");
        require(glowing.elemental(caster,target,*elemental(2192,12,13),20,100)&&glowing.resources(1)->energy==10,"Glowing Ice cannot repay from arbitrary hex");
        require(glowing.status_mutation(target,211,78,12,100,2100,true),"owned Water-Magic hex prerequisite fixture, not a claim of Ice Spikes casting implementation");out->drain();
        require(glowing.elemental(caster,target,*elemental(2192,12,13),20,100)&&glowing.resources(1)->energy==21&&glowing.resources(2)->health==398,"Glowing Ice actual cold damage and Storage-derived gain");
        const auto packets=out->drain();require(std::find(packets.begin(),packets.end(),*fw::gw::skills::particles::target_body_2022(2,2192))!=packets.end(),"Glowing Ice pinned victim particle publication");
        require(glowing.elemental(caster,target,*elemental(2192,12,13),20,2100)&&glowing.resources(1)->energy==21,"expired Water hex cannot repay Glowing Ice");
    }
    for(unsigned id:{828u,864u}){
        using namespace fw::gw::world;using namespace fw::gw::skills::effects;using fw::gw::combat::conditions::Kind;
        Instance conditional;auto caster_out=std::make_shared<Outbox>();conditional.join(1,"Caster",0,0,caster_out,0,{}, {},100,1);
        conditional.join(2,"Target",10,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        conditional.join(3,"Clean Foe",30,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        conditional.join(4,"Marked Foe",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        conditional.join(5,"Ally",20,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        conditional.join(6,"Far Foe",177,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        for(unsigned actor=1;actor<=6;++actor)conditional.set_vitals(actor,{480,480,40,40},100);
        const auto caster=*conditional.effect_scope(1),target=*conditional.effect_scope(2);const auto plan=*condition_damage(id,12);
        require(conditional.elemental_armor(target,60,60),"reviewed target cold armor");
        require(conditional.apply_condition(caster,target,Kind::weakness,100,20000),"prior target condition");
        require(conditional.apply_condition(caster,*conditional.effect_scope(4),Kind::blind,100,20000),"prior adjacent condition");caster_out->drain();
        require(conditional.condition_damage(caster,target,plan,20,100),"reviewed conditional damage actual complete effect");
        const auto effects=caster_out->drain();require(packet_count(effects,65)==(id==828?1:3),"Vile conditional hex or Oppressive Poison plus renewed Weakness");
        require(conditional.resources(2)->health==480-plan.damage,"actual initial cold or shadow damage");conditional.tick(1100);
        require(conditional.resources(2)->health==(id==828?418u:446u),"conditional hex degeneration or Poison applies after initial damage");
        require(conditional.resources(3)->health==(id==828?480u:454u)&&conditional.resources(4)->health==(id==828?480u:446u)&&conditional.resources(5)->health==480&&conditional.resources(6)->health==480,"individual prerequisite snapshots, adjacent boundary and ally exclusion");
        require(!conditional.has_condition(*conditional.effect_scope(3),Kind::poison,1100),"Oppressive cannot grant prerequisite via another recipient");
        auto forged=plan;++forged.damage;require(!conditional.condition_damage(caster,target,forged,20,1100),"forged conditional damage rejected");
        require(!conditional.condition_damage(caster,{target.instance,target.actor,target.generation+1},plan,20,1100),"conditional stale target rejected");
        conditional.tick(10100);const auto end=conditional.resources(2)->health;conditional.tick(11100);require(conditional.resources(2)->health==end,"conditional secondary duration ends exactly");
    }
    {
        using namespace fw::gw::world;using namespace fw::gw::skills::effects;using fw::gw::combat::conditions::Kind;
        Instance clean;auto out=std::make_shared<Outbox>();clean.join(1,"Caster",0,0,out,0,{}, {},100,1);clean.join(2,"Foe",10,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        clean.set_vitals(1,{480,480,40,40},100);clean.set_vitals(2,{480,480,40,40},100);const auto caster=*clean.effect_scope(1),target=*clean.effect_scope(2);const auto plan=*condition_damage(828,12);
        require(!clean.condition_damage(caster,target,plan,20,100)&&clean.resources(2)->health==480,"unknown cold armor prevents fake damage");
        clean.elemental_armor(target,60,60);out->drain();require(clean.condition_damage(caster,target,plan,20,100),"unconditioned Vile initial strike still succeeds");
        require(packet_count(out->drain(),65)==0,"unconditioned Vile never fabricates a hex");clean.tick(1100);require(clean.resources(2)->health==426,"unconditioned Vile no degeneration");
        require(clean.apply_condition(caster,target,Kind::weakness,1100,20000)&&clean.condition_damage(caster,target,plan,20,1100),"conditioned Vile hex begins");
        require(clean.remove_effects(target,2,*removal(301,0,0),1100),"Vile hex removable");clean.tick(2100);require(clean.resources(2)->health==372,"removed Vile cannot retain degeneration");
        clean.set_vitals(2,{10,480,40,40},100);require(clean.condition_damage(caster,target,plan,20,2100)&&clean.resources(2)->health==0,"lethal Vile does not apply postmortem hex");
    }
    {
        using namespace fw::gw::world;using namespace fw::gw::skills::effects;
        Instance mutual;mutual.join(1,"Caster A",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);mutual.join(2,"Caster B",10,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        mutual.set_vitals(1,{100,480,40,40},100);mutual.set_vitals(2,{100,480,40,40},100);
        const auto a=*mutual.effect_scope(1),b=*mutual.effect_scope(2);const auto plan=*degeneration_hex(109,12);
        require(mutual.degeneration_hex(a,b,plan,100)&&mutual.degeneration_hex(b,a,plan,100),"mutual same-skill drain preserves positive and negative independent sources");mutual.tick(1100);
        require(mutual.resources(1)->health==100&&mutual.resources(2)->health==100,"mutual life drain nets zero without recursive clock advancement");
        require(mutual.remove_effects(a,1,*removal(301,0,0),1100),"remove one mutual drain");mutual.tick(2100);
        require(mutual.resources(1)->health==106&&mutual.resources(2)->health==94,"removing incoming hex retains outgoing caster benefit");
        mutual.join(3,"Caster C",20,0,std::make_shared<Outbox>(),0,{}, {},2100,1);mutual.set_vitals(3,{100,480,40,40},100);
        require(mutual.degeneration_hex(*mutual.effect_scope(3),b,plan,2100),"cross-caster renewal replaces target ownership");mutual.tick(3100);
        require(mutual.resources(1)->health==106&&mutual.resources(3)->health==106&&mutual.resources(2)->health==88,"renewal revokes old caster benefit and grants actual new caster");
        mutual.tick(25100);const auto expired=mutual.resources(3)->health;mutual.tick(26100);
        require(mutual.resources(3)->health==expired,"natural target expiry cannot leave orphan caster regeneration");
    }
    {
        using namespace fw::gw::world;using namespace fw::gw::skills::effects;
        Instance distance;distance.join(1,"Caster",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);distance.join(2,"Foe",1240,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        distance.set_vitals(1,{100,480,40,40},100);distance.set_vitals(2,{480,480,40,40},100);
        require(distance.degeneration_hex(*distance.effect_scope(1),*distance.effect_scope(2),*degeneration_hex(109,12),100),"Siphon maximum casting distance");distance.move(2,6500,0,100);distance.tick(17100);
        const auto before=distance.resources(1)->health;distance.tick(17200);const auto ended=distance.resources(1)->health;distance.tick(18200);
        require(ended>=before&&distance.resources(1)->health==ended,"Siphon compass crossing ends benefit before natural expiry");
    }
    {
        using namespace fw::gw::world;using namespace fw::gw::skills::effects;
        Instance drain;auto caster_out=std::make_shared<Outbox>();
        drain.join(1,"Drainer",0,0,caster_out,0,{}, {},100,1);
        for(unsigned id=2;id<=4;++id)drain.join(id,"Foe",float(id*10),0,std::make_shared<Outbox>(),0,{}, {},100,2);
        drain.join(5,"Ally",20,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        drain.set_vitals(1,{100,480,40,40},100);for(unsigned id=2;id<=5;++id)drain.set_vitals(id,{480,480,40,40},100);
        const auto caster=*drain.effect_scope(1),foe=*drain.effect_scope(2);
        caster_out->drain();require(drain.degeneration_hex(caster,foe,*degeneration_hex(126,12),100),"Life Transfer actual adjacent linked commit");
        const auto effects=caster_out->drain();require(packet_count(effects,65)==3,"Life Transfer has target effects but no caster effects-monitor entry");
        drain.tick(1100);require(drain.resources(1)->health==120&&drain.resources(2)->health==466&&drain.resources(3)->health==466&&drain.resources(5)->health==480,"per-foe regeneration combines with joint ten-pip cap and excludes allies");
        require(drain.degeneration_hex(caster,foe,*degeneration_hex(126,12),1100),"Life Transfer renewal");drain.tick(2100);
        require(drain.resources(1)->health==140,"renewal does not duplicate linked benefit");
        require(drain.kill(caster,2600),"caster death linked cleanup");drain.tick(3100);
        require(drain.resources(2)->health==445&&drain.resources(3)->health==445,"caster death ends all dependent target hexes preserving elapsed degeneration");
    }
    {
        using namespace fw::gw::world;using namespace fw::gw::skills::effects;
        Instance siphon;siphon.join(1,"Drainer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        for(unsigned id=2;id<=3;++id)siphon.join(id,"Foe",float(id*10),0,std::make_shared<Outbox>(),0,{}, {},100,2);
        siphon.set_vitals(1,{100,480,40,40},100);for(unsigned id=2;id<=3;++id)siphon.set_vitals(id,{480,480,40,40},100);
        const auto caster=*siphon.effect_scope(1),foe=*siphon.effect_scope(2),other=*siphon.effect_scope(3);
        require(siphon.degeneration_hex(caster,foe,*degeneration_hex(109,12),100)&&siphon.degeneration_hex(caster,other,*degeneration_hex(109,12),100),"same-skill different-foe benefits independent");
        siphon.tick(1100);require(siphon.resources(1)->health==112,"Life Siphon cumulative six pips");
        require(siphon.remove_effects(foe,2,*removal(301,0,0),1600),"remove one linked hex between caster ticks");siphon.tick(2100);
        require(siphon.resources(1)->health==121&&siphon.resources(2)->health==471&&siphon.resources(3)->health==468,"early removal retains nine HP accrued from independently timed benefits");
        siphon.leave(3);siphon.tick(3100);require(siphon.resources(1)->health==121,"target departure stops remaining linked benefit");
        siphon.join(3,"Recycled Foe",30,0,std::make_shared<Outbox>(),0,{}, {},3100,2);siphon.set_vitals(3,{480,480,40,40},100);siphon.tick(4100);
        require(siphon.resources(1)->health==121&&siphon.resources(3)->health==480,"recycled actor inherits neither hex nor benefit");
    }
    for(unsigned skill:{31u,859u,108u,2054u}){
        using namespace fw::gw::world;using namespace fw::gw::skills::effects;
        Instance hexes;hexes.join(1,"Hex Caster",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        hexes.join(2,"Target",10,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        hexes.join(3,"Nearby Foe",260,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        hexes.join(4,"Ally",10,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        hexes.join(5,"Beyond Radius",263,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        for(unsigned id=1;id<=5;++id)hexes.set_vitals(id,{480,480,40,40},100);
        const auto caster=*hexes.effect_scope(1),foe=*hexes.effect_scope(2);const auto plan=*degeneration_hex(skill,12);
        require(hexes.degeneration_hex(caster,foe,plan,100),"reviewed degeneration hex actual commit");hexes.tick(1100);
        require(hexes.resources(2)->health==480-2*plan.pips,"native-scaled hex degeneration");
        require(hexes.resources(3)->health==(skill==108?480-2*plan.pips:480)&&hexes.resources(4)->health==480&&hexes.resources(5)->health==480,"Suffering nearby foes only, other hexes single target");
        auto invalid=plan;++invalid.pips;require(!hexes.degeneration_hex(caster,foe,invalid,1100),"forged hex magnitude rejected");
        require(!hexes.degeneration_hex(caster,{foe.instance,foe.actor,foe.generation+1},plan,1100),"stale hex target lifetime rejected");
        if(skill==2054){hexes.tick(10100);require(hexes.resources(2)->health==420&&hexes.has_condition(foe,fw::gw::combat::conditions::Kind::cracked_armor,10100),"Shrinking Armor full degeneration then real end condition");hexes.tick(27100);require(!hexes.has_condition(foe,fw::gw::combat::conditions::Kind::cracked_armor,27100),"Cracked Armor exact end lifetime");}
        else{hexes.tick(100+plan.duration);require(hexes.resources(2)->health==480-2*plan.pips*plan.duration/1000,"delayed tick integrates full bounded duration");}
    }
    {
        using namespace fw::gw::world;using namespace fw::gw::skills::effects;
        Instance overlap;overlap.join(1,"Caster",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);overlap.join(2,"Target",10,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        overlap.set_vitals(1,{480,480,40,40},100);overlap.set_vitals(2,{480,480,40,40},100);const auto caster=*overlap.effect_scope(1),foe=*overlap.effect_scope(2);
        require(overlap.degeneration_hex(caster,foe,*degeneration_hex(31,12),100)&&overlap.degeneration_hex(caster,foe,*degeneration_hex(859,12),100),"two independent hex sources");overlap.tick(1100);
        require(overlap.resources(2)->health==460,"joint degeneration cap is ten pips, not per-source");
        require(overlap.degeneration_hex(caster,foe,*degeneration_hex(31,12),1100),"renewal refreshes cover position");
        require(overlap.remove_effects(foe,2,*removal(301,0,0),1100),"newest renewed hex removable");overlap.tick(2100);require(overlap.resources(2)->health==444,"renewal moved Phantasm above Nightmare and removal leaves eight pips");
        require(overlap.degeneration_hex(caster,foe,*degeneration_hex(2054,12),2100),"Shrinking early removal fixture");
        require(overlap.remove_effects(foe,2,*removal(301,0,0),2100)&&overlap.has_condition(foe,fw::gw::combat::conditions::Kind::cracked_armor,2100),"actual early hex removal invokes Cracked Armor");
        require(overlap.kill(foe,2100)&&!overlap.has_condition(foe,fw::gw::combat::conditions::Kind::cracked_armor,2100),"corpse removes active end condition");
    }
    {
        using namespace fw::gw::world;Instance group;
        auto one=std::make_shared<Outbox>(),two=std::make_shared<Outbox>(),outside=std::make_shared<Outbox>();
        group.join(1,"Consumer",0,0,one);group.join(2,"Party Ally",10000,0,two);group.join(3,"Stranger",0,0,outside);
        group.set_vitals(1,{400,500,30,66},100);group.set_vitals(2,{400,480,10,20},100);group.set_vitals(3,{400,480,10,20},100);
        const auto a=*group.effect_scope(1),b=*group.effect_scope(2),c=*group.effect_scope(3);
        auto continuity=std::make_shared<fw::gw::consumables::continuity::Morale>();
        require(group.bind_morale(a,continuity,false)==100,"actual world actor claims owned continuity");
        require(group.reviewed_health_capacity(a,480,20,0)&&group.energy_capacity(1,6,12,0,{10,2}),"reviewed morale base capacities initialized");
        require(group.reviewed_health_capacity(b,480,0,0)&&group.energy_capacity(2,1,0,0),"ally morale capacity metadata initialized");
        require(group.reviewed_health_capacity(c,480,0,0)&&group.energy_capacity(3,1,0,0),"stranger capacity metadata initialized");
        require(group.party_action(1,158,2,0)&&group.party_action(2,154,1,0),"consumable test party formed");
        unsigned calls=0;
        require(!group.honeycomb(a,0,false,[&]{++calls;return true;})&&calls==0,"PvP denial cannot debit consumable");
        require(!group.honeycomb(a,0,true,[&]{++calls;return false;})&&group.resources(1)->max_health==500&&group.resources(2)->max_health==480,"failed owned commit leaves entire party unchanged");
        require(group.honeycomb(a,0,true,[&]{++calls;return true;}),"reviewed owned commit enables party morale");
        require(group.resources(1)->max_health==524&&group.resources(1)->health==424&&group.resources(1)->max_energy==68&&group.resources(1)->energy==32,"morale applies base and ES excludes equipment preserving deficit");
        require(group.resources(2)->max_health==504&&group.resources(3)->max_health==480,"party boost ignores distance and excludes unrelated players");
        require(group.honeycomb(a,0,true,[]{return true;})&&group.resources(1)->max_health==548&&group.resources(1)->max_energy==71,"second use reaches ten percent cap with explicit integer policy");
        const auto before=calls;require(!group.honeycomb(a,0,true,[&]{++calls;return true;})&&calls==before,"fully capped party cannot consume another item");
        require(group.reviewed_health_capacity(a,480,20,0)&&group.resources(1)->max_health==548&&group.energy_capacity(1,6,12,0,{10,2})&&group.resources(1)->max_energy==71,"equipment refresh preserves morale capacities");
        {
            Instance destination;auto q=std::make_shared<Outbox>();destination.join(1,"Transferred",0,0,q);
            const auto scope=*destination.effect_scope(1);const auto saved=destination.bind_morale(scope,continuity,false);
            require(saved==110,"actual party consumable effect retained across world transition");
            destination.set_vitals(1,{480,480,20,20},saved);
            require(destination.reviewed_health_capacity(scope,480,0,0)&&destination.energy_capacity(1,1,0,0)&&destination.resources(1)->max_health==528,"retained morale feeds new-world authoritative capacity");
            group.set_vitals(2,{480,480,20,20},100);
            require(!group.honeycomb(a,0,true,[&]{++calls;return true;})&&calls==before&&group.resources(2)->max_health==480,"superseded source world cannot consume another item or boost uncapped ally");
            require(destination.bind_morale(scope,continuity,true)==100,"outpost claim resets owned session value");
        }
        group.join(4,"Unmodeled Ally",0,0,std::make_shared<Outbox>());group.set_vitals(4,{400,480,10,20},100);
        require(group.party_action(1,158,4,0)&&group.party_action(4,154,1,0),"unmodeled member joined actual party");
        require(!group.honeycomb(a,0,true,[&]{++calls;return true;})&&calls==before&&group.resources(4)->max_health==480,"invalid party capacity authority rejects before inventory CAS and state mutation");
        group.leave(1);group.join(1,"Replacement",0,0,one);require(!group.honeycomb(a,0,true,[&]{++calls;return true;})&&calls==before,"stale actor generation cannot debit item");
    }

    {
        using namespace fw::gw::world;Instance group;
        group.join(1,"Elixir User",0,0,std::make_shared<Outbox>());group.join(2,"Elixir Ally",5000,0,std::make_shared<Outbox>());
        group.set_vitals(1,{400,480,10,20},100);group.set_vitals(2,{400,480,10,20},100);
        const auto a=*group.effect_scope(1),b=*group.effect_scope(2);
        require(group.reviewed_health_capacity(a,480,0,0)&&group.energy_capacity(1,1,0,0)&&group.reviewed_health_capacity(b,480,0,0)&&group.energy_capacity(2,1,0,0),"Elixir capacities have authoritative bases");
        require(group.party_action(1,158,2,0)&&group.party_action(2,154,1,0),"Elixir party established");unsigned calls=0;
        require(!group.party_morale(a,0,true,{99},[&]{++calls;return true;})&&calls==0,"unknown consumable amount rejects before debit");
        require(!group.party_morale(a,0,true,{10},[&]{++calls;return false;})&&group.resources(1)->max_health==480&&group.resources(2)->max_health==480,"Elixir failed CAS leaves every party member unchanged");
        require(!group.party_morale(a,0,true,{10,false,true},[]{return false;})&&!group.take_morale_recharge(a),"Seal failed owned CAS cannot publish recharge");
        require(group.party_morale(a,0,true,{10,false,true},[]{return true;})&&group.resources(1)->max_health==528&&group.resources(2)->max_health==480,"Seal user-only morale excludes party ally");
        auto stale=a;++stale.generation;require(!group.take_morale_recharge(stale)&&group.take_morale_recharge(a)&&!group.take_morale_recharge(a)&&!group.take_morale_recharge(b),"Seal native recharge delivery is owned generation and once-only");
        group.set_vitals(1,{400,480,10,20},100);
        require(group.party_morale(a,0,true,{10},[]{return true;})&&group.resources(1)->max_health==528&&group.resources(2)->max_health==528&&group.resources(1)->max_energy==22,"Elixir applies full ten-percent party effect once");
        require(!group.party_morale(a,0,true,{10},[]{return true;}),"capped Elixir cannot consume another unit");
    }
    {
        using namespace fw::gw;world::Instance instance;
        instance.join(1,"Dervish",0,0,std::make_shared<world::Outbox>());
        std::array<CharacterProfile::EquipmentAppearance,5> visual{};for(auto& piece:visual)piece.composite=1;
        auto armor=energy::creation_starter_armor(10,3,visual,u"Dervish");
        const auto actor=*instance.effect_scope(1);
        instance.set_vitals(1,{400,505,20,20},100);
        require(instance.reviewed_health_capacity(actor,480,energy::owned_basic_health(armor,10),0),"reviewed Dervish health installed");
        armor.items.at(2).bag=1;armor.items.at(2).slot=0;
        require(instance.reviewed_health_capacity(actor,480,energy::owned_basic_health(armor,10),0)&&instance.resources(1)->health==375&&instance.resources(1)->max_health==480,"chest removal preserves missing health");
        armor.items.at(2).bag=2;armor.items.at(2).slot=2;
        require(instance.reviewed_health_capacity(actor,480,energy::owned_basic_health(armor,10),0)&&instance.resources(1)->health==400&&instance.resources(1)->max_health==505,"chest restoration cannot refill missing health");
        instance.set_vitals(1,{0,505,0,20},100);
        require(instance.reviewed_health_capacity(actor,480,0,0)&&instance.resources(1)->health==0,"chest change cannot revive dead actor");
    }
    for(unsigned skill:{1052u,3184u})for(unsigned hex_count:{0u,1u,2u}){
        using namespace fw::gw::world;using namespace fw::gw::skills::effects;
        Instance spells;spells.join(1,"Mesmer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);spells.join(2,"Foe",10,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        spells.set_vitals(1,{480,480,40,40},100);spells.set_vitals(2,{480,480,40,40},100);const auto caster=*spells.effect_scope(1),foe=*spells.effect_scope(2);
        // Trusted registry fixtures establish active hex count; this does not
        // declare the fixture skills' own mechanics implemented.
        if(hex_count)require(spells.status_mutation(foe,43,77,0,100,20000,true),"first hex fixture");
        if(hex_count==2)require(spells.status_mutation(foe,44,78,0,100,20000,true),"second hex fixture");
        const auto plan=*wound_spell(skill,12);require(spells.wound_spell(caster,foe,plan,20,100),"Accumulated Pain actual damage and conditional wound");
        require(spells.resources(2)->health==(skill==1052?417u:450u)-(hex_count==2?96u:0u)&&spells.resources(2)->max_health==(hex_count==2?384u:480u),"Accumulated Pain two-hex threshold and PvE/PvP distinct damage");
        auto invalid=plan;++invalid.damage;const auto before=spells.resources(2)->health;
        require(!spells.wound_spell(caster,foe,invalid,20,100)&&spells.resources(2)->health==before,"forged wound plan rejected atomically");
        require(!spells.wound_spell({caster.instance,1,caster.generation+1},foe,plan,20,100),"stale caster rejected");
    }
    {
        using namespace fw::gw::world;using namespace fw::gw::skills::effects;
        Instance phantom;phantom.join(1,"Mesmer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);phantom.join(2,"Foe",10,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        phantom.set_vitals(1,{480,480,40,40},100);phantom.set_vitals(2,{480,480,40,40},100);const auto caster=*phantom.effect_scope(1),foe=*phantom.effect_scope(2);const auto plan=*wound_spell(44,12);
        require(phantom.wound_spell(caster,foe,plan,20,100),"Phantom real hex degeneration");phantom.tick(1100);
        require(!phantom.wound_spell(caster,foe,plan,20,UINT64_MAX-plan.hex_duration),"Phantom reserves full hex plus end-condition lifetime before commit");
        require(phantom.resources(2)->health==474&&phantom.resources(2)->max_health==480,"Phantom rank12 three degeneration pips");
        require(phantom.wound_spell(caster,foe,plan,20,1100)&&phantom.resources(2)->max_health==480,"renewal does not trigger end condition");
        phantom.tick(11100);require(phantom.resources(2)->health==318&&phantom.resources(2)->max_health==384,"Phantom expires at exact partition and applies seventeen-second wound");
        phantom.tick(28100);require(phantom.resources(2)->health==414&&phantom.resources(2)->max_health==480,"end condition expires in delayed tick with exact latent HP restored");
        require(phantom.wound_spell(caster,foe,plan,20,28100),"Phantom removal fixture");
        const auto removal=*fw::gw::skills::effects::removal(301,0,0);
        require(phantom.remove_effects(foe,2,removal,28100)&&phantom.resources(2)->max_health==384,"actual self hex removal triggers Phantom end condition early");
        require(phantom.kill(foe,28100)&&phantom.resources(2)->max_health==480,"corpse cleanup cannot spawn new end condition");
    }
    {
        using namespace fw::gw::world;using fw::gw::combat::conditions::Kind;
        Instance wound;auto box=std::make_shared<Outbox>();
        wound.join(1,"Wound",0,0,box,0,{}, {},100,1);
        wound.set_vitals(1,{50,480,20,20},100);const auto scope=*wound.effect_scope(1);
        require(wound.apply_condition(scope,scope,Kind::deep_wound,100,1000),"Deep Wound applied at low health without death");
        require(wound.resources(1)->health==1&&wound.resources(1)->max_health==384,"Deep Wound effective capacity and living floor");
        require(wound.health_capacity(scope,480,100)&&wound.resources(1)->health==1&&wound.resources(1)->max_health==384,"gear refresh cannot remove wound capacity");
        require(wound.health_capacity(scope,500,100)&&wound.resources(1)->max_health==400,"gear baseline capacity recomputes wound reduction");
        wound.tick(1100);require(wound.resources(1)->health==70&&wound.resources(1)->max_health==500,"expiry restores exact latent health, no free healing");
        require(wound.apply_condition(scope,scope,Kind::deep_wound,1100,1000),"second low-health wound");
        require(wound.heal_target(1,1,10,0,true,1100)&&wound.resources(1)->health==0&&wound.resources(1)->max_health==500,"insufficient healing settles lethal deficit and corpse clears capacity");
        require(wound.resurrect(scope,1100)&&wound.resources(1)->health==500,"resurrection uses restored baseline capacity");
        wound.set_vitals(1,{300,500,20,20},100);
        require(wound.apply_condition(scope,scope,Kind::deep_wound,1100,1000),"nonlethal wound");
        require(wound.heal_target(1,1,100,0,true,1100)&&wound.resources(1)->health==280,"direct healing reduced twenty percent");
        wound.set_vitals(1,{380,500,20,20},100);
        require(wound.resources(1)->health==280&&wound.resources(1)->max_health==400,"admin resource reset preserves active wound capacity and healing policy");
        const auto cleanse=*fw::gw::skills::effects::condition_removal(278,0,0);
        require(wound.remove_conditions(scope,1,cleanse,1100)&&wound.resources(1)->health==380&&wound.resources(1)->max_health==500,"cleansing restores wound capacity before healing");
        require(!wound.apply_condition({scope.instance,scope.actor,scope.generation+1},scope,Kind::deep_wound,1100,1000),"stale generation cannot wound replacement");
        wound.set_vitals(1,{50,480,20,20},100);
        require(wound.apply_condition(scope,scope,Kind::deep_wound,1100,1000),"regeneration wound setup");
        const auto breeze=*fw::gw::skills::regeneration::reviewed(288,12);
        require(wound.apply_health_regeneration(scope,1,breeze,1100,1100+breeze.duration,0),"wounded actor receives genuine regeneration");
        wound.tick(2100);require(wound.resources(1)->health==66&&wound.resources(1)->max_health==480,"regeneration unaffected, does not settle lethal deficit before wound expires");
        auto foe_box=std::make_shared<Outbox>();wound.join(2,"Victim",10,0,foe_box,0,{}, {},2100,2);
        wound.set_vitals(2,{50,480,20,20},100);const auto foe=*wound.effect_scope(2);
        require(wound.apply_condition(scope,foe,Kind::deep_wound,2100,1000),"damage wound setup");
        require(wound.shadow_damage(scope,foe,*fw::gw::skills::effects::shadow_damage(133,0),20,2100),"actual shadow damage settles wound debt");
        require(wound.resources(2)->health==0&&wound.resources(2)->max_health==480,"lethal pending wound damage clears corpse capacity");
    }
    {
        using namespace fw::gw::world;Instance colors;auto own=std::make_shared<Outbox>(),peer=std::make_shared<Outbox>();
        const auto original=fw::gw::equipment::encode_2022(fw::gw::equipment::Update::color,17,0,12);
        colors.join(17,"Dye Owner",0,0,own,0,{original});colors.join(18,"Observer",0,0,peer);
        const auto scope=*colors.effect_scope(17);own->drain();peer->drain();
        require(colors.equipment_color(scope,0,0x23),"owned equipped appearance color update");
        const auto expected=fw::gw::equipment::encode_2022(fw::gw::equipment::Update::color,17,0,0x23);
        require(own->drain()==std::vector<Packet>{expected}&&peer->drain()==std::vector<Packet>{expected},"native dye appearance broadcast owner and peers");
        auto late=std::make_shared<Outbox>();const auto snapshots=colors.join(19,"Late Observer",0,0,late);
        require(std::any_of(snapshots.begin(),snapshots.end(),[&](const auto& p){return p.id==17&&std::find(p.presentation.begin(),p.presentation.end(),expected)!=p.presentation.end();}),"late observer gets current dye cache");
        require(!colors.equipment_color(scope,4,3),"missing equipped appearance never invented");
        colors.leave(17);colors.join(17,"Replacement",0,0,std::make_shared<Outbox>(),0,{original});
        require(!colors.equipment_color(scope,0,2),"stale actor generation cannot recolor replacement");
    }

    {
        using namespace fw::gw::world;Instance rename_world;auto owner=std::make_shared<Outbox>(),peer=std::make_shared<Outbox>();
        rename_world.join(17,"Old Name",0,0,owner,0x12345678);rename_world.join(18,"Observer",0,0,peer);
        const auto scope=*rename_world.effect_scope(17);const std::string account(32,'a'),character(32,'b');
        require(rename_world.bind_character_identity(scope,account,character),"bind durable actor identity");
        require(!rename_world.bind_character_identity(scope,std::string(32,'c'),character),"identity cannot be rebound to another account");
        owner->drain();peer->drain();unsigned commits=0;
        const auto commit=[&](const auto& identity,const auto& next){++commits;require(identity.account==account&&identity.character==character&&identity.name=="Old Name"&&next=="New Name","rename commit holds exact durable identity");return true;};
        require(!rename_world.rename_character(scope,"Stale Name","New Name",commit)&&commits==0,"stale expected name cannot reach durable commit");
        require(!rename_world.rename_character(scope,"Old Name","New Name",[](const auto&,const auto&){return false;}),"failed durable commit rejects live rename");
        require(owner->drain().empty()&&peer->drain().empty()&&rename_world.character_identity(scope)->name=="Old Name","failed rename sends no native update");
        require(rename_world.rename_character(scope,"Old Name","New Name",commit)&&commits==1,"owned live rename commits once");
        const auto updates=owner->drain(),peer_updates=peer->drain();require(!updates.empty()&&!peer_updates.empty()&&updates[0]==peer_updates[0]&&updates[0][0]==88&&packet_u32(updates[0],10)==0x12345678&&updates.size()==6,"native rename preserves appearance and broadcasts without agent spawn");
        require(packet_count(updates,465)==1&&packet_count(updates,466)==1&&packet_count(updates,458)==1&&packet_count(peer_updates,465)==0,"rename republishes only affected party roster");
        require(packet_count(updates,380)==1&&packet_count(peer_updates,380)==0,"native owner name changes only renamed character context");
        require(rename_world.named_effect_scope(u"New Name")==scope&&!rename_world.named_effect_scope(u"Old Name"),"world targeting follows committed name");
        rename_world.leave(17);rename_world.join(17,"Replacement",0,0,owner);
        require(!rename_world.rename_character(scope,"New Name","Stale Rename",commit)&&commits==1,"recycled actor lifetime cannot rename replacement");
    }
    {
        const auto mesh=fixture_mesh();
        const auto file=std::filesystem::temp_directory_path()/"gw-spawn-test.txt";
        {std::ofstream out(file);out<<"# explicit entry\n248 5 5\n";}
        const auto first=fw::gw::world::map_spawn(file,248,*mesh);
        require(first&&first->x==5&&first->y==5,"configured entry replaces origin");
        require(!fw::gw::world::map_spawn(file,999,*mesh),"missing map has no origin fallback");
        {std::ofstream out(file);out<<"248 500 500\n";}
        fails_with([&]{fw::gw::world::map_spawn(file,248,*mesh);},"map spawn outside ground navigation");
        {std::ofstream out(file);out<<"248 5 5\n248 15 5\n";}
        fails_with([&]{fw::gw::world::map_spawn(file,248,*mesh);},"duplicate map spawn");
        for(const char* row:{"248 nan 5\n","248 5 inf\n","248 5\n","248 5 5 unexpected\n"}){
            {std::ofstream out(file);out<<row;}
            fails_with([&]{fw::gw::world::map_spawn(file,248,*mesh);},"invalid map spawn row");
        }
        std::filesystem::remove(file);
        fails_with([&]{fw::gw::world::map_spawn(file,248,*mesh);},"spawn configuration unavailable");
    }

    {
        using namespace fw::gw::world;
        std::vector<std::uint8_t> rotation{0x3e,0x80,0,0,0x80,0xff,0,0,0x80,0x3f};
        for(unsigned n=0;n<rotation.size();++n)
            require(outgoing_frame_2022(std::span(rotation).first(n)).state==FrameState::incomplete,"fragmented rotation frame");
        rotation.push_back(10);rotation.push_back(0x80);
        require(outgoing_frame_2022(rotation).size==10,"coalesced next packet is retained");
        require(outgoing_frame_2022(std::span(rotation).subspan(10)).size==2,"heartbeat descriptor");
        require(std::size(outgoing_2022)==189,"complete extracted outgoing table");
    }

    {
        fw::gw::world::Instance turning;
        auto queue=std::make_shared<fw::gw::world::Outbox>();
        turning.join(1,"Turning",0,0,queue);
        turning.rotate(1,-std::numeric_limits<float>::infinity(),1,1000);
        require(close(turning.heading(1,1500),-1.0471975f),"negative infinity starts left turn");
        turning.rotate(1,0.5f,1,1500);
        require(close(turning.heading(1,2000),0.5f),"finite angle stops continuous turn");
        fails_with([&]{turning.rotate(1,std::numeric_limits<float>::quiet_NaN(),1,2000);},"invalid rotation");
    }
    {
        fw::gw::world::Instance turning;auto queue=std::make_shared<fw::gw::world::Outbox>();
        turning.join(1,"Walking turn",0,0,queue);
        turning.steer(1,1,0,1000);turning.rotate(1,INFINITY,1,1000);
        turning.tick(1500);
        const float rate=2.094395f;
        auto point=turning.position(1,1500);
        require(std::abs(point.first-288.f*std::sin(rate*.5f)/rate)<.03f&&
                std::abs(point.second-288.f*(1-std::cos(rate*.5f))/rate)<.03f,
                "walking turn follows arc between sparse movement direction refreshes");
        turning.rotate(1,rate*.5f,1,1500);turning.tick(2000);
        point=turning.position(1,2000);
        require(std::abs(point.first-(288.f*std::sin(rate*.5f)/rate+144.f*std::cos(rate*.5f)))<.03f&&
                std::abs(point.second-(288.f*(1-std::cos(rate*.5f))/rate+144.f*std::sin(rate*.5f)))<.03f,
                "ending turn preserves the final held vector rather than reverting to pre-turn movement");
        turning.rotate(1,INFINITY,1,2000);turning.steer(1,0,0,2100);
        const auto stopped=turning.position(1,2100);
        turning.steer(1,1,0,2200);turning.tick(2700);
        point=turning.position(1,2700);
        require(std::abs(point.first-stopped.first-144.f)<.03f&&std::abs(point.second-stopped.second)<.03f,
                "releasing held movement clears its turn integration before a new direction starts");
    }

    {
        using namespace fw::gw::world;
        using S=Instance::CheckpointStatus;using R=Instance::SteeringReport;
        Instance reports;auto box=std::make_shared<Outbox>();reports.join(1,"Reported turn",0,0,box);
        require(reports.steer(1,1,0,1000,1,R{9000,9000,0}).status==S::baseline&&
            close(reports.position(1,1000).first,0),"initial report cannot select an arbitrary position");
        reports.tick(1500);
        const auto turn=reports.steer(1,0,1,1500,1,R{95,40,0});
        require(turn.status==S::accepted&&turn.correction>40&&close(reports.position(1,1500).first,95),
            "sparse held-turn report reconciles an elapsed-speed-valid checkpoint");
        require(reports.steer(1,0,1,1500,1,R{900,40,0}).status==S::over_budget,
            "same-time reports cannot replenish movement credit");
        require(reports.steer(1,0,1,1500,1,R{NAN,40,0}).status==S::invalid,
            "nonfinite reported coordinates rejected without mutation");
        reports.tick(2000);
        require(reports.steer(1,0,0,2000,1,R{95,150,0}).status==S::accepted&&
            close(reports.position(1,2000).second,150),"release validates its final checkpoint before stopping");
        const auto stopped=reports.position(1,2000);
        reports.steer(1,1,0,2000,1,R{stopped.first,stopped.second,0});
        const auto first=reports.steer(1,0,0,2000,1,R{stopped.first+22,stopped.second,0});
        require(first.status==S::accepted,"remaining once-funded prediction lead may be consumed");
        reports.steer(1,1,0,2000,1,R{0,0,0});
        require(reports.steer(1,0,0,2000,1,R{stopped.first+44,stopped.second,0}).status==S::over_budget,
            "release/restart cannot mint repeated prediction lead");
    }
    {
        using namespace fw::gw::world;
        using S=Instance::CheckpointStatus;using R=Instance::SteeringReport;
        Instance reports;auto box=std::make_shared<Outbox>();reports.join(1,"Sparse free walk",0,0,box);
        reports.steer(1,1,0,1000,1,R{0,0,0});reports.tick(3328);
        require(reports.steer(1,1,0,3328,1,R{512.16f,0,0}).status==S::accepted,
            "retail 2328ms free-space report interval fits elapsed movement budget");
        reports.tick(10000);
        require(reports.steer(1,1,0,10000,1,R{512.16f+1000,0,0}).status==S::over_budget,
            "silent held input cannot bank unbounded movement credit");
    }
    {
        using namespace fw::gw::world;
        using S=Instance::CheckpointStatus;using R=Instance::SteeringReport;
        Instance reports;reports.configure_navigation(party_healing_mesh());auto box=std::make_shared<Outbox>();
        reports.join(1,"Report collisions",195,100,box);
        reports.steer(1,0,1,1000,1,R{195,100,0});reports.tick(1500);
        const auto before=reports.position(1,1500);
        require(reports.steer(1,0,1,1500,1,R{225,100,0}).status==S::unreachable,
            "a nearby checkpoint across a disconnected wall is rejected");
        require(reports.steer(1,0,1,1500,1,R{195,100,1}).status==S::unreachable,
            "overlapping floors do not permit a report to change layer");
        require(reports.steer(1,0,1,1500,1,R{195,100,99}).status==S::invalid,
            "out of range report plane is rejected before navigation lookup");
        require(close(reports.position(1,1500).first,before.first)&&close(reports.position(1,1500).second,before.second),
            "rejected checkpoints preserve authoritative motion");
    }

    require(fw::gw::items::equipment_bag_2022(1,7)==
            std::vector<std::uint8_t>{0x3e,1,1,0,2,21,7,0,9,0,0,0,0},"2022 equipped bag reference");
    require(fw::gw::items::equip_2022(1,17,7,4)==
            std::vector<std::uint8_t>{0x4a,1,1,0,17,0,0,0,7,0,4},"2022 item placement reference");
    bool placement_rejected=false;
    try{fw::gw::items::equip_2022(1,17,7,9);}catch(const std::invalid_argument&){placement_rejected=true;}
    require(placement_rejected,"equipment slot bound");
    using fw::gw::equipment::Update;
    // Original serializer and action producer: equipment_assignment_2022.py.
    require(fw::gw::equipment::assignment_2022(42,2,123)==
            std::vector<std::uint8_t>{110,0,42,0,0,0,2,0,0,0,123,0,0,0},
            "2022 owned equipment assignment reference");
    require(fw::gw::equipment::assignment_2022(42,4,123)==
            std::vector<std::uint8_t>{110,0,42,0,0,0,6,0,0,0,123,0,0,0},
            "2022 physical head slot maps to wire slot six");
    bool assignment_rejected=false;
    try{fw::gw::equipment::assignment_2022(42,9,123);}
    catch(const std::invalid_argument&){assignment_rejected=true;}
    require(assignment_rejected,"owned equipment slot must fit client array");
    require(fw::gw::equipment::appearance_2022(1,0x601234)==
            std::vector<std::uint8_t>{158,0,0,0,0,0,1,0,0,0,0x34,0x12,0x60,0},
            "2022 packed composite appearance reference");
    // Actual pinned serializer output from gw_2022_player_view_wire_probe.py,
    // with only the transport framing bit cleared.
    require(fw::gw::equipment::encode_2022(Update::composite,1,0,2116)==
            std::vector<std::uint8_t>{159,0,14,0,0,0,1,0,0,0,0,0,0,0,0x44,8,0,0},
            "2022 composite update reference");
    require(fw::gw::equipment::encode_2022(Update::color,1,0,12)==
            std::vector<std::uint8_t>{159,0,15,0,0,0,1,0,0,0,0,0,0,0,12,0,0,0},
            "2022 color update reference");
    bool equipment_rejected=false;
    try{fw::gw::equipment::encode_2022(Update::color,1,5,12);}
    catch(const std::invalid_argument&){equipment_rejected=true;}
    require(equipment_rejected,"equipment index must fit client array");
    fw::gw::CharacterProfile profile;profile.unknown1=6;profile.extension_count=42;
    auto profile_bytes=profile.encode();profile_bytes[38]=1;
    profile_bytes[43]=0x44;profile_bytes[44]=8;profile_bytes[45]=12;profile_bytes[47]=6;
    profile=fw::gw::CharacterProfile::decode(profile_bytes);
    const auto equipment=profile.equipment_2022();
    require(equipment.size()==1&&equipment[0].composite==2116&&equipment[0].color==12&&
            equipment[0].packed_composite()==0x60844,"summary equipment decode and packing");
    profile.extension_count=41;equipment_rejected=false;
    try{profile.equipment_2022();}catch(const std::invalid_argument&){equipment_rejected=true;}
    require(equipment_rejected,"equipment summary shape validation");
    // Reference bytes come from executing build-37130's real serializer
    // (scripts/gw_2022_item_wire_probe.py), with its framing bit cleared.
    fw::gw::items::Definition item;
    item.id=17;item.file=0x123456;item.type=4;item.dye=8;item.model=100;
    const std::vector<std::uint8_t> item_reference={
        0x60,0x01,0x11,0,0,0,0x56,0x34,0x12,0,4,0,8,0,0,0,0,
        0,0,0,0,0,0,0,0,100,0,0,0,1,0,0,0,0,0,0};
    require(fw::gw::items::definition_2022(item)==item_reference,"2022 item serializer reference");
    item.encoded_name=u"Test";item.modifiers={0x12345678};
    auto encoded_item=fw::gw::items::definition_2022(item);
    require(encoded_item.size()==48&&encoded_item[43]==1&&encoded_item[44]==0x78,
            "2022 item modifier count is BYTE");
    item.modifiers.resize(65);
    bool item_rejected=false;
    try{fw::gw::items::definition_2022(item);}catch(const std::invalid_argument&){item_rejected=true;}
    require(item_rejected,"2022 item modifier bound");
    require(fw::gw::items::equipment_change_2022(17,0)==std::vector<std::uint8_t>{0x5c,1,17,0,0,0,0,0,0,0},
            "2022 item replacement field order");
    // Golden frame from the pinned retail serializer, not the server encoder.
    const auto record=fw::gw::world::player_identity_packets(7,"Only Flares",0,0).at(1);
    const std::string golden="59000700000007000000000000000000000000000000000b004f006e006c007900200046006c006100720065007300";
    std::string hex;const char* digits="0123456789abcdef";
    for(auto byte:record){hex.push_back(digits[byte>>4]);hex.push_back(digits[byte&15]);}
    require(hex==golden,"current-client player record serializer golden");
    const auto unicode_record=fw::gw::world::player_identity_packets(7,"\xc3\x89\xe6\x9e\x97\xf0\x9f\x98\x80",0,0).at(1);
    require(std::vector<std::uint8_t>(unicode_record.begin()+23,unicode_record.end())==
        std::vector<std::uint8_t>{4,0,0xc9,0,0x97,0x67,0x3d,0xd8,0,0xde},
        "world name length and characters use UTF-16 units, not UTF-8 bytes");
    const auto agent=fw::gw::world::player_identity_packets(7,"Only Flares",1.25f,-2.5f).at(0);
    const std::string agent_golden="2000070000000700003001000000a03f000020c000000000803f0000000000000090430000803f000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    hex.clear();
    for(auto byte:agent){hex.push_back(digits[byte>>4]);hex.push_back(digits[byte&15]);}
    require(hex==agent_golden,"current-client agent spawn serializer golden");
    // Retail AgentSpawn -> agent +0x5c/+0x60 -> destination duration division.
    const auto wire_float=[&](std::size_t offset){
        std::uint32_t bits=0;for(unsigned i=0;i<4;++i)bits|=std::uint32_t(agent[offset+i])<<(8*i);
        return std::bit_cast<float>(bits);
    };
    const float travel_ms=288.f*1000.f/(wire_float(31)*wire_float(35));
    require(std::isfinite(travel_ms)&&close(travel_ms,1000.f),"spawn supplies finite one-second movement duration");
    using namespace fw::gw::world;
    Instance keyboard;
    auto keyboard_outbox=std::make_shared<Outbox>();
    keyboard.join(1,"Keyboard",0,0,keyboard_outbox);
    auto keyboard_observer=std::make_shared<Outbox>();
    keyboard.join(2,"Observer",0,0,keyboard_observer);
    keyboard_outbox->drain();
    keyboard.steer(1,10000,0,1000);
    auto start=keyboard_outbox->drain();
    require(start.size()==2&&start[0][0]==43&&start[1][0]==41,"keyboard starts synchronized speed and forward destination");
    const auto observer_start=keyboard_observer->drain();
    require(packet_count(observer_start,41)==1&&packet_float(observer_start.back(),6)>700,
        "remote keyboard animation starts with a continuing forward goal");
    keyboard.tick(1100);
    require(close(keyboard.position(1,1100).first,28.8f),"keyboard vector magnitude cannot choose speed");
    require(keyboard_outbox->drain().empty(),"owner prediction is not overridden by echoed keyboard destinations");
    const auto animated=keyboard_observer->drain();
    require(animated.empty(),"free-space ticks retain the remote native forward goal instead of replacing it with completed points");
    fails_with([&]{keyboard.move(1,std::numeric_limits<float>::quiet_NaN(),0,1200);},"movement bounds");
    require(keyboard_outbox->drain().empty(),"invalid move does not publish a partial change");
    keyboard.tick(1500);
    require(close(keyboard.position(1,1500).first,144),"held keyboard input advances continuously");
    keyboard.steer(1,0,0,1500);
    const auto remote_finish=keyboard_observer->drain();
    require(packet_count(remote_finish,44)==0&&packet_count(remote_finish,40)==1&&packet_count(remote_finish,41)==1&&
        remote_finish.front()[0]==40&&close(packet_float(remote_finish.back(),6),144),
        "remote release clears the old native path before moving to the validated endpoint, without teleporting");
    keyboard.tick(2000);
    require(close(keyboard.position(1,2000).first,144),"keyboard release stops movement");
    // Direction changes faster than the 100 ms broadcast interval must not
    // discard movement, and release must include the final partial interval.
    Instance rapid;
    auto rapid_outbox=std::make_shared<Outbox>();
    rapid.join(1,"Rapid",0,0,rapid_outbox);
    rapid.steer(1,1,0,1000);
    rapid.steer(1,0,1,1050);
    rapid.steer(1,0,0,1075);
    require(close(rapid.position(1,1075).first,14.4f)&&close(rapid.position(1,1075).second,7.2f),
            "direction replacement and release preserve elapsed input time");
    for(unsigned direction=1;direction<=8;++direction){
        Instance gait;auto out=std::make_shared<Outbox>();gait.join(1,"Gait",0,0,out);
        gait.steer(1,-10,0,1000,direction);gait.tick(2000);
        require(close(gait.position(1,2000).first,-288.f*(direction<=3?1.f:direction<=6?.66f:.75f)),
                "keyboard gait uses client forward/backward/strafe speed without changing vector orientation");
        require(out->drain().size()==2,"all eight gaits initialize movement without per-tick owner corrections");
    }
    Instance collision;
    auto collision_outbox=std::make_shared<Outbox>();
    collision.configure_navigation(fixture_mesh(),0);
    collision.join(1,"Collision",5,5,collision_outbox);
    collision.steer(1,-1,0,1000);collision.tick(1100);
    const auto wall_contact_y=collision.position(1,1100).second;
    require(std::abs(collision.position(1,1100).first)<.011f,"keyboard reaches its wall without crossing the mesh boundary");
    const auto collision_updates=collision_outbox->drain();
    require(collision_updates.size()>=2&&collision_updates[collision_updates.size()-2][0]==40&&collision_updates.back()[0]==44,
            "blocked contact clears queued local prediction before authoritative position correction");
    collision.tick(1200);
    require(collision_outbox->drain().empty(),"held input at a wall does not flood duplicate stop corrections");
    collision.steer(1,-100,0,1200);const auto blocked_refresh=collision_outbox->drain();
    require(blocked_refresh.empty(),"unchanged blocked input does not repeatedly restart the actor animation");
    for(std::uint64_t now=1201;now<1300;now+=10){
        collision.steer(1,-100,now%20?.5f:-.5f,now);
        require(collision_outbox->drain().empty(),"near-head-on direction jitter does not emit another stop/correction");
        require(std::abs(collision.position(1,now).second-wall_contact_y)<.001f,
                "near-head-on input remains stationary instead of alternately sliding sideways");
    }
    collision.tick(1300);
    require(collision_outbox->drain().empty(),"periodic blocked ticks do not repeat the input-event reset");
    collision.steer(1,1,0,1300);collision.tick(1400);
    require(collision.position(1,1400).first>20,"reverse input leaves a blocked wall immediately");
    struct SlideInput { float x,y;unsigned gait; };
    for(const auto input:std::array<SlideInput,4>{{{1,1,1},{1,10,1},{1,10,4},{1,10,7}}}){
        Instance sliding;auto owner=std::make_shared<Outbox>(),observer=std::make_shared<Outbox>();
        const auto mesh=fixture_mesh(1000);
        const float gait=input.gait<=3?1.f:input.gait<=6?.66f:.75f;
        // Original 79c8d0 arithmetic is executed independently by
        // tools/protocol/steering_rate_2022.py, including floor and upper clamp.
        const float tangent_ratio=std::clamp(input.x/std::hypot(input.x,input.y)+.01f,.33f,1.f);
        const float expected_speed=288*gait*tangent_ratio;
        sliding.configure_navigation(mesh);sliding.join(1,"Sliding",5,5,owner);
        sliding.join(2,"Observer",5,5,observer);owner->drain();observer->drain();
        // The pinned agent engine treats 41 as a finite goal, 43 as speed,
        // and a moving 44 as a position correction which clears that goal.
        // This bounded receiver detects a silently exhausted wall destination;
        // it does not claim to reproduce the client's rendered prediction.
        float client_x=5,client_y=5,target_x=5,target_y=5,client_speed=288;bool client_moving=false;
        auto receive=[&](const std::vector<Packet>& packets){
            for(const auto& packet:packets){
                if(packet[0]==43)client_speed=288*packet_float(packet,6);
                if(packet[0]==41){target_x=packet_float(packet,6);target_y=packet_float(packet,10);client_moving=true;}
                if(packet[0]==44){client_x=packet_float(packet,6);client_y=packet_float(packet,10);client_moving=false;}
                if(packet[0]==40)client_moving=false;
            }
        };
        auto elapse=[&](float seconds){
            const float distance=std::hypot(target_x-client_x,target_y-client_y);
            if(!client_moving||distance<.0001f)return;
            const float fraction=std::min(1.f,seconds*client_speed/distance);
            client_x+=(target_x-client_x)*fraction;client_y+=(target_y-client_y)*fraction;
            if(fraction==1)client_moving=false;
        };
        sliding.steer(1,input.x,input.y,1000,input.gait);owner->drain();observer->drain();
        sliding.tick(1100);const auto first=sliding.position(1,1100);
        require(first.first>5&&first.second<=10.011f&&first.second>=9.99f,
                "diagonal keyboard input slides instead of freezing at a small edge");
        const auto contact=owner->drain();
        require(contact.size()==4&&contact[0][0]==40&&contact[1][0]==44&&contact[2][0]==43&&contact[3][0]==41,
                "first contact clears the queued local path and control gate before tangent speed and destination");
        require(close(packet_float(contact[2],6),gait*tangent_ratio),
                "sliding client speed matches server ratio floor multiplied by the selected gait");
        if(!(packet_float(contact[3],6)>first.first+expected_speed*.99f&&close(packet_float(contact[3],10),first.second)))
            std::cerr<<"slide input="<<input.x<<','<<input.y<<" gait="<<input.gait<<" first="<<first.first<<','<<first.second
                <<" speed_factor="<<packet_float(contact[2],6)<<" goal="<<packet_float(contact[3],6)<<','<<packet_float(contact[3],10)
                <<" expected_speed="<<expected_speed<<'\n';
        require(packet_float(contact[3],6)>first.first+expected_speed*.99f&&close(packet_float(contact[3],10),first.second),
                "sliding destination is ahead on the wall rather than the completed tick position");
        require(fw::gw::navigation::LayeredMesh(*mesh).trace({0,0},{first.first,first.second},
                    {packet_float(contact[3],6),packet_float(contact[3],10)}).has_value(),
                "owner sliding destination stays on valid navigation");
        receive(contact);elapse(.1f);
        sliding.tick(1200);const auto second=sliding.position(1,1200);
        const auto continuing=owner->drain();
        require(close(second.first-first.first,expected_speed*.1f)&&second.second<=10.011f&&
                    continuing.size()==2&&continuing[0][0]==43&&continuing[1][0]==41,
                "held input renews a future tangent destination without another position reset");
        require(close(client_x,second.first)&&close(client_y,second.second),"finite client target tracks continued server sliding");
        receive(continuing);
        const auto updates=observer->drain();
        require(updates==std::vector<Packet>({contact[2],contact[3],continuing[0],continuing[1]}),
                "observer receives projected speed and forward goals without owner contact resets");
        sliding.steer(1,input.x*100,input.y*100,1200,input.gait);const auto refresh=owner->drain();
        require(refresh.size()==2&&refresh[0][0]==43&&refresh[1][0]==41&&
                    close(packet_float(refresh[0],6),gait*tangent_ratio),
                "unchanged held refresh keeps the tangent without repeating stop or position correction");
        require(observer->drain()==refresh,"owner and observers retain continuous tangent movement on held refresh");
        receive(refresh);
        for(std::uint64_t time=1300;time<=8000;time+=100){
            elapse(.1f);sliding.tick(time);const auto position=sliding.position(1,time);
            require(std::hypot(client_x-position.first,client_y-position.second)<.02f,
                    "seven-second wall hold cannot silently outrun the client destination");
            const auto packets=owner->drain();
            require(packets.size()==2&&packets[0][0]==43&&packets[1][0]==41,
                    "continuing wall contact has no repeated teleport or stop");
            receive(packets);observer->drain();
        }
        elapse(.05f);sliding.steer(1,0,0,8050,input.gait);const auto released=sliding.position(1,8050);
        require(std::abs(released.first-(first.first+expected_speed*6.95f))<.05f,"release consumes final partial sliding interval");
        require(std::hypot(client_x-released.first,client_y-released.second)<.02f,
                "release correction stays bounded after a long wall slide");
        const auto stopped=owner->drain();require(!stopped.empty()&&stopped.back()[0]==40,"release publishes stop after sliding");
        sliding.tick(9000);require(close(sliding.position(1,9000).first,released.first),"released movement never resumes along wall");
    }
    {
        Instance jitter;auto owner=std::make_shared<Outbox>();
        jitter.configure_navigation(fixture_mesh(1000));jitter.join(1,"Slide Jitter",5,9,owner);owner->drain();
        jitter.steer(1,1,1,1000);owner->drain();jitter.tick(1100);owner->drain();
        auto previous=jitter.position(1,1100);
        for(std::uint64_t now=1200;now<=4100;now+=100){
            // Recorded keyboard vectors fluctuate even while a direction is held.
            // Exercise refreshes at contact rather than only an identical vector.
            jitter.steer(1,1,(now/100)%2?1.02f:.98f,now);
            const auto packets=owner->drain();
            for(const auto& packet:packets)
                require(packet[0]!=40&&packet[0]!=44,
                    "sub-two-degree held-input jitter cannot restart or teleport an ongoing wall slide");
            const auto position=jitter.position(1,now);
            require(position.first>previous.first&&position.first-previous.first<23&&
                    std::abs(position.second-previous.second)<.02f,
                    "jittering wall slide advances continuously without sideways jumps");
            previous=position;
        }
    }
    {
        Instance turning;auto owner=std::make_shared<Outbox>();
        turning.configure_navigation(fixture_mesh(1000));turning.join(1,"Slide Turn",5,9,owner);owner->drain();
        turning.steer(1,1,1,1000);owner->drain();turning.tick(1100);owner->drain();
        for(std::uint64_t now=1200;now<=2100;now+=100){
            turning.steer(1,1,(now/100)%2?1.4f:.6f,now);owner->drain();
            turning.tick(now+50);
            for(const auto& packet:owner->drain())
                require(packet[0]!=40&&packet[0]!=44,
                    "turning along the same wall cannot reacquire contact with a stop and teleport");
        }
    }
    {
        Instance short_goal;auto owner=std::make_shared<Outbox>();
        short_goal.configure_navigation(fixture_mesh(1000));short_goal.join(1,"Short Goal",2999.5f,9.99f,owner);owner->drain();
        short_goal.steer(1,1,1,1000);owner->drain();short_goal.tick(1001);
        const auto contact=owner->drain();
        require(contact.size()==2&&contact.front()[0]==40&&contact.back()[0]==44,
            "sub-unit legal corner horizon does not advertise a finite segment below the retail engine threshold");
    }
    {
        Instance tip;auto owner=std::make_shared<Outbox>();
        tip.configure_navigation(wall_tip_mesh());tip.join(1,"Wall Tip",95,9,owner);owner->drain();
        tip.steer(1,1,1,1000);owner->drain();tip.tick(1100);
        const auto contact=owner->drain();
        require(contact.size()==4&&contact[0][0]==40&&contact[1][0]==44,
                "wall-tip fixture first clears the local predicted path on contact");
        require(tip.position(1,1100).first>100,"sliding reaches the open end of the wall");
        for(std::uint64_t now=1200;now<=5000;now+=100){
            tip.tick(now);const auto packets=owner->drain();
            require(packets.size()==2&&packets[0][0]==43&&packets[1][0]==41,
                    "after the wall tip, cleared client path keeps receiving forward speed and finite goals");
            require(close(packet_float(packets[0],6),1)&&
                    packet_float(packets[1],6)>tip.position(1,now).first+1&&
                    packet_float(packets[1],10)>tip.position(1,now).second+1,
                    "leaving wall contact restores diagonal running instead of an exhausted tangent goal");
        }
        require(tip.position(1,5000).second>600,"held input continues past the wall without another input event");
        tip.steer(1,100,100,5000);owner->drain();tip.tick(5100);
        require(owner->drain().empty(),"fresh free-space input hands prediction back to the retail client");
        tip.steer(1,0,0,5100);owner->drain();const auto stopped=tip.position(1,5100);tip.tick(5200);
        require(tip.position(1,5200)==stopped,"release after the wall tip cannot resume server-guided movement");
    }
    Instance world;
    {
        Instance health_world;
        auto owner=std::make_shared<Outbox>(),observer=std::make_shared<Outbox>();
        health_world.join(1,"Owner",0,0,owner);
        health_world.set_vitals(1,{25,100,7,20},100);
        health_world.set_vitals(1,{50,100,3,20},80);
        auto snapshot=health_world.join(2,"Observer",0,0,observer);
        require(snapshot.size()==1&&snapshot[0].presentation==fw::gw::vitals::public_health_2022(1,{50,100,3,20}),
                "late observer receives current health without private energy or stale history");
        owner->drain();observer->drain();
        bool invalid=false;
        try{health_world.set_vitals(1,{101,100,3,20},80);}catch(const std::invalid_argument&){invalid=true;}
        require(invalid&&observer->drain().empty(),"invalid health is not published");
        auto third=std::make_shared<Outbox>();
        snapshot=health_world.join(3,"Later",0,0,third);
        require(snapshot[0].presentation==fw::gw::vitals::public_health_2022(1,{50,100,3,20}),
                "invalid update preserves latest valid health snapshot");
    }
    Instance presented;
    auto presented_first=std::make_shared<Outbox>();
    auto presented_second=std::make_shared<Outbox>();
    const std::vector<Packet> first_presentation={fw::gw::vitals::level_2022(1,20),fw::gw::equipment::appearance_2022(1,0x601234)};
    const std::vector<Packet> second_presentation={fw::gw::vitals::level_2022(2,1),fw::gw::equipment::encode_2022(Update::color,2,0,12)};
    presented.join(1,"First",0,0,presented_first,0x601234,first_presentation);
    const auto presented_prior=presented.join(2,"Second",0,0,presented_second,0,second_presentation);
    require(presented_prior.size()==1&&presented_prior[0].presentation==first_presentation,
            "newcomer snapshot retains existing level and appearance packets");
    const auto presented_updates=presented_first->drain();
    require(presented_updates.size()==4&&presented_updates[2]==second_presentation[0]&&presented_updates[3]==second_presentation[1],
            "existing peer receives newcomer level and appearance after identity");
    auto first_outbox = std::make_shared<Outbox>();
    auto second_outbox = std::make_shared<Outbox>();

    require(world.join(1, "First", 0, 0, first_outbox).empty(), "first player has no prior roster");
    const auto prior = world.join(2, "Second", 25, -5, second_outbox);
    require(prior.size() == 1 && prior[0].id == 1 && prior[0].name == "First", "join returns a stable peer snapshot");
    require(world.size() == 2, "both players belong to the instance");
    auto newcomer = first_outbox->drain();
    require(newcomer.size() == 2 && newcomer[0].size() == 37 && newcomer[1].size() == 99 &&
            newcomer[0][0]==88 && newcomer[1][0]==32,
            "2022 live join defines the player before spawning its referencing agent");

    world.rotate(1,3.14159265f,1,900);
    world.move(1, 50, 0, 1000);
    require(std::abs(world.heading(1,1000))<.001f,"click destination faces travel instead of walking backward");
    auto click_start=first_outbox->drain();
    require(click_start.size()==2&&click_start[0][0]==43&&click_start[0].back()==1&&click_start[1][0]==41, "accepted click initializes forward gait before destination");
    auto before_start = world.position(1, 900);
    require(close(before_start.first, 0) && close(before_start.second, 0), "position queries before a tick do not underflow time");
    auto at_start = world.position(1, 1000);
    require(close(at_start.first, 0) && close(at_start.second, 0), "movement starts from the authoritative position");
    world.tick(1100);
    auto midway = world.position(1, 1100);
    require(midway.first > 28 && midway.first < 30, "fixed-step tick advances interpolated movement");
    world.tick(1230);
    auto at_target = world.position(1, 1230);
    require(close(at_target.first, 50) && close(at_target.second, 0), "movement converges to its server target");
    auto updates = first_outbox->drain();
    require(!updates.empty() && updates.back().size() == 6 && updates.back()[0] == 40,
            "movement completion broadcasts a stop");

    world.move(1, 9000, 0, 2000);
    require(close(world.position(1, 2000).first, 50), "distant destination does not teleport");
    require(close(world.position(1, 2100).first, 78.8f), "distant destination retains server speed");
    fails_with([&] { world.move(1, 10001, 0, 2000); }, "movement bounds");
    world.move(1, 100, 0, 2000);
    fails_with([&] { world.move(1, 110, 0, 1999); }, "simulation clock regressed");
    world.tick(2230);
    auto after_budget_refill = world.position(1, 2230);
    require(close(after_budget_refill.first, 100), "replacement destination follows server travel time");

    Instance replanned;
    auto replanned_outbox = std::make_shared<Outbox>();
    replanned.join(7, "Replanned", 0, 0, replanned_outbox);
    replanned.move(7, 100, 0, 1000);
    (void)replanned_outbox->drain();
    replanned.move(7, 80, 0, 1200);
    require(close(replanned.position(7, 1365).first, 80),
            "replanned movement duration starts from the interpolated authoritative position");

    world.leave(2);
    require(world.size() == 1, "leaving removes the player");
    auto departed = first_outbox->drain();
    require(departed.size() >= 2 && departed[departed.size() - 2][0] == 33 &&
            departed.back()==Packet{89,0,2,0,0,0},
            "2022 leave destroys the agent then clears its player record");
    fails_with([&] { (void)world.position(2, 2300); }, "world player is not joined");

    Outbox oversized;
    oversized.enqueue({Packet(65537)});
    fails_with([&] { (void)oversized.drain(); }, "world outbound queue limit");

    Instance crowded;
    std::vector<std::shared_ptr<Outbox>> crowded_outboxes;
    crowded_outboxes.reserve(400);
    std::vector<Instance::Snapshot> last_snapshot;
    for (std::uint32_t id = 1; id <= 400; ++id) {
        auto peer_outbox = std::make_shared<Outbox>();
        crowded_outboxes.push_back(peer_outbox);
        last_snapshot = crowded.join(id, "P" + std::to_string(id), float(id), 0, peer_outbox);
        require(last_snapshot.size() == id - 1, "crowded join receives every current peer");
    }
    require(crowded.size() == 400, "crowded instance retains all joined players");
    auto slow_peer = crowded_outboxes.front()->drain();
    require(slow_peer.size() == 2 * 399, "bounded fanout queues every newcomer identity for a slow peer");
    for (std::size_t index = 1; index < crowded_outboxes.size(); ++index)
        (void)crowded_outboxes[index]->drain();

    Instances instances;
    {
        auto first = instances.acquire(10);
        auto second = instances.acquire(10);
        auto isolated = instances.acquire(11);
        require(&first.instance() == &second.instance(), "matching world id shares one instance");
        require(&first.instance() != &isolated.instance(), "different world id remains isolated");
    }
    Instance navigable;
    navigable.configure_navigation(fixture_mesh());
    auto nav_outbox=std::make_shared<Outbox>();
    navigable.join(99,"Nav",2,5,nav_outbox);
    navigable.move(99,28,5,1000);navigable.tick(1050);
    require(navigable.position(99,1050).first>10&&navigable.position(99,1050).first<20,
            "click crosses a cell before its destination");
    navigable.move(99,3,5,1051);navigable.tick(1200);
    require(close(navigable.position(99,1200).first,3),"replacement click uses the current cell rather than stale start cell");
    Instance interrupted;auto interrupted_out=std::make_shared<Outbox>();
    interrupted.configure_navigation(fixture_mesh());interrupted.join(1,"Interrupted",2,5,interrupted_out);
    interrupted.move(1,28,5,1000);interrupted.tick(1050);
    const float interrupted_x=interrupted.position(1,1051).first;
    interrupted.steer(1,1,0,1051);interrupted.tick(1076);interrupted.steer(1,0,0,1076);
    // Portal clearance in the navigation slide can trim a fraction of a
    // world unit; the interruption must still continue forward from the
    // materialized click position instead of snapping to its old route start.
    const float keyboard_advance=interrupted.position(1,1076).first-interrupted_x;
    require(keyboard_advance>=7.f&&keyboard_advance<=7.21f,"keyboard interruption continues from the current click surface");
    auto invalid_spawn=std::make_shared<Outbox>();
    fails_with([&]{navigable.join(100,"Outside",40,5,invalid_spawn);},"spawn outside navigable mesh");
    {
        Instance healing;auto owner=std::make_shared<Outbox>(),other=std::make_shared<Outbox>();
        healing.join(1,"Caster",0,0,owner);healing.join(2,"Stranger",0,0,other);
        healing.set_vitals(1,{100,480,10,20},100);healing.set_vitals(2,{100,480,10,20},100);
        owner->drain();other->drain();
        require(healing.consume_energy(1,5)&&healing.resources(1)->energy==5,"cast energy spends authoritative resource atomically");
        require(!healing.consume_energy(1,6)&&healing.resources(1)->energy==5,"insufficient cast energy leaves resource unchanged");
        require(healing.valid_heal_target(1,1,0)&&!healing.valid_heal_target(1,2,1248),"healing targets require self or live party membership");
        require(healing.heal_target(1,1,500,0)&&healing.resources(1)->health==480,"healing caps at authoritative maximum");
        require(!healing.heal_target(1,2,20,1248)&&healing.resources(2)->health==100,"stranger healing is rejected without health mutation");
        require(!healing.heal_target(1,1,20,0,false),"other-only healing cannot target self");
        require(!other->drain().empty(),"changed health is published to observers");
        const auto before_regeneration=healing.resources(1)->energy;
        owner->drain();healing.tick(1000);healing.tick(2500);
        require(healing.resources(1)->energy==before_regeneration+1,"world ticks restore innate energy at two pips");
        require(!owner->drain().empty(),"regenerated energy is published to its owner");
    }
    {
        Instance arena;arena.configure_recovery(fw::gw::recovery::development_schedule);
        auto first=std::make_shared<Outbox>(),second=std::make_shared<Outbox>(),far=std::make_shared<Outbox>();
        arena.join(1,"Caster",0,0,first,0,{}, {},100,1);
        arena.join(2,"Opponent",100,0,second,0,{}, {},100,2);
        arena.join(3,"Distant",200,0,far,0,{}, {},100,2);
        arena.set_vitals(1,{480,480,20,20},100);arena.set_vitals(2,{480,480,20,20},100);arena.set_vitals(3,{480,480,20,20},100);
        const auto source=*arena.effect_scope(1),target=*arena.effect_scope(2);
        require(!arena.party_action(1,158,2,100),"opponents cannot merge parties");
        require(arena.apply_condition(source,target,fw::gw::combat::conditions::Kind::weakness,100,10000),"arena condition source accepted");
        const auto plan=*fw::gw::skills::effects::crystal_wave(12);
        first->drain();second->drain();far->drain();
        require(arena.crystal_wave(source,plan,100),"hostile crystal wave accepted");
        require(packet_count(first->drain(),21)==0&&packet_count(second->drain(),21)==0&&packet_count(far->drain(),21)==0,
            "Crystal Wave native sentinel does not invent a victim particle");
        require(arena.resources(2)->health==409&&arena.resources(1)->health==480&&arena.resources(3)->health==480,"crystal wave damages adjacent foe only and adds removed condition bonus");
        require(arena.crystal_wave(source,plan,100)&&arena.resources(2)->health==351,"condition cured once, second wave has base damage only");
        auto invalid=plan;invalid.base_damage=999;
        require(!arena.crystal_wave(source,invalid,100)&&arena.resources(2)->health==351,"client-like forged damage rejected without mutation");
        auto stale=source;++stale.generation;
        require(!arena.crystal_wave(stale,plan,100),"stale caster lifetime rejected");
        for(unsigned n=0;n<7;++n)require(arena.crystal_wave(source,plan,100),"repeated crystal wave resolves through death");
        require(arena.resources(2)->health==0,"lethal damage caps at zero");
        const auto corpse=arena.position(2,100);
        arena.move(2,110,0,100);arena.steer(2,1,0,100);arena.tick(1100);
        require(arena.position(2,1100)==corpse&&!arena.follow(2,1,1100),"dead players cannot move or follow");
        require(arena.resources(2)->health==0,"dead players do not regenerate or resurrect on ticks");
    }
    {
        Instance idle;idle.configure_recovery(fw::gw::recovery::development_schedule);
        auto idle_box=std::make_shared<Outbox>();idle.join(1,"Resting",0,0,idle_box);
        idle.set_vitals(1,{100,480,20,20},100);idle.tick(100);idle.tick(5100);idle.tick(8100);
        require(idle.resources(1)->health==106,"automatic one-pip recovery heals six points over three seconds");
        idle.tick(11100);require(idle.resources(1)->health==118,"automatic recovery ramps to two pips");
        idle_box->drain();idle.tick(60100);require(idle.resources(1)->health==480,"delayed recovery tick partitions ramp boundaries and caps health");
        const auto displayed_pips=[](const std::vector<Packet>& packets){for(auto it=packets.rbegin();it!=packets.rend();++it)
            if(it->size()==14&&it->at(0)==161&&packet_u32(*it,2)==44&&packet_u32(*it,6)==1)return packet_float(*it,10);throw std::runtime_error("missing healthpip update");};
        require(displayed_pips(idle_box->drain())==0,"fullhealth hides automatichealthpips");
        const auto resting=*idle.effect_scope(1);const auto breeze=*fw::gw::skills::regeneration::reviewed(288,0);
        require(idle.apply_health_regeneration(resting,1,breeze,60100,75100,0),"fullhealth skillregen setup");idle_box->drain();idle.tick(60200);
        require(std::abs(displayed_pips(idle_box->drain())-8.f/480)<.000001,"fullhealth stillshows four skillregenpips");
        idle.tick(75200);require(displayed_pips(idle_box->drain())==0,"expiredskillregen leaves no naturalpips atfullhealth");
        require(idle.apply_condition(resting,resting,fw::gw::combat::conditions::Kind::poison,75200,5000),"fullhealth degeneration setup");idle_box->drain();idle.tick(75300);
        const auto degeneration=idle_box->drain();require(displayed_pips(degeneration)<0&&health_from_packets(degeneration,1,480)<480,"negativeexternalpips remainvisible andhealthdeclines");
    }
    {
        Instance capacity;auto capacity_box=std::make_shared<Outbox>();
        capacity.join(1,"Elementalist",0,0,capacity_box);capacity.set_vitals(1,{480,480,5,20},100);
        capacity_box->drain();
        require(capacity.energy_capacity(1,6,12,1000)&&capacity.resources(1)->max_energy==56&&capacity.resources(1)->energy==41,
            "Energy Storage alters actual world capacity and current energy by36");
        require(!capacity_box->drain().empty(),"capacity change publishes actual owner vitals");
        require(capacity.consume_energy(1,36),"capacity provides spendable energy");
        require(capacity.energy_capacity(1,6,0,1000)&&capacity.resources(1)->energy==0,
            "rank removal clamps only UI while retaining negative energy");
        require(!capacity.consume_energy(1,1)&&capacity.restore_energy(1,20)&&!capacity.consume_energy(1,1),
            "returns repay debt before another cast can spend energy");
        capacity.tick(1000);capacity.tick(4000);
        require(capacity.resources(1)->energy==0&&capacity.restore_energy(1,10)&&capacity.resources(1)->energy==1,
            "world regeneration and returns repay the remaining debt exactly");
        require(capacity.energy_capacity(1,8,12,4000)&&capacity.resources(1)->max_energy==20,
            "secondary Energy Storage cannot raise non-Elementalist capacity");
        require(!capacity.energy_capacity(99,6,12,4000),"capacity mutation cannot target an absent actor");
        Instance armored;auto armor_box=std::make_shared<Outbox>();
        armored.join(1,"Armored caster",0,0,armor_box);armored.set_vitals(1,{480,480,0,20},100);
        require(armored.energy_capacity(1,6,0,1000,{10,2})&&armored.resources(1)->energy==10&&armored.resources(1)->max_energy==30,
            "owned basic armor changes actual capacity without refilling deficit");
        armored.tick(1000);armored.tick(4000);
        require(armored.resources(1)->energy==14,"four armor-adjusted pips restore fourenergy perthree seconds");
        require(armored.energy_capacity(1,6,0,4000,{})&&armored.resources(1)->energy==4&&armored.resources(1)->max_energy==20,
            "removing armor reverses capacity delta");
        armored.tick(7000);require(armored.resources(1)->energy==6,"armor removal restores innate two-pip regeneration");
        Instance speed_world;auto owner=std::make_shared<Outbox>();
        speed_world.join(1,"Runner",0,0,owner);
        speed_world.set_vitals(1,{480,480,5,20},100);
        require(speed_world.restore_energy(1,100)&&speed_world.resources(1)->energy==20,"energy return caps at authoritative maximum");
        require(!speed_world.restore_energy(999,10),"energy return rejects absent actor");
        owner->drain();speed_world.move(1,10000,0,0);
        require(speed_world.movement_multiplier(1,1.5f,1000),"Dash speed accepted");
        require(std::abs(speed_world.position(1,1000).first-288)<.01f,"speed change preserves elapsed ordinary movement");
        speed_world.tick(2000);require(std::abs(speed_world.position(1,2000).first-720)<.25f,"click movement uses actual432 speed");
        require(speed_world.movement_multiplier(1,1.f,2000),"Dash expiration accepted");
        speed_world.tick(3000);require(std::abs(speed_world.position(1,3000).first-1008)<.5f,"expiration preserves position and restores288 speed");
        require(!speed_world.movement_multiplier(1,2.f,3000),"unbounded speed boost rejected");
        auto packets=owner->drain();bool base=false;
        for(const auto& p:packets){
            if(p.size()==10&&p[0]==39)base=true;
            if(p.size()==11&&p[0]==43){const auto fraction=std::bit_cast<float>(std::uint32_t(p[6])|(std::uint32_t(p[7])<<8)|(std::uint32_t(p[8])<<16)|(std::uint32_t(p[9])<<24));require(fraction<=1.f,"Dash must never violate native fraction assertion");}
        }
        require(base,"Dash publishes actual base-speed property");
        Instance held;auto held_box=std::make_shared<Outbox>();held.join(1,"Held",0,0,held_box);
        require(held.movement_multiplier(1,1.5f,0),"held Dash enabled");held.steer(1,766,0,0);held.tick(1000);
        require(std::abs(held.position(1,1000).first-432)<.01f,"held movement uses boosted speed");
        require(held.movement_multiplier(1,1.f,1000),"held Dash expires");held.tick(2000);
        require(std::abs(held.position(1,2000).first-720)<.01f,"held velocity is rescaled on expiration");
    }
    {
        Instance stride;auto out=std::make_shared<Outbox>();stride.join(1,"Stride",0,0,out);
        stride.set_vitals(1,{480,480,40,40},100);
        const auto scope=*stride.effect_scope(1);
        require(stride.status_mutation(scope,169,7,0,100,10000,true),"preexisting enchantment");
        require(stride.status_mutation(scope,1727,8,0,100,1100,true),"Natural Stride canonical stance");
        require(stride.status_snapshot(scope,100)->size()==2,"existing enchantment preserves Natural Stride");
        require(!stride.status_mutation(scope,1727,9,0,101,1100,true),"Natural Stride rejects malformed duration");
        out->drain();
        require(stride.status_mutation(scope,169,10,0,200,10000,true),"new enchantment application");
        const auto active=*stride.status_snapshot(scope,200);
        require(std::none_of(active.begin(),active.end(),[](const auto& entry){return entry.skill==1727;}),"committed enchantment ends exact Natural Stride");
        const auto packets=out->drain();
        require(std::count(packets.begin(),packets.end(),fw::gw::skills::casting::effect_remove(1,8))==1,"Natural Stride end published once");
    }
    {
        Instance statuses;auto box=std::make_shared<Outbox>();statuses.join(1,"Effects",0,0,box);
        const auto scope=*statuses.effect_scope(1);
        require(statuses.status_mutation(scope,169,1,12,100,1100,true),"reviewed enchantment accepted");
        require(statuses.has_enchantment(scope,1,100),"authoritative enchantment query");
        require(!statuses.has_enchantment(scope,1,1100),"expired enchantment unavailable");
        auto packets=box->drain();require(packets.size()==1&&packets[0]==fw::gw::emotes::effects_2022(1,0x80),"status mask published");
        auto peer=std::make_shared<Outbox>();const auto snapshot=statuses.join(2,"Observer",0,0,peer);
        bool found=false;for(const auto& p:snapshot.at(0).presentation)if(p==fw::gw::emotes::effects_2022(1,0x80))found=true;
        require(found,"late join receives active enchantment mask");box->drain();
        require(statuses.status_mutation(scope,169,2,12,200,1200,true),"renewal accepted");
        require(statuses.status_mutation(scope,169,1,0,200,{},false),"old effect removal idempotent");
        require(statuses.has_enchantment(scope,1,200),"old removal preserves renewal");
        require(statuses.status_mutation(scope,169,2,0,1200,{},false),"natural expiry removal accepted");
        require(!statuses.has_enchantment(scope,1,1200),"removed effect absent");
        statuses.leave(1);statuses.join(1,"Replacement",0,0,box);
        require(*statuses.effect_scope(1)!=scope,"reused actor gets new lifetime");
        require(!statuses.status_mutation(scope,169,3,12,1300,2300,true),"stale actor mutation denied");
        require(!statuses.has_enchantment(scope,1,1300),"stale caller query denied");
    }
    {
        using namespace fw::gw::skills;
        const auto breeze=*regeneration::reviewed(288,12),troll=*regeneration::reviewed(446,12);
        Instance timed;auto first=std::make_shared<Outbox>(),target=std::make_shared<Outbox>(),third=std::make_shared<Outbox>();
        timed.join(1,"Monk One",0,0,first);timed.join(2,"Ranger",10,0,target);timed.join(3,"Monk Two",20,0,third);
        for(unsigned id=1;id<=3;++id)timed.set_vitals(id,{100,1000,20,20},100);
        const auto s1=*timed.effect_scope(1),s2=*timed.effect_scope(2),s3=*timed.effect_scope(3);
        require(!timed.apply_health_regeneration(s1,2,breeze,1000,16000,1248),"regeneration rejects nonparty targets");
        require(timed.party_action(1,158,2,0)&&timed.party_action(2,154,1,0)&&
            timed.party_action(1,158,3,0)&&timed.party_action(3,154,1,0),"regeneration test party formed");
        first->drain();target->drain();third->drain();
        auto wrong=breeze;++wrong.pips;
        require(!timed.apply_health_regeneration(s1,2,wrong,1000,16000,1248)&&
            !timed.apply_health_regeneration(s1,2,breeze,1000,16001,1248)&&
            !timed.apply_health_regeneration(s1,2,breeze,1000,16000,9)&&
            !timed.apply_health_regeneration(s1,1,breeze,1000,16000,0,false)&&
            !timed.apply_health_regeneration(s1,2,troll,1000,14000,1248,true)&&
            !timed.apply_health_regeneration({s1.instance,1,s1.generation+1},2,breeze,1000,16000,1248)&&
            !timed.apply_health_regeneration(s1,2,breeze,1000,16000,std::numeric_limits<float>::infinity()),
            "reviewed magnitude, exact lifetime, range, self policy and caster generation are authoritative");
        require(first->drain().empty()&&target->drain().empty()&&third->drain().empty(),"rejected regeneration emits no fabricated effects");
        require(timed.status_mutation(s2,169,1,12,1000,20000,true),"independent low-range caster buff remains compatible");
        first->drain();target->drain();third->drain();
        require(timed.apply_health_regeneration(s1,2,breeze,1000,16000,1248),"party health regeneration applied");
        const auto add=casting::effect_add(2,288,12,0x80000000u,15.f);
        for(const auto& out:{first,target,third}){const auto packets=out->drain();require(std::count(packets.begin(),packets.end(),add)==1,
            "target effect ID and exact opcode65 reaches every player once");}
        timed.tick(1100);
        require(std::abs(health_from_packets(target->drain(),2,1000)-101.6)<.001,"100ms tick retains fractional HP");
        first->drain();third->drain();
        require(timed.heal_target(1,2,10,1248,true,1150),"direct heal advances pending regeneration first");
        require(std::abs(health_from_packets(target->drain(),2,1000)-112.4)<.001,"direct heal preserves the existing fraction");
        first->drain();third->drain();
        require(timed.consume_energy(2,1),"energy-only mutation permitted during regeneration");
        require(std::abs(health_from_packets(target->drain(),2,1000)-112.4)<.001,"energy publication does not truncate health");
        timed.tick(1120);require(target->drain().empty(),"queued older tick cannot rewind health after cast completion");
        require(timed.apply_health_regeneration(s3,2,breeze,1200,16200,1248),"different caster renews the same target skill");
        const auto renewal=target->drain();
        require(std::count(renewal.begin(),renewal.end(),casting::effect_remove(2,0x80000000u))==1&&
            std::count(renewal.begin(),renewal.end(),casting::effect_add(2,288,12,0x80000001u,15.f))==1,
            "cross-caster renewal removes exactly the previous target-wide identity");
        first->drain();third->drain();
        require(timed.apply_health_regeneration(s2,2,troll,1200,14200,0),"self-only Troll stacks with ally enchantment");
        const auto troll_add=target->drain();require(std::count(troll_add.begin(),troll_add.end(),casting::effect_add(2,446,12,0x80000002u,13.f))==1,
            "distinct regeneration source receives another collision-free effect ID");
        first->drain();third->drain();
        auto late=std::make_shared<Outbox>();const auto snapshots=timed.join(4,"Late Observer",0,0,late);
        const auto selected=std::find_if(snapshots.begin(),snapshots.end(),[](const auto& row){return row.id==2;});
        require(selected!=snapshots.end()&&packet_count(selected->presentation,65)==2,"late observer sees both current timed effects");
        first->drain();target->drain();third->drain();
        timed.tick(17000);
        // 1000..1200:16 HP/s;1200..14200:20 capped HP/s;
        //14200..16200:16 HP/s. Add the direct10 HP without rounding loss.
        require(timed.resources(2)->health==406&&std::abs(health_from_packets(target->drain(),2,1000)-405.2)<.001,
            "late tick integrates overlap cap and both exact expiry boundaries");
        const auto observed=first->drain();require(packet_count(observed,67)==2&&
            std::count(observed.begin(),observed.end(),casting::effect_remove(2,0x80000001u))==1&&
            std::count(observed.begin(),observed.end(),casting::effect_remove(2,0x80000002u))==1,
            "expired State sources retain removal IDs until both opcode67 publications");
        require(timed.has_enchantment(s1,2,17000),"regeneration expiry preserves an unrelated enchantment");
        timed.tick(18000);require(packet_count(target->drain(),67)==0,"natural expiry never emits duplicate removals");
        require(!timed.apply_health_regeneration(s1,2,breeze,17000,32000,1248),"stale source timestamp rejects rather than rewinding");
    }
    {
        using namespace fw::gw::skills;
        const auto breeze=*regeneration::reviewed(288,0);
        Instance renewal;auto box=std::make_shared<Outbox>();renewal.join(1,"Alive",0,0,box);
        renewal.set_vitals(1,{100,480,20,20},100);const auto original=*renewal.effect_scope(1);box->drain();
        require(renewal.apply_health_regeneration(original,1,breeze,0,15000,0),"time zero is a valid regeneration start");box->drain();
        renewal.tick(50);
        require(std::abs(health_from_packets(box->drain(),1,480)-100.4)<.001,"small ticks preserve a sub-HP source contribution");
        renewal.set_vitals(1,{0,480,20,20},100);const auto death=box->drain();
        require(packet_count(death,67)==1&&!renewal.has_enchantment(original,1,50),"death removes source identity and presentation immediately");
        require(!renewal.apply_health_regeneration(original,1,breeze,50,15050,0)&&!renewal.heal_target(1,1,100,0,true,50),
            "regeneration and direct healing cannot resurrect a dead target");
        renewal.set_vitals(1,{100,480,20,20},100);box->drain();renewal.tick(1000);
        require(renewal.resources(1)->health==100&&packet_count(box->drain(),65)==0,"resurrection never carries a removed regeneration source");
        require(renewal.apply_health_regeneration(original,1,breeze,1000,16000,0),"resurrected actor may receive a new source");box->drain();
        require(renewal.status_mutation(original,288,0x80000001u,0,1250,{},false),"explicit removal advances source contribution to removal time");
        const auto removed=box->drain();require(packet_count(removed,67)==1&&std::abs(health_from_packets(removed,1,480)-102.)<.001,
            "explicit removal reconciles mechanics and presentation exactly once");
        renewal.tick(2000);require(renewal.resources(1)->health==102,"removed source no longer regenerates");box->drain();
        renewal.leave(1);renewal.join(1,"Different Lifetime",0,0,box);renewal.set_vitals(1,{100,480,20,20},100);box->drain();
        require(!renewal.apply_health_regeneration(original,1,breeze,2000,17000,0)&&
            renewal.apply_health_regeneration(*renewal.effect_scope(1),1,breeze,2000,17000,0),
            "actor reuse rejects stale caster scope while permitting the new lifetime");
    }
    {
        using namespace fw::gw::skills;
        const auto breeze=*regeneration::reviewed(288,12),troll=*regeneration::reviewed(446,12);
        Instance delayed,fragmented;auto one=std::make_shared<Outbox>(),many=std::make_shared<Outbox>();
        for(auto pair:{std::pair{&delayed,one},std::pair{&fragmented,many}}){
            pair.first->join(1,"Timing",0,0,pair.second);pair.first->set_vitals(1,{100,1000,20,20},100);
            const auto scope=*pair.first->effect_scope(1);
            require(pair.first->apply_health_regeneration(scope,1,breeze,0,15000,0)&&
                pair.first->apply_health_regeneration(scope,1,troll,0,13000,0),"timing sources accepted");pair.second->drain();
        }
        delayed.tick(16000);const auto once=health_from_packets(one->drain(),1,1000);
        double split=100;for(unsigned now=25;now<=16000;now+=25){fragmented.tick(now);const auto packets=many->drain();
            if(packet_count(packets,161))split=health_from_packets(packets,1,1000);}
        require(std::abs(once-392)<.001&&std::abs(once-split)<.001,"fragmented and delayed world ticks integrate the same exact overlapped health");
        require(fragmented.heal_target(1,1,10000,0,true,16000)&&fragmented.resources(1)->health==1000,
            "direct healing retains the authoritative health cap after regeneration");
    }
    {
        using namespace fw::gw::skills;
        Instance group;std::array<std::shared_ptr<Outbox>,9> queues;
        for(unsigned id=1;id<=8;++id)queues[id]=std::make_shared<Outbox>();
        group.join(1,"Primary Monk",0,0,queues[1]);group.public_professions(1,3,0);
        group.join(2,"Boundary Ally",3000,4000,queues[2]);group.join(3,"Dead Ally",10,0,queues[3]);
        group.join(4,"Click Outside",4999,0,queues[4]);group.join(5,"Click Inside",5010,0,queues[5]);
        group.join(6,"Key Outside",4990,0,queues[6]);group.join(7,"Key Inside",5015,0,queues[7]);
        group.join(8,"Stranger",0,0,queues[8]);
        for(unsigned id=1;id<=8;++id)group.set_vitals(id,{id==3?0u:100u,1000,20,20},100);
        for(unsigned id=2;id<=7;++id)require(group.party_action(1,158,id,0)&&group.party_action(id,154,1,0),"party healing recipients joined");
        const auto scope=*group.effect_scope(1);const auto plan=*effects::party_heal(287,12,12);
        require(plan.base_heal==66&&plan.caster_divine_heal==38,"party test uses reviewed pinned skill and DF values");
        auto invalid=plan;invalid.base_heal++;
        for(auto variant:{invalid,effects::PartyHealPlan{plan.skill,plan.rank,plan.base_heal,37,5000},
            effects::PartyHealPlan{plan.skill,plan.rank,plan.base_heal,plan.caster_divine_heal,5001},
            effects::PartyHealPlan{169,plan.rank,plan.base_heal,plan.caster_divine_heal,5000}})
            require(!group.party_heal(scope,variant,100),"arbitrary party heal magnitude, DF, radius and skill rejected");
        require(!group.party_heal({scope.instance,scope.actor,scope.generation+1},plan,100),"party healer lifetime is authoritative");
        group.move(4,5019,0,0);group.move(5,4990,0,0);group.steer(6,1,0,0);group.steer(7,-1,0,0);
        const auto breeze=*regeneration::reviewed(288,0);
        require(group.apply_health_regeneration(*group.effect_scope(2),2,breeze,0,15000,0),"party target has fractional regeneration before heal");
        for(unsigned id=1;id<=8;++id)queues[id]->drain();
        require(group.party_heal(scope,plan,100),"owned party healing succeeds");
        require(group.resources(1)->health==204,"party spell grants DF bonus only to primary-Monk caster");
        require(group.resources(2)->health==167&&std::abs(health_from_packets(queues[2]->drain(),2,1000)-166.8)<.001,
            "XY radius includes exact diagonal boundary and party heal preserves regenerated fraction");
        require(group.resources(3)->health==0,"party heal never revives dead member");
        require(group.resources(4)->health==100&&group.resources(5)->health==166,
            "click recipients use completion position rather than cached movement start");
        require(group.resources(6)->health==100&&group.resources(7)->health==166,
            "held movement recipients use authoritative completion position");
        require(group.resources(8)->health==100,"party spell excludes nearby strangers");
        for(unsigned id=1;id<=8;++id){const auto packets=queues[id]->drain();require(!packet_count(packets,65),"instant party healing does not invent a timed effect icon");}
        require(!group.party_heal(scope,plan,99),"party heal cannot rewind the caster's completed health cursor");
        group.public_professions(1,6,3);
        require(!group.party_heal(scope,plan,200),"secondary Monk cannot receive Divine Favor as a primary attribute");
        const auto secondary=*effects::party_heal(287,12,0);
        require(group.party_heal(scope,secondary,200)&&group.resources(1)->health==270,
            "secondary Monk party healing accepts reviewed base healing without DF");
        require(group.party_action(5,160,0,200),"member leaves party before next healing");
        const auto departed=group.resources(5)->health;
        require(group.party_heal(scope,secondary,300)&&group.resources(5)->health==departed,"party membership rechecked for each cast");
        group.set_vitals(1,{0,1000,20,20},100);
        require(!group.party_heal(scope,secondary,400),"dead caster cannot heal party");
        group.leave(1);group.join(1,"Reused Actor",0,0,queues[1]);group.set_vitals(1,{100,1000,20,20},100);
        require(!group.party_heal(scope,secondary,500),"party spell from replaced actor scope rejected");
    }
    {
        using namespace fw::gw::skills;
        Instance area;area.configure_navigation(party_healing_mesh());
        auto owner=std::make_shared<Outbox>(),other_floor=std::make_shared<Outbox>(),across_wall=std::make_shared<Outbox>();
        area.join(1,"Caster",195,100,owner,0,{},0);area.join(2,"Upstairs",195,100,other_floor,0,{},1);
        area.join(3,"Other Room",225,100,across_wall,0,{},0);
        for(unsigned id=1;id<=3;++id)area.set_vitals(id,{100,480,20,20},100);
        for(unsigned id=2;id<=3;++id)require(area.party_action(1,158,id,0)&&area.party_action(id,154,1,0),"layered party joined");
        require(!area.valid_heal_target(1,2,5000),"single-target surface restriction remains distinct from party area");
        require(area.party_heal(*area.effect_scope(1),*effects::party_heal(287,0,0),0)&&
            area.resources(1)->health==130&&area.resources(2)->health==130&&area.resources(3)->health==130,
            "party area affects nearby members across stairs, overlapping floors and walls");
        area.set_vitals(1,{470,480,20,20},100);area.set_vitals(2,{470,480,20,20},100);area.set_vitals(3,{470,480,20,20},100);
        require(area.party_heal(*area.effect_scope(1),*effects::party_heal(1397,15,0),100)&&
            area.resources(1)->health==480&&area.resources(2)->health==480&&area.resources(3)->health==480,
            "Light of Deliverance party healing caps each recipient's own maximum");
        require(area.party_heal(*area.effect_scope(1),*effects::party_heal(3232,0,0),200),"reviewed PvP party heal variant has the same owned area API");
    }
    {
        using namespace fw::gw::skills;
        Instance cleanses;auto healer=std::make_shared<Outbox>(),recipient=std::make_shared<Outbox>(),observer=std::make_shared<Outbox>();
        cleanses.join(1,"Primary Monk",0,0,healer);cleanses.public_professions(1,3,0);
        cleanses.join(2,"Hexed Ally",10,0,recipient);cleanses.join(3,"Observer",20,0,observer);
        for(unsigned id=1;id<=3;++id)cleanses.set_vitals(id,{100,1000,20,20},100);
        require(cleanses.party_action(1,158,2,0)&&cleanses.party_action(2,154,1,0),"hex removal party formed");
        const auto caster=*cleanses.effect_scope(1),ally=*cleanses.effect_scope(2);
        const auto cure=*effects::removal(2003,12,12),remove=*effects::removal(301,0,12);
        require(cure.conditional_heal==102&&cure.divine_heal==38,"Cure Hex test uses actual pinned magnitudes");
        std::vector<unsigned> hex_skills;for(const auto& row:skill_catalog_2022)if(row.id&&row.type==4){hex_skills.push_back(row.id);if(hex_skills.size()==2)break;}
        require(hex_skills.size()==2,"reviewed catalog contains two hex categories for lifecycle test");
        require(cleanses.apply_health_regeneration(ally,2,*regeneration::reviewed(288,0),1000,16000,0)&&
            cleanses.status_mutation(ally,169,9,12,1000,21000,true)&&
            cleanses.status_mutation(ally,hex_skills[0],100,0,1000,5000,true)&&
            cleanses.status_mutation(ally,hex_skills[1],1,0,1000,5000,true),"independent real registry sources applied");
        for(const auto& queue:{healer,recipient,observer})queue->drain();
        auto bad=cure;++bad.conditional_heal;
        for(auto plan:{bad,effects::RemovalPlan{cure.skill,cure.rank,effects::Category::enchantment,1,cure.conditional_heal,cure.divine_heal,1248,true},
            effects::RemovalPlan{cure.skill,cure.rank,effects::Category::hex,2,cure.conditional_heal,cure.divine_heal,1248,true},
            effects::RemovalPlan{cure.skill,cure.rank,effects::Category::hex,1,cure.conditional_heal,37,1248,true},
            effects::RemovalPlan{cure.skill,cure.rank,effects::Category::hex,1,cure.conditional_heal,cure.divine_heal,1249,true}})
            require(!cleanses.remove_effects(caster,2,plan,1050),"unreviewed removal/healing/category/count/range rejected");
        require(!cleanses.remove_effects({caster.instance,1,caster.generation+1},2,cure,1050)&&
            !cleanses.remove_effects(caster,3,cure,1050),"removal checks caster lifetime and target party ownership");
        require(healer->drain().empty()&&recipient->drain().empty()&&observer->drain().empty(),"invalid removal cannot emit cosmetic success or mutate health");
        require(cleanses.remove_effects(caster,2,cure,1050),"Cure Hex applied to owned ally");
        const auto observed=observer->drain();
        require(packet_count(observed,67)==1&&std::count(observed.begin(),observed.end(),casting::effect_remove(2,1))==1,
            "same-millisecond cover ordering removes latest insertion rather than numerically greatest ID");
        require(cleanses.resources(1)->health==100&&std::abs(health_from_packets(recipient->drain(),2,1000)-240.4)<.001,
            "Cure healing and caster's DF heal target ally while preserving fractional regeneration");
        require(cleanses.has_enchantment(caster,2,1050),"hex removal preserves actual enchantment sources");healer->drain();
        require(cleanses.remove_effects(caster,2,remove,1100),"Remove Hex removes remaining older cover");
        const auto cleared=observer->drain();
        require(packet_count(cleared,67)==1&&std::count(cleared.begin(),cleared.end(),casting::effect_remove(2,100))==1&&
            std::count(cleared.begin(),cleared.end(),fw::gw::emotes::effects_2022(2,0x80))==1,
            "last hex removal clears only hex mask and leaves enchantment mask");
        require(std::abs(health_from_packets(recipient->drain(),2,1000)-278.8)<.001,"Remove Hex grants only normal spell DF healing");healer->drain();
        require(cleanses.remove_effects(caster,2,cure,1150),"valid empty hex set is still a successful spell");
        const auto empty=observer->drain();require(packet_count(empty,67)==0&&std::abs(health_from_packets(recipient->drain(),2,1000)-317.2)<.001,
            "empty set grants DF without fabricated removal or conditional Cure healing");healer->drain();
        cleanses.tick(1250);
        require(std::abs(health_from_packets(recipient->drain(),2,1000)-318.)<.001,"removing hexes does not silently stop unrelated timed regeneration");
        for(const auto& queue:{healer,observer})queue->drain();
        require(cleanses.status_mutation(ally,hex_skills[0],7,0,1250,1300,true),"short-lived hex registered");
        for(const auto& queue:{healer,recipient,observer})queue->drain();
        require(cleanses.remove_effects(caster,2,cure,1300),"expired-only hex set remains a valid empty set");
        require(std::abs(health_from_packets(recipient->drain(),2,1000)-356.4)<.001&&packet_count(observer->drain(),67)==0,
            "expired source does not falsely trigger Cure's conditional heal or an active removal");healer->drain();
        cleanses.public_professions(1,6,3);require(!cleanses.remove_effects(caster,2,cure,1350),"secondary Monk cannot supply Divine Favor");
        cleanses.set_vitals(2,{0,1000,20,20},100);require(!cleanses.remove_effects(caster,2,*effects::removal(2003,12,0),1400),
            "removal spell cannot heal or resurrect a dead target");
    }
    {
        using namespace fw::gw::skills;
        Instance ranged;auto caster_box=std::make_shared<Outbox>(),outgoing=std::make_shared<Outbox>(),incoming=std::make_shared<Outbox>();
        ranged.join(1,"Caster",0,0,caster_box);ranged.join(2,"Moving Outside",1240,0,outgoing);ranged.join(3,"Moving Inside",1250,0,incoming);
        for(unsigned id=1;id<=3;++id)ranged.set_vitals(id,{100,480,20,20},100);
        for(unsigned id=2;id<=3;++id)require(ranged.party_action(1,158,id,0)&&ranged.party_action(id,154,1,0),"removal range party joined");
        ranged.move(2,1260,0,0);ranged.move(3,1230,0,0);
        const auto no_divine=*effects::removal(301,0,0);
        require(!ranged.remove_effects(*ranged.effect_scope(1),2,no_divine,100)&&
            ranged.remove_effects(*ranged.effect_scope(1),3,no_divine,100),"single-target removal samples range at completion for moving allies");
        require(ranged.remove_effects(*ranged.effect_scope(1),1,no_divine,100),"self removal is a valid reviewed target even without hexes");
    }
    {
        using namespace fw::gw::skills;using fw::gw::combat::conditions::Kind;
        Instance conditions;auto caster_box=std::make_shared<Outbox>(),ally_box=std::make_shared<Outbox>(),observer=std::make_shared<Outbox>();
        conditions.join(1,"Monk",0,0,caster_box);conditions.public_professions(1,3,0);
        conditions.join(2,"Ally",10,0,ally_box);conditions.join(3,"Observer",20,0,observer);
        for(unsigned id=1;id<=3;++id)conditions.set_vitals(id,{100,1000,20,20},100);
        require(conditions.party_action(1,158,2,0)&&conditions.party_action(2,154,1,0),"condition cleansing party formed");
        const auto caster=*conditions.effect_scope(1),ally=*conditions.effect_scope(2);
        const auto mend=*effects::condition_removal(275,12,12),restore=*effects::condition_removal(276,12,12),
            ailment=*effects::condition_removal(277,12,12),purge=*effects::condition_removal(278,0,12);
        require(mend.per_condition_heal==57&&restore.per_condition_heal==58&&ailment.per_condition_heal==57,"reviewed Protection rank values");
        require(conditions.apply_condition(caster,ally,Kind::blind,0,10000)&&
            conditions.apply_condition(caster,ally,Kind::weakness,0,10000)&&
            conditions.apply_condition(caster,ally,Kind::cracked_armor,0,10000),"bounded condition sources stored with real player lifetimes");
        for(const auto& queue:{caster_box,ally_box,observer})queue->drain();
        require(conditions.has_condition(ally,Kind::blind,0)&&!conditions.has_condition({ally.instance,ally.actor,ally.generation+1},Kind::blind,0),
            "condition query is lifetime scoped");
        auto invalid=ailment;++invalid.per_condition_heal;
        require(!conditions.remove_conditions(caster,2,invalid,0)&&!conditions.remove_conditions(caster,3,ailment,0)&&
            !conditions.remove_conditions(caster,1,mend,0)&&!conditions.remove_conditions(caster,1,restore,0),
            "reviewed healing, party and other-only policies enforced");
        require(conditions.remove_conditions(caster,2,ailment,0)&&conditions.resources(2)->health==252&&
            !conditions.has_condition(ally,Kind::cracked_armor,0)&&conditions.has_condition(ally,Kind::blind,0),
            "Mend Ailment removes newest condition and heals per remaining condition plus target DF");
        const auto one=observer->drain();require(packet_count(one,67)==1&&
            std::count(one.begin(),one.end(),casting::effect_remove(2,0x80000002u))==1,"latest condition source emits one exact67");
        caster_box->drain();ally_box->drain();
        require(conditions.remove_conditions(caster,2,restore,0)&&conditions.resources(2)->health==406&&
            !conditions.has_condition(ally,Kind::blind,0)&&!conditions.has_condition(ally,Kind::weakness,0),
            "Restore Condition removes all and heals per removed source");
        const auto all=observer->drain();require(packet_count(all,67)==2&&
            std::count(all.begin(),all.end(),fw::gw::emotes::effects_2022(2,0))==1,"all removals clear owned conditioned status once");
        require(conditions.resources(1)->health==100,"condition spell DF heals target rather than caster");caster_box->drain();ally_box->drain();
        require(conditions.remove_conditions(caster,2,restore,0)&&conditions.resources(2)->health==444&&packet_count(observer->drain(),67)==0,
            "empty condition set grants only ordinary spell DF without fabricated removed-source healing");
        require(conditions.apply_condition(caster,ally,Kind::blind,0,10000),"condition reapplied after cleanse");
        for(const auto& queue:{caster_box,ally_box,observer})queue->drain();
        require(conditions.remove_conditions(caster,2,mend,0)&&conditions.resources(2)->health==539&&
            !conditions.has_condition(ally,Kind::blind,0),"Mend Condition other ally gets one actual-removal heal plus DF");
        require(conditions.apply_condition(caster,caster,Kind::poison,0,1000)&&
            conditions.apply_condition(caster,caster,Kind::blind,0,1000),"server-owned self conditions represented independently");
        require(conditions.remove_conditions(caster,1,purge,0)&&conditions.resources(1)->health==138&&
            !conditions.has_condition(caster,Kind::poison,0)&&!conditions.has_condition(caster,Kind::blind,0),
            "Purge Conditions self removes all with no invented base heal");
        for(const auto& queue:{caster_box,ally_box,observer})queue->drain();
        require(!conditions.apply_condition(caster,{ally.instance,2,ally.generation+1},Kind::poison,0,1000)&&
            !conditions.apply_condition({caster.instance,1,caster.generation+1},ally,Kind::poison,0,1000)&&
            !conditions.apply_condition(caster,ally,Kind::count,0,1000)&&!conditions.apply_condition(caster,ally,Kind::poison,0,0),
            "stale source/target, unknown kind and invalid duration cannot apply conditions");
        require(caster_box->drain().empty()&&ally_box->drain().empty()&&observer->drain().empty(),"invalid condition mutation has no effect publications");
    }
    {
        using namespace fw::gw::skills;using fw::gw::combat::conditions::Kind;
        Instance delayed,split;auto once=std::make_shared<Outbox>(),frequent=std::make_shared<Outbox>();
        for(auto entry:{std::pair{&delayed,once},std::pair{&split,frequent}}){
            entry.first->join(1,"Timing",0,0,entry.second);entry.first->set_vitals(1,{100,480,20,20},100);
            const auto scope=*entry.first->effect_scope(1);
            require(entry.first->apply_condition(scope,scope,Kind::poison,0,1000)&&
                entry.first->apply_condition(scope,scope,Kind::burning,0,1500)&&
                entry.first->apply_health_regeneration(scope,1,*regeneration::reviewed(288,12),0,15000,0),"condition and regeneration sources coexist");
            entry.second->drain();
        }
        delayed.tick(1600);const auto delayed_packets=once->drain();const double final=health_from_packets(delayed_packets,1,480);
        require(std::abs(final-96.6)<.001&&packet_count(delayed_packets,67)==2,"delayed tick partitions both condition expiry times and regeneration pips exactly");
        double fragmented=100;for(unsigned now=100;now<=1600;now+=100){split.tick(now);const auto packets=frequent->drain();
            if(packet_count(packets,161))fragmented=health_from_packets(packets,1,480);}
        require(std::abs(final-fragmented)<.001,"fragmentation cannot change condition degeneration or remaining regeneration");
        const auto scope=*delayed.effect_scope(1);
        require(delayed.has_enchantment(scope,1,1600)&&!delayed.has_condition(scope,Kind::poison,1600)&&
            !delayed.has_condition(scope,Kind::burning,1600),"condition expiry preserves the real regeneration enchantment");
        auto late=std::make_shared<Outbox>();const auto rows=delayed.join(2,"Late",0,0,late);
        require(packet_count(rows.at(0).presentation,65)==1,"late join includes remaining world effect but not expired condition icons");
        once->drain();delayed.tick(1700);require(packet_count(once->drain(),67)==0,"condition expiry is published once");
        Instance cap;auto cap_box=std::make_shared<Outbox>();cap.join(1,"Cap",0,0,cap_box);cap.set_vitals(1,{100,480,20,20},100);
        const auto capped=*cap.effect_scope(1);
        for(auto kind:{Kind::poison,Kind::burning,Kind::bleeding,Kind::disease})require(cap.apply_condition(capped,capped,kind,0,1000),"stacked condition source");
        cap_box->drain();cap.tick(1000);require(cap.resources(1)->health==80,"raw18 condition pips are capped jointly at10, not applied as uncapped damage");
    }
    {
        using namespace fw::gw::skills;using fw::gw::combat::conditions::Kind;
        Instance renewal;auto box=std::make_shared<Outbox>();renewal.join(1,"Renewal",0,0,box);renewal.set_vitals(1,{100,480,20,20},100);
        const auto scope=*renewal.effect_scope(1);
        require(renewal.apply_condition(scope,scope,Kind::poison,0,1000),"initial poison applied");box->drain();
        require(renewal.apply_condition(scope,scope,Kind::poison,100,100),"short poison reapplication accepted");
        require(packet_count(box->drain(),65)==0&&renewal.has_condition(scope,Kind::poison,999),"short renewal preserves original duration and client effect ID");
        require(renewal.apply_condition(scope,scope,Kind::poison,200,2000),"long condition renews duration");const auto extended=box->drain();
        require(packet_count(extended,67)==1&&packet_count(extended,65)==1&&
            std::count(extended.begin(),extended.end(),casting::effect_remove(1,0x80000000u))==1&&
            std::count(extended.begin(),extended.end(),casting::effect_add(1,484,0,0x80000001u,2.f))==1,
            "long renewal replaces client identity without colliding with regeneration range");
        renewal.tick(2300);require(renewal.resources(1)->health==83&&!renewal.has_condition(scope,Kind::poison,2300),"renewed poison contributes through2200 only");box->drain();
        renewal.set_vitals(1,{1,480,20,20},100);
        require(renewal.apply_condition(scope,scope,Kind::burning,2300,5000)&&
            renewal.apply_health_regeneration(scope,1,*regeneration::reviewed(288,0),2300,17300,0),"low-life degeneration and regeneration sources");box->drain();
        renewal.steer(1,1,0,2300);renewal.rotate(1,INFINITY,1,2300);box->drain();
        renewal.tick(3300);const auto death=box->drain();require(renewal.resources(1)->health==0&&packet_count(death,67)==2&&
            !renewal.has_condition(scope,Kind::burning,3300)&&!renewal.has_enchantment(scope,1,3300),"lethal degeneration clears conditions and buffs with exact removals");
        require(packet_count(death,40)>=1,"lethal degeneration publishes movement stop");
        const auto corpse_position=renewal.position(1,3300);const auto corpse_heading=renewal.heading(1,3300);
        renewal.steer(1,-1,0,3400);renewal.move(1,300,0,3400);renewal.rotate(1,-INFINITY,1,3400);renewal.tick(3500);
        require(renewal.position(1,3500)==corpse_position&&renewal.heading(1,3500)==corpse_heading&&
            renewal.resources(1)->health==0,"degeneration corpse rejects movement and turning");
        require(box->drain().empty(),"dead tick does not repeat death packets");
        require(!renewal.remove_conditions(scope,1,*effects::condition_removal(278,0,0),3300),"cleansing cannot resurrect a dead target");
        renewal.set_vitals(1,{100,480,20,20},100);box->drain();renewal.tick(4300);
        require(renewal.resources(1)->health==100&&packet_count(box->drain(),67)==0,"resurrection never carries condition degeneration or stale removals");
        renewal.leave(1);renewal.join(1,"Replacement",0,0,box);renewal.set_vitals(1,{100,480,20,20},100);
        require(!renewal.has_condition(scope,Kind::burning,4300)&&!renewal.remove_conditions(scope,1,*effects::condition_removal(278,0,0),4300),
            "reused actor does not accept old condition scope");
    }
    {
        Instance floor;floor.configure_navigation(fixture_mesh(400));
        auto owner_box=std::make_shared<Outbox>(),other_box=std::make_shared<Outbox>(),near_box=std::make_shared<Outbox>();
        floor.join(1,"Owner",5,5,owner_box);floor.join(2,"Distant",600,5,other_box);floor.join(3,"Nearby",5,5,near_box);
        for(unsigned id:{1u,2u,3u})floor.set_vitals(id,{100,480,20,20},100);
        for(auto queue:{owner_box,other_box,near_box})queue->drain();
        const auto owner=*floor.effect_scope(1),distant=*floor.effect_scope(2),near=*floor.effect_scope(3);
        const auto reserved_first=floor.reserve_ground_item_id(),reserved_second=floor.reserve_ground_item_id();
        require(reserved_first==32768&&reserved_second==32769,"instance floor reservations are disjoint from owned IDs and never reused");
        fw::gw::items::Definition owned;owned.id=9;owned.file=1234;owned.type=11;owned.quantity=40;owned.encoded_name=u"Transaction fixture";
        unsigned drop_commits=0;
        auto commit_drop=[&](const auto& item){++drop_commits;require(item.source==owner&&item.source_item==9&&
            item.definition.id==50000&&item.definition.quantity==20&&item.profession==6&&item.customized==u"Transaction Owner",
            "persistent callback sees original/global identities and actual ownership metadata separately");return true;};
        require(!floor.drop_ground_item(owner,owned,20,50000,100,false,commit_drop)&&
            !floor.drop_ground_item({owner.instance,1,owner.generation+1},owned,20,50000,100,true,commit_drop)&&
            !floor.drop_ground_item(owner,owned,20,9,100,true,commit_drop)&&
            !floor.drop_ground_item(owner,owned,41,50000,100,true,commit_drop)&&drop_commits==0,
            "map admission, actor lifetime, shared identity and owned quantity validated before CAS");
        require(!floor.drop_ground_item(owner,owned,20,50000,100,true,[](const auto&){return false;})&&floor.ground_item_count()==0,
            "failed inventory CAS cannot create a world item");
        fails_with([&]{floor.drop_ground_item(owner,owned,20,50000,100,true,[](const auto&)->bool{
            throw std::runtime_error("fixture database failure");});},"fixture database failure");
        require(floor.ground_item_count()==0,"pre-commit storage exception does not publish staged item");
        for(auto queue:{owner_box,other_box,near_box})require(queue->drain().empty(),"failed drop CAS has no item publication");
        const auto drop=floor.drop_ground_item(owner,owned,20,50000,100,true,commit_drop,600000,6,u"Transaction Owner");
        require(drop&&drop_commits==1&&drop->scope.actor==32768&&floor.ground_item_count()==1,"committed real owned item becomes one ground actor");
        for(auto queue:{owner_box,other_box,near_box}){const auto packets=queue->drain();require(packets.size()==5&&
            packet_count(packets,352)==1&&packet_count(packets,345)==1&&packet_count(packets,313)==1&&
            packet_count(packets,308)==1&&packet_count(packets,32)==1,
            "successful drop publishes definition, metadata, allocation and item actor to every observer");}
        auto arrival=std::make_shared<Outbox>();floor.join(4,"Late arrival",5,5,arrival,0,{},std::nullopt,101);
        const auto arrival_packets=arrival->drain();require(packet_count(arrival_packets,352)==1&&packet_count(arrival_packets,32)==1&&packet_count(arrival_packets,308)==1,"map entry snapshots existing ground definition, actor and ownership once");
        for(auto queue:{owner_box,other_box,near_box})queue->drain();
        require(floor.select_target(3,drop->scope.actor,0)&&floor.selection(3).first==drop->scope.actor,"ground actor selection recognizes actual floor item");
        unsigned pickup_commits=0;auto pickup=[&](const auto& item,unsigned quantity){++pickup_commits;
            require(item.scope==drop->scope&&quantity<=item.definition.quantity&&item.profession==6&&item.customized==u"Transaction Owner",
                "claim callback uses exact floor lifetime/count and preserves actual item metadata");return true;};
        require(!floor.pickup_ground_item(distant,drop->scope,1,160,100,pickup)&&
            !floor.pickup_ground_item(near,drop->scope,1,160,100,pickup)&&
            !floor.pickup_ground_item(owner,{drop->scope.instance,drop->scope.actor,drop->scope.generation+1},1,160,100,pickup)&&
            !floor.pickup_ground_item(owner,drop->scope,21,160,100,pickup)&&pickup_commits==0,
            "range, allocation, ground generation and source count reject before inventory mutation");
        require(!floor.pickup_ground_item(owner,drop->scope,5,160,100,[](const auto&,unsigned){return false;})&&
            floor.ground_item(drop->scope.actor)->definition.quantity==20,"failed pickup CAS preserves real floor quantity");
        fails_with([&]{floor.pickup_ground_item(owner,drop->scope,5,160,100,[](const auto&,unsigned)->bool{
            throw std::runtime_error("fixture pickup storage failure");});},"fixture pickup storage failure");
        require(floor.ground_item(drop->scope.actor)->definition.quantity==20,"storage exception retains exclusive-claim source for retry");
        require(floor.pickup_ground_item(owner,drop->scope,5,160,100,pickup)&&pickup_commits==1&&
            floor.ground_item(drop->scope.actor)->definition.quantity==15,"partial pickup commits only accepted source quantity");
        for(auto queue:{owner_box,other_box,near_box}){const auto p=queue->drain();require(p.size()==1&&p[0][0]==56&&p[0][1]==1&&
            packet_u32(p[0],2)==50000&&packet_u32(p[0],6)==15,"partial pickup publishes312 for the surviving floor stack");}
        require(!floor.drop_ground_item(owner,owned,1,50000,101,true,[](const auto&){return true;}),"duplicate global item identity rejected while active");
        floor.tick(600100);
        require(!floor.ground_item(drop->scope.actor)->allocation,"ten-minute allocation becomes public exactly at expiry");
        for(auto queue:{owner_box,other_box,near_box})require(packet_count(queue->drain(),308)==1,"allocation expiry published once");
        std::atomic<unsigned> durable_claims=0;std::atomic<unsigned> successful_claims=0;
        auto contender=[&](const auto& who){if(floor.pickup_ground_item(who,drop->scope,15,160,600100,[&](const auto&,unsigned){
            ++durable_claims;return true;}))++successful_claims;};
        std::thread first([&]{contender(owner);}),second([&]{contender(near);});first.join();second.join();
        require(durable_claims==1&&successful_claims==1&&floor.ground_item_count()==0,"two concurrent players produce exactly one persistent pickup commit");
        for(auto queue:{owner_box,other_box,near_box}){const auto p=queue->drain();require(p.size()==2&&p[0][0]==33&&p[1][0]==92&&p[1][1]==1,
            "complete pickup despawns actor before deleting immutable ground definition");}
        require(floor.selection(3).first==0,"full pickup clears server selection of removed actor");
        fails_with([&]{floor.join(32768,"Reuse",5,5,std::make_shared<Outbox>());},"duplicate or invalid world agent id");
        const auto second_drop=floor.drop_ground_item(owner,owned,10,50001,600101,true,[](const auto&){return true;},10000);
        require(second_drop&&second_drop->scope.actor==32769&&second_drop->scope!=drop->scope,"retired raw actor IDs never refer to later drops");
        for(auto queue:{owner_box,other_box,near_box})queue->drain();
        require(!floor.pickup_ground_item(owner,drop->scope,1,160,600101,pickup),"old ground handle cannot claim replacement item");
        floor.leave(1);require(!floor.ground_item(second_drop->scope.actor)->allocation&&floor.ground_item_count()==1,
            "leaving owner releases allocation while preserving committed floor item");
        require(packet_count(near_box->drain(),308)==1&&packet_count(other_box->drain(),308)==1,"leave ownership release reaches remaining observers");
        const auto late_packets=floor.ground_packets(600102);
        require(late_packets.size()==2&&packet_count(late_packets,352)==1&&packet_count(late_packets,32)==1,
            "late join snapshot reconstructs actual ground data without expired ownership");
        floor.set_vitals(3,{0,480,20,20},100);near_box->drain();other_box->drain();
        require(!floor.pickup_ground_item(near,second_drop->scope,1,160,600102,[](const auto&,unsigned){return true;}),"dead player cannot acquire inventory from ground");
        floor.set_vitals(3,{100,480,20,20},100);near_box->drain();other_box->drain();
        require(floor.approach_ground(2,second_drop->scope,160,600102)==Instance::NpcApproach::moving,"distant pickup starts authoritative route");
        floor.steer(2,0,0,600102);require(!floor.take_ground_interaction(2,600102),"keyboard steering cancels pending pickup");
        require(floor.approach_ground(2,second_drop->scope,160,600102)==Instance::NpcApproach::moving,"cancelled ground route can restart");
        require(!floor.take_ground_interaction(2,600102),"pickup intent cannot complete at its distant destination before arrival");
        require(!floor.pickup_ground_item(distant,second_drop->scope,10,160,600102,[](const auto&,unsigned){return true;}),
            "pickup checks actual movement-time position rather than destination");
        require(floor.take_ground_interaction(2,603000)==second_drop->scope&&!floor.take_ground_interaction(2,603000),"arrival releases exact lifetime intent once");
        require(floor.pickup_ground_item(distant,second_drop->scope,10,160,603000,[](const auto&,unsigned){return true;}),
            "completed authoritative route permits in-range pickup without teleporting");
    }
    {
        using namespace fw::gw::skills;Instance kiss;auto one=std::make_shared<Outbox>(),two=std::make_shared<Outbox>(),other=std::make_shared<Outbox>();
        kiss.join(1,"Monk",0,0,one);kiss.public_professions(1,3,0);kiss.join(2,"Ally",10,0,two);kiss.join(3,"Stranger",20,0,other);
        for(unsigned id=1;id<=3;++id)kiss.set_vitals(id,{100,1000,20,20},100);
        require(kiss.party_action(1,158,2,0)&&kiss.party_action(2,154,1,0),"Kiss ownedparty setup");const auto scope=*kiss.effect_scope(1),target=*kiss.effect_scope(2);
        unsigned hex=0;for(const auto& row:skill_catalog_2022)if(row.id&&row.type==4){hex=row.id;break;}
        require(kiss.status_mutation(target,169,1,12,100,500,true)&&kiss.status_mutation(target,hex,2,0,100,1000,true),"Kiss genuine enchantment and hex sources");
        const auto plan=*effects::status_heal(283,12,10);require(!kiss.status_heal(scope,1,plan,100)&&!kiss.status_heal(scope,3,plan,100),"Kiss rejects self andstranger");
        require(!kiss.status_heal({scope.instance,scope.actor,scope.generation+1},2,plan,100),"Kiss lifetimebound");
        require(kiss.status_heal(scope,2,plan,100)&&kiss.resources(2)->health==243,"Kiss counts2realstatuses adds60+51+32");
        require(kiss.status_heal(scope,2,plan,500)&&kiss.resources(2)->health==356,"Kiss exactlyexpired enchantment excluded");
        require(kiss.status_heal(scope,2,plan,1000)&&kiss.resources(2)->health==439,"Kiss expiredhex leaves base+DF");
        auto forged=plan;++forged.per_status;require(!kiss.status_heal(scope,2,forged,1000),"Kiss alteredmagnitude denied");
        kiss.public_professions(1,6,3);require(!kiss.status_heal(scope,2,plan,1000),"Kiss secondaryMonk cannot forgeDF");
    }
    {
        using namespace fw::gw::skills;using fw::gw::combat::conditions::Kind;
        Instance death;auto box=std::make_shared<Outbox>(),observer=std::make_shared<Outbox>();
        death.join(1,"GM",0,0,box);death.join(2,"Observer",0,0,observer);
        death.set_vitals(1,{100,480,20,20},100);death.set_vitals(2,{100,480,20,20},100);
        const auto owned=*death.effect_scope(1);
        require(death.apply_condition(owned,owned,Kind::poison,1000,5000)&&
            death.apply_health_regeneration(owned,1,*regeneration::reviewed(288,12),1000,16000,0),"self-death source setup");
        death.steer(1,1,0,1000);death.rotate(1,INFINITY,1,1000);box->drain();observer->drain();
        require(!death.kill({owned.instance,owned.actor,owned.generation+1},1100)&&!death.kill(owned,999),"self-death rejects stale lifetime and clock");
        require(death.resources(1)->health>0&&box->drain().empty(),"rejected death has no effects");
        require(death.kill(owned,1100)&&death.resources(1)->health==0,"authorized owned death commitszerohealth");
        const auto own=box->drain(),public_death=observer->drain();
        require(packet_count(own,67)==2&&packet_count(public_death,67)==2&&packet_count(public_death,40)>=1,"death clears both condition and regeneration and stops motion publicly");
        require(!death.has_condition(owned,Kind::poison,1100)&&!death.has_enchantment(owned,1,1100),"death cannot retain condition or regeneration");
        require(!death.kill(owned,1100)&&box->drain().empty(),"duplicate death rejected withoutduplicatepackets");
        const auto position=death.position(1,1100);const auto heading=death.heading(1,1100);
        death.steer(1,-1,0,1200);death.move(1,300,0,1200);death.rotate(1,-INFINITY,1,1200);death.tick(2100);
        require(death.position(1,2100)==position&&death.heading(1,2100)==heading&&death.resources(1)->health==0,"corpse cannot move turn or passivelyresurrect");
        box->drain();observer->drain();
        std::optional<fw::gw::chat::commands::External> recovery_request;
        fw::gw::chat::commands::Context recovery_context;recovery_context.external=&recovery_request;
        require(death.chat(1,u"/resurrect Observer",2100,recovery_context)&&recovery_request&&
            recovery_request->kind==u"resurrect"&&recovery_request->message==u"Observer","dead actor can route nativechat recovery to authorization boundary");
        require(death.chat(1,u"/.resurrect",2100,recovery_context)&&!recovery_request,"dotted resurrection is not a developmentcommand");
        box->drain();observer->drain();
        require(!death.resurrect({owned.instance,owned.actor,owned.generation+1},2100)&&!death.resurrect(owned,2099),"resurrection rejects stale lifetime and clock withoutmutation");
        require(box->drain().empty()&&death.resources(1)->health==0,"rejected resurrection remainsdead andsilent");
        require(death.resurrect(owned,2100),"authorized corpse recovery succeeds");
        require(death.resources(1)->health==480&&death.resources(1)->energy==20,"resurrection restores fullprivate resources");
        require(packet_count(observer->drain(),240)>=1&&packet_count(box->drain(),161)>=1,"resurrection publishes status and ownerresourcepipstate");
        require(!death.resurrect(owned,2100)&&box->drain().empty(),"alive recovery rejected idempotently");
        death.tick(5100);require(death.position(1,5100)==position&&death.heading(1,5100)==heading,"resurrection cannot resume old steering or turning");
        require(!death.has_condition(owned,Kind::poison,5100)&&!death.has_enchantment(owned,1,5100),"resurrection cannot carry old effects");
        death.steer(1,1,0,5100);death.tick(5200);require(death.position(1,5200)!=position,"fresh input moves resurrected actor");
        death.leave(1);death.join(1,"Replacement",0,0,box);death.set_vitals(1,{100,480,20,20},100);
        require(!death.resurrect(owned,5300),"resurrection cannot target recycledactor");
        require(!death.kill(owned,2200)&&death.resources(1)->health==100,"oldscope cannot kill recycledactor");
    }
    {
        using namespace fw::gw::skills;Instance revival;auto caster=std::make_shared<Outbox>(),target=std::make_shared<Outbox>(),stranger=std::make_shared<Outbox>();
        revival.join(1,"Monk",0,0,caster);revival.join(2,"Party",10,0,target);revival.join(3,"Stranger",20,0,stranger);
        for(unsigned id=1;id<=3;++id)revival.set_vitals(id,{480,480,40,40},100);
        require(revival.party_action(1,158,2,0)&&revival.party_action(2,154,1,0),"resurrection party setup");
        const auto source=*revival.effect_scope(1),dead=*revival.effect_scope(2),outside=*revival.effect_scope(3);
        require(revival.kill(dead,1000)&&revival.kill(outside,1000),"resurrection genuinecorpses");
        require(revival.resurrection_target_valid(source,2,1248)&&!revival.resurrection_target_valid(source,1,1248)&&!revival.resurrection_target_valid(source,3,1248),"resurrection onlydeadparty notselfstranger");
        auto forged=*effects::resurrection(305);forged.health_percent=100;
        require(!revival.resurrect_target(source,dead,forged,1000)&&!revival.resurrect_target(source,{dead.instance,dead.actor,dead.generation+1},*effects::resurrection(305),1000),"resurrection rejectsforgedplan andstale target");
        require(revival.resurrect_target(source,dead,*effects::resurrection(305),1000)&&revival.resources(2)->health==120&&revival.resources(2)->energy==0,"realspell restores25healthzeroenergy");
        require(!revival.resurrect_target(source,dead,*effects::resurrection(2),1000),"alive target cannotresurrectagain");
        require(revival.kill(dead,2000)&&revival.resurrect_target(source,dead,*effects::resurrection(2),2000)&&revival.resources(2)->health==480&&revival.resources(2)->energy==10,"realSignet restores100health25energy");
        revival.tick(5000);require(revival.resources(2)->energy==12,"revived energy beginsfromnewclock andfraction");
        require(revival.kill(dead,5000)&&revival.resurrect_target(source,dead,*effects::resurrection(314,12),5000)&&
            revival.resources(2)->health==269&&revival.resources(2)->energy==32,"RestoreLife touch56health80energy rank12");
        revival.set_vitals(2,{1,1,40,40},100);require(revival.kill(dead,6000),"onehealthcorpse setup");
        require(!revival.resurrect_target(source,dead,*effects::resurrection(314,10),6000)&&revival.resources(2)->health==0,"belowhalfpercent cannotinvent1health resurrection");
        require(revival.resurrect_target(source,dead,*effects::resurrection(314,11),6000)&&revival.resources(2)->health==1,"wiki55Monk requires11HealingPrayers at1maxhealth");
        revival.set_vitals(1,{120,480,40,40},100);revival.set_vitals(2,{480,480,40,40},100);
        require(revival.kill(dead,7000)&&revival.resurrect_target(source,dead,*effects::resurrection(1128,12),7000)&&
            revival.resources(2)->health==120&&revival.resources(2)->energy==12,"ResurrectionChant currentcasterhealth and29percentenergy");
        revival.set_vitals(1,{400,480,40,40},100);revival.set_vitals(2,{100,100,40,40},100);
        require(revival.kill(dead,8000)&&revival.resurrect_target(source,dead,*effects::resurrection(1128,12),8000)&&revival.resources(2)->health==100,"Chantcaps casterabsolutehealth at targetmaximum");
        revival.join(4,"Area",300,0,std::make_shared<Outbox>());revival.join(5,"Far",323,0,std::make_shared<Outbox>());
        for(unsigned id:{4u,5u}){revival.set_vitals(id,{480,480,40,40},100);require(revival.party_action(1,158,id,8000)&&revival.party_action(id,154,1,8000),"mass resurrection party setup");require(revival.kill(*revival.effect_scope(id),9000),"mass genuinecorpse");}
        require(revival.kill(dead,9000)&&revival.resurrect_party(source,*effects::party_resurrection(304),9000),"LightofDwayna atomically revives nearby deadparty");
        require(revival.resources(2)->health==25&&revival.resources(4)->health==120&&revival.resources(4)->energy==0&&revival.resources(5)->health==0&&revival.resources(3)->health==0,"Lightarea includes300 excludes323 andnonparty");
        auto altered=*effects::party_resurrection(304);altered.radius=10000;require(!revival.resurrect_party(source,altered,9000),"mass resurrection rejectsforgedradius");
        caster->drain();target->drain();
        require(revival.kill(dead,10000)&&revival.resurrect_target(source,dead,*effects::resurrection(1816,7),10000),"Sunspear signet revives genuine party corpse");
        require(revival.resources(2)->health==100&&revival.resources(2)->energy==28&&revival.position(2,10000)==revival.position(1,10000),"Sunspear rank seven restores full Health, 70percent Energy and relocates corpse to caster");
        require(packet_count(target->drain(),44)>=1&&packet_count(caster->drain(),44)>=1,"Sunspear relocation publishes native authoritative position to party");
        auto forged_sunspear=*effects::resurrection(1816,7);forged_sunspear.energy_percent=100;
        require(revival.kill(dead,11000)&&!revival.resurrect_target(source,dead,forged_sunspear,11000)&&revival.resources(2)->health==0,"forged Sunspear rank/energy plan cannot revive");
    }
    {
        using namespace fw::gw::skills;using fw::gw::combat::conditions::Kind;Instance wave;std::vector<std::shared_ptr<Outbox>> boxes;for(unsigned i=0;i<4;++i)boxes.push_back(std::make_shared<Outbox>());
        wave.join(1,"Earth Caster",0,0,boxes[0],0,{}, {},100,1);wave.join(2,"Adjacent",100,0,boxes[1],0,{}, {},100,2);wave.join(3,"Nearby",200,0,boxes[2],0,{}, {},100,2);wave.join(4,"Area",300,0,boxes[3],0,{}, {},100,2);for(unsigned id=1;id<=4;++id)wave.set_vitals(id,{480,480,20,20},100);
        const auto scope=*wave.effect_scope(1),close=*wave.effect_scope(2),near=*wave.effect_scope(3),far=*wave.effect_scope(4);const auto plan=*effects::caster_earth_aoe(937,12);
        require(!wave.shockwave(scope,plan,20,100)&&wave.resources(2)->health==480,"Shockwave cannot invent unknown foe armor");for(const auto target:{close,near,far})require(wave.combat_armor(target,80),"Shockwave reviewed armor supplied");
        require(wave.shockwave(scope,plan,20,100),"Shockwave actual3ring compound mutation");require(wave.resources(1)->health==480&&wave.resources(2)->health==357&&wave.resources(3)->health==408&&wave.resources(4)->health==444,"Shockwave sequentialarmor damage respects3distinctrings");
        require(wave.has_condition(close,Kind::blind,8099)&&wave.has_condition(near,Kind::cracked_armor,8099)&&!wave.has_condition(near,Kind::blind,100)&&wave.has_condition(far,Kind::weakness,8099)&&!wave.has_condition(far,Kind::cracked_armor,100),"Shockwave range-specific real conditions");
        wave.tick(8100);require(!wave.has_condition(close,Kind::blind,8100)&&!wave.has_condition(near,Kind::cracked_armor,8100),"Shockwave exactconditionexpiry");
        require(!wave.shockwave({scope.instance,scope.actor,scope.generation+1},plan,20,8100),"Shockwave stale caster denied");auto forged=plan;forged.strikes[2].earth_damage=900;require(!wave.shockwave(scope,forged,20,8100),"Shockwave forgedplan denied");wave.set_vitals(2,{1,480,20,20},100);require(wave.shockwave(scope,plan,20,8100)&&wave.resources(2)->health==0&&!wave.has_condition(close,Kind::weakness,8100),"Shockwave lethalstrike uses real death cleanup");
        Instance empty;auto out=std::make_shared<Outbox>();empty.join(1,"Solo",0,0,out);empty.set_vitals(1,{480,480,20,20},100);require(empty.shockwave(*empty.effect_scope(1),plan,20,0),"Shockwave nohostiletargets genuine success");
    }
    {
        using namespace fw::gw::skills;Instance area;auto a=std::make_shared<Outbox>(),b=std::make_shared<Outbox>(),c=std::make_shared<Outbox>();
        area.join(1,"Caster",0,0,a,0,{}, {},100,1);area.join(2,"Foe",240,0,b,0,{}, {},100,2);area.join(3,"Outside",246,0,c,0,{}, {},100,2);
        for(unsigned id=1;id<=3;++id)area.set_vitals(id,{480,480,20,20},100);
        const auto scope=*area.effect_scope(1);const auto plan=*effects::caster_earth_aoe(174,12);
        require(!area.shockwave(scope,plan,20,100)&&area.resources(2)->health==480,"Aftershock denies missing armor without damage");
        require(area.combat_armor(*area.effect_scope(2),60),"Aftershock explicit reviewed armor");
        require(area.shockwave(scope,plan,20,100)&&area.resources(2)->health==395&&area.resources(3)->health==480,"Aftershock ordinary nearby hit no invented knockdown bonus");
        auto forged=plan;forged.strikes[0].knocked_down_bonus=900;require(!area.shockwave(scope,forged,20,100),"Aftershock forged conditional bonus denied");
    }
    {
        using namespace fw::gw::skills;using fw::gw::combat::conditions::Kind;
        Instance missile;auto a=std::make_shared<Outbox>(),b=std::make_shared<Outbox>();
        missile.join(1,"Caster",0,0,a,0,{}, {},100,1);missile.join(2,"Foe",180,0,b,0,{}, {},100,2);
        missile.set_vitals(1,{480,480,0,40},100);missile.set_vitals(2,{480,480,20,20},100);
        const auto caster=*missile.effect_scope(1),target=*missile.effect_scope(2);missile.combat_armor(target,60);
        const auto hawk=*effects::earth_projectile(1374,12,12),stone=*effects::earth_projectile(171,12,12),glow=*effects::earth_projectile(1661,12,12);
        require(missile.projectile_target_valid(caster,2,1248,100)&&!missile.projectile_target_valid(caster,1,1248,100),"projectile validates owned lifetime hostiletarget");
        require(missile.launch_projectile(caster,2,hawk,20,100)&&missile.resources(2)->health==480,"earthprojectile launch delaysdamageuntilflight");
        missile.tick(199);require(missile.resources(2)->health==480,"projectile notinstant");missile.tick(300);
        require(missile.resources(2)->health==410&&missile.has_condition(target,Kind::weakness,300),"EbonHawk dealsEarth70 atAR60 andweakness onimpact");
        a->drain();b->drain();
        const auto glow_fx=fw::gw::combat::detail::integer(21,2,1632);
        require(missile.launch_projectile(caster,2,glow,20,300),"Glowstone launches");
        require(packet_count(a->drain(),21)==0&&packet_count(b->drain(),21)==0,"Glowstone target particle waits for impact");
        missile.tick(400);require(missile.resources(2)->health==369&&missile.resources(1)->energy==11,"Glowstone refunds11 atES12 onlyweaknessimpact");
        const auto glow_caster=a->drain(),glow_target=b->drain();
        require(std::count(glow_caster.begin(),glow_caster.end(),glow_fx)==1&&std::count(glow_target.begin(),glow_target.end(),glow_fx)==1,
            "Glowstone native target1632 exactly once to caster and victim on hit");
        require(missile.launch_projectile(caster,2,stone,20,400),"Stoning launches");missile.tick(500);require(missile.resources(2)->health==276&&missile.knocked_down(target,500),"Stoning weakfoeEarth93 plus2secondknockdown");
        const auto stuck=missile.position(2,500);missile.steer(2,1,0,600);missile.move(2,300,0,600);missile.tick(1000);require(missile.position(2,1000)==stuck,"knockeddownmovementdenied");
        require(missile.launch_projectile(caster,2,stone,20,1000),"repeatStoning launches");missile.tick(1100);require(missile.knocked_down(target,2499)&&!missile.knocked_down(target,2500),"knockdown cannotextend whilealreadydown");
        a->drain();b->drain();
        // At180units the100ms flight permits only28.5units of post-release
        // movement,inside the32unit private hit radius. Use360units/200ms so
        // this tests a real dodge at impact,not movement accrued until a late tick.
        require(missile.launch_projectile(caster,2,glow,20,2600),"projectile beforetargetleave");missile.leave(2);missile.join(2,"Replacement",360,0,b,0,{}, {},100,2);missile.set_vitals(2,{480,480,20,20},100);missile.combat_armor(*missile.effect_scope(2),60);missile.tick(2700);require(missile.resources(2)->health==480,"projectile preserves targetlifetime acrossslotreuse");
        require(packet_count(a->drain(),21)==0&&packet_count(b->drain(),21)==0,"Recycled projectile target emits no body particle");
        require(missile.launch_projectile(caster,2,glow,20,2800),"dodgeprojectile launches");missile.steer(2,0,1,2801);missile.tick(3200);require(missile.resources(2)->health==480&&!missile.has_condition(*missile.effect_scope(2),Kind::weakness,3200),"changingdirection afterrelease dodgesfixedprojectile");
        require(packet_count(a->drain(),21)==0&&packet_count(b->drain(),21)==0,"Dodged Glowstone emits no body particle");
    }
    {
        using namespace fw::gw::skills;using fw::gw::combat::conditions::Kind;
        Instance lightning;std::vector<std::shared_ptr<Outbox>> boxes;for(unsigned i=0;i<5;++i)boxes.push_back(std::make_shared<Outbox>());
        lightning.join(1,"Air",0,0,boxes[0],0,{}, {},100,1);lightning.join(2,"Touch",100,0,boxes[1],0,{}, {},100,2);
        lightning.join(3,"Adjacent",150,0,boxes[2],0,{}, {},100,2);lightning.join(4,"Far",267,0,boxes[3],0,{}, {},100,2);lightning.join(5,"Ally",140,0,boxes[4],0,{}, {},100,1);
        for(unsigned id=1;id<=5;++id)lightning.set_vitals(id,{480,480,40,40},100);
        const auto caster=*lightning.effect_scope(1),touch=*lightning.effect_scope(2),far=*lightning.effect_scope(4);
        require(!lightning.lightning(caster,touch,*effects::lightning(232,12),20,100),"lightning cannot invent unknown lightningarmor");
        for(unsigned id=2;id<=4;++id)require(lightning.lightning_armor(*lightning.effect_scope(id),80),"explicit lightningarmor");
        require(lightning.lightning(caster,touch,*effects::lightning(232,12),20,100),"LightningTouch actual targetcentered adjacent damagecontrol");
        require(lightning.resources(2)->health==430&&lightning.resources(3)->health==430&&lightning.resources(4)->health==480&&lightning.resources(5)->health==480,"Touch25percentpenetration andadjacentfoes only");
        require(lightning.has_condition(touch,Kind::blind,3099)&&lightning.has_condition(touch,Kind::cracked_armor,8099),"Touch timedblind/crackedarmor");
        require(lightning.lightning(caster,far,*effects::lightning(224,12),20,1000)&&lightning.resources(4)->health==435&&lightning.has_condition(far,Kind::weakness,17999),"Charge45damage and17secondWeakness");
        require(lightning.lightning(caster,far,*effects::lightning(229,12),20,1000)&&lightning.resources(4)->health==435,"Orb delaysdamage untilprojectilearrival");
        lightning.tick(1001);require(lightning.resources(4)->health==435,"Orb cannotimpact early");lightning.tick(2000);require(lightning.resources(4)->health==353,"Orb impacts with actual82damage");
        require(lightning.lightning(caster,far,*effects::lightning(229,12),20,2100),"Orb dodge launch");lightning.steer(4,0,1,2101);lightning.tick(2500);require(lightning.resources(4)->health==353,"Orb changingdirectiondodges fixedaim");lightning.steer(4,0,0,2500);
        auto stale=far;++stale.generation;require(!lightning.lightning(caster,stale,*effects::lightning(224,12),20,2500),"Lightning rejectsrecycled targetscope");
        lightning.set_vitals(4,{480,480,40,40},100);for(auto& box:boxes)box->drain();
        require(lightning.lightning(caster,far,*effects::lightning(865,12),20,3000)&&lightning.resources(4)->health==398,"Hammer deals82 immediately using armor before its CrackedArmor");
        require(lightning.has_condition(far,Kind::cracked_armor,19999)&&!lightning.has_condition(far,Kind::cracked_armor,20000),"Hammer17second CrackedArmor expires precisely");
        // Targeted particle carries victim and source, unlike self-body kind21.
        const fw::gw::combat::Packet hammer_fx{159,0,20,0,0,0,4,0,0,0,1,0,0,0,223,3,0,0};
        for(auto& box:boxes){const auto packets=box->drain();require(std::count(packets.begin(),packets.end(),hammer_fx)==1,"Hammer native victim991 sent once to every peer after effect commit");}
    }
    {
        namespace effects=fw::gw::skills::effects;using Kind=fw::gw::combat::conditions::Kind;
        Instance drain;std::vector<std::shared_ptr<Outbox>> boxes;for(unsigned i=0;i<5;++i)boxes.push_back(std::make_shared<Outbox>());
        drain.join(1,"Mesmer",0,0,boxes[0],0,{}, {},100,1);drain.join(2,"Target",100,0,boxes[1],0,{}, {},100,2);drain.join(3,"Nearby",150,0,boxes[2],0,{}, {},100,2);drain.join(4,"Far",353,0,boxes[3],0,{}, {},100,2);drain.join(5,"Ally",140,0,boxes[4],0,{}, {},100,1);
        for(unsigned id=1;id<=5;++id)drain.set_vitals(id,{480,480,40,40},100);drain.set_vitals(1,{480,480,10,40},100);drain.set_vitals(2,{480,480,3,40},100);const auto caster=*drain.effect_scope(1),target=*drain.effect_scope(2),near=*drain.effect_scope(3);
        require(drain.energy_drain(caster,target,*effects::energy_drain(39,12),20,100),"Surge actualenergybased armorignored area damage");require(drain.resources(2)->energy==0&&drain.resources(2)->health==453&&drain.resources(3)->energy==40&&drain.resources(3)->health==453&&drain.resources(4)->health==480&&drain.resources(5)->health==480,"Surge27 based on onlythree primaryenergy and nearbyfoes");
        require(drain.energy_drain(caster,target,*effects::energy_drain(42,12),20,100)&&drain.resources(2)->health==453,"Emptytarget Burn loseszeroenergy andnodamage");
        drain.set_vitals(2,{480,480,40,40},100);require(drain.energy_drain(caster,target,*effects::energy_drain(42,12),20,100)&&drain.resources(2)->health==408&&drain.resources(2)->energy==32&&drain.resources(3)->health==453,"Burn72 onlytarget");
        drain.set_vitals(3,{480,480,2,40},100);require(drain.energy_drain(caster,target,*effects::energy_drain(59,12),20,100)&&drain.resources(2)->energy==25&&drain.resources(3)->energy==0&&drain.has_condition(target,Kind::weakness,10099)&&drain.has_condition(near,Kind::weakness,10099)&&!drain.has_condition(*drain.effect_scope(4),Kind::weakness,100),"Weariness independentlycapped area denial andWeakness10s");
        drain.set_vitals(2,{480,480,3,40},100);require(drain.energy_drain(caster,target,*effects::energy_drain(80,12),20,100)&&drain.resources(2)->energy==0&&drain.resources(1)->energy==16,"Tap returnsdoubleactuallylost3");
        require(drain.energy_drain(caster,target,*effects::energy_drain(79,12),20,100)&&drain.resources(1)->energy==16,"Drainzeroenergy returnszero");drain.set_vitals(2,{480,480,40,40},100);require(drain.energy_drain(caster,target,*effects::energy_drain(79,12),20,100)&&drain.resources(1)->energy==40&&drain.resources(2)->energy==32,"Draintriple8 cappedownermax");
        auto stale=target;++stale.generation;require(!drain.energy_drain(caster,stale,*effects::energy_drain(39,12),20,100)&&!drain.energy_drain(caster,target,*effects::energy_drain(39,12),20,99),"Drain staleScope/backwardsclock rejected");
        drain.set_vitals(1,{100,480,10,40},100);drain.set_vitals(2,{480,480,2,40},100);
        require(drain.energy_drain(caster,target,*effects::energy_drain(40,12),20,100)&&drain.resources(1)->health==212&&drain.resources(1)->energy==10&&drain.resources(2)->energy==0,"Ether Feast heals only for energy actually lost");
        require(drain.energy_drain(caster,target,*effects::energy_drain(40,12),20,100)&&drain.resources(1)->health==212,"Ether Feast healed against empty energy");
        drain.set_vitals(1,{450,480,10,40},100);drain.set_vitals(2,{480,480,3,40},100);
        require(drain.energy_drain(caster,target,*effects::energy_drain(40,12),20,100)&&drain.resources(1)->health==480&&drain.resources(2)->energy==0,"Ether Feast exceeded caster maximum health");
        drain.set_vitals(2,{5,480,2,40},100);require(drain.energy_drain(caster,target,*effects::energy_drain(42,12),20,100)&&drain.resources(2)->health==0,"EnergyBurn lethalowneddeath");
    }
    {
        namespace effects=fw::gw::skills::effects;using Kind=fw::gw::combat::conditions::Kind;
        // The documented 240-unit Rodgort exception is relative to its target,
        // not to the caster. A foe in the old 240..252 annulus stays untouched.
        Instance boundary;auto boundary_box=std::make_shared<Outbox>();
        boundary.join(1,"Caster",0,0,boundary_box,0,{}, {},100,1);
        boundary.join(2,"Target",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        boundary.join(3,"Edge",340,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        boundary.join(4,"Outside",346,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        for(unsigned id=1;id<=4;++id){boundary.set_vitals(id,{480,480,40,60},100);boundary.elemental_armor(*boundary.effect_scope(id),60,60);}
        require(boundary.elemental(*boundary.effect_scope(1),*boundary.effect_scope(2),*effects::elemental(189,12),20,100),"Rodgort boundary effect commits");
        require(boundary.resources(3)->health==381&&boundary.has_condition(*boundary.effect_scope(3),Kind::burning,3099),"Rodgort exact240 receives damage and Burning");
        require(boundary.resources(4)->health==480&&!boundary.has_condition(*boundary.effect_scope(4),Kind::burning,100),"Rodgort246 receives neither damage nor Burning");
        const auto dead_center=*boundary.effect_scope(2);
        require(boundary.kill(dead_center,100),"Rodgort target dies before completion");
        require(!boundary.elemental(*boundary.effect_scope(1),dead_center,*effects::elemental(191,12),20,100),"Immolate still requires a living target");
        require(boundary.elemental(*boundary.effect_scope(1),dead_center,*effects::elemental(189,12),20,100)&&boundary.resources(2)->health==0&&boundary.resources(3)->health==282&&boundary.resources(4)->health==480,"Rodgort retains corpse center and damages only surviving nearby foes");
        Instance fire;std::vector<std::shared_ptr<Outbox>> boxes;for(unsigned i=0;i<5;++i)boxes.push_back(std::make_shared<Outbox>());
        fire.join(1,"Elementalist",0,0,boxes[0],0,{}, {},100,1);fire.join(2,"Target",100,0,boxes[1],0,{}, {},100,2);fire.join(3,"Adjacent",150,0,boxes[2],0,{}, {},100,2);fire.join(4,"Outer",267,0,boxes[3],0,{}, {},100,2);fire.join(5,"Ally",140,0,boxes[4],0,{}, {},100,1);
        for(unsigned id=1;id<=5;++id)fire.set_vitals(id,{480,480,40,60},100);const auto caster=*fire.effect_scope(1),target=*fire.effect_scope(2);
        for(auto& box:boxes)box->drain();
        require(!fire.elemental(caster,target,*effects::elemental(191,12),20,100),"Fire damage refuses absent reviewed armor");
        const auto rejected_fx=boxes[0]->drain();const auto immolate_fx=fw::gw::combat::detail::integer(21,2,339);
        require(std::find(rejected_fx.begin(),rejected_fx.end(),immolate_fx)==rejected_fx.end(),"rejected Immolate cannot show victim effect");
        for(unsigned id=2;id<=4;++id)require(fire.elemental_armor(*fire.effect_scope(id),60,60),"Owned explicit fire/cold armor");
        require(fire.elemental(caster,target,*effects::elemental(191,12),20,100)&&fire.resources(2)->health==416&&fire.has_condition(target,Kind::burning,3099),"Immolate64fire and Burning3s");
        for(auto& box:boxes){const auto packets=box->drain();require(std::count(packets.begin(),packets.end(),immolate_fx)==1,"native Immolate victim resource published once to owner and peers after effect");}
        require(fire.elemental(caster,target,*effects::elemental(1379,12,12),20,100)&&fire.resources(2)->health==375&&fire.resources(1)->energy==51,"GlowingGaze41 and conditional11energy");
        require(fire.elemental(caster,target,*effects::elemental(846,12),20,100)&&fire.resources(2)->health==323&&fire.has_condition(target,Kind::blind,9099),"Steam52cold and burning conditional9sBlind");
        for(unsigned id=2;id<=4;++id)fire.set_vitals(id,{480,480,40,60},100);
        require(fire.elemental(caster,target,*effects::elemental(89,12),20,100)&&fire.resources(2)->health==398,"DeathlyChill41cold plus41armorignoredShadow");
        for(unsigned id=2;id<=4;++id)fire.set_vitals(id,{480,480,40,60},100);
        require(fire.elemental(caster,caster,*effects::elemental(183,12),20,100)&&fire.resources(2)->health==366&&fire.resources(3)->health==366&&fire.resources(4)->health==480&&fire.resources(5)->health==480,"Inferno caster-centered adjacent114 only foes");
        for(unsigned id=2;id<=4;++id)fire.set_vitals(id,{480,480,40,60},100);
        require(fire.elemental(caster,target,*effects::elemental(189,12),20,100)&&fire.resources(2)->health==381&&fire.resources(3)->health==381&&fire.resources(4)->health==381,"Rodgort nearby99 and burn all hit foes");
        for(unsigned id=2;id<=4;++id)fire.set_vitals(id,{480,480,40,60},100);
        require(fire.elemental(caster,target,*effects::elemental(1095,12),20,100)&&fire.resources(2)->health==389&&fire.resources(3)->health==389&&fire.resources(4)->health==389&&fire.resources(1)->energy==57,"StarBurst3areafoes91 each refund6energy");
        auto stale=target;++stale.generation;require(!fire.elemental(caster,stale,*effects::elemental(191,12),20,100),"Fire/cold lifetime rejection");
        auto forged=*effects::elemental(189);forged.radius=9999;require(!fire.elemental(caster,target,forged,20,100),"Elemental forged radius rejected");
    }
    {
        namespace effects=fw::gw::skills::effects;Instance ball;auto a=std::make_shared<Outbox>(),b=std::make_shared<Outbox>(),c=std::make_shared<Outbox>();ball.join(1,"Caster",0,0,a,0,{}, {},100,1);ball.join(2,"Target",400,0,b,0,{}, {},100,2);ball.join(3,"Adjacent",450,0,c,0,{}, {},100,2);
        for(unsigned id=1;id<=3;++id)ball.set_vitals(id,{480,480,40,40},100);for(unsigned id=2;id<=3;++id)ball.elemental_armor(*ball.effect_scope(id),60,60);const auto caster=*ball.effect_scope(1),target=*ball.effect_scope(2);
        require(ball.elemental(caster,target,*effects::elemental(186,12),20,100),"Fireball actual projectile launch");require(ball.resources(2)->health==480,"Fireball waits flight");ball.tick(101);require(ball.resources(2)->health==480,"Fireball earlytick no impact");ball.tick(1000);require(ball.resources(2)->health==389&&ball.resources(3)->health==389,"Fireball delayed91 adjacent explosion");
        require(ball.elemental(caster,target,*effects::elemental(186,12),20,1100),"Fireball dodge launch");ball.steer(2,0,1,1101);ball.tick(1600);require(ball.resources(2)->health==389&&ball.resources(3)->health==389,"Dodge cancels Fireball explosion");
    }
    {
        namespace effects=fw::gw::skills::effects;
        Instance steal;auto a=std::make_shared<Outbox>(),b=std::make_shared<Outbox>();steal.join(1,"Vampire",0,0,a,0,{}, {},100,1);steal.join(2,"Victim",100,0,b,0,{}, {},100,2);steal.set_vitals(1,{100,480,40,40},100);steal.set_vitals(2,{480,480,40,40},100);
        const auto caster=*steal.effect_scope(1),foe=*steal.effect_scope(2);a->drain();b->drain();
        require(steal.life_steal(caster,foe,*effects::life_steal(153,12),100),"Gaze actual health transfer without invented armor");require(steal.resources(1)->health==152&&steal.resources(2)->health==428,"Gaze52 life moved");
        auto packets=a->drain();require(packet_count(packets,162)==2&&packet_count(packets,161)==2,"Life transfer paired signed changes and final health");
        const auto gaze_fx=*fw::gw::skills::particles::targeted_body_2022(2,1,153);require(std::count(packets.begin(),packets.end(),gaze_fx)==1,"Vampiric Gaze actual victim resource once to caster");
        const auto victim_packets=b->drain();require(std::count(victim_packets.begin(),victim_packets.end(),gaze_fx)==1,"Vampiric Gaze victim resource once to peer");
        auto stale=foe;++stale.generation;require(!steal.life_steal(caster,stale,*effects::life_steal(153,12),200),"Life transfer rejects replaced target");
        require(a->drain().empty()&&b->drain().empty(),"stale life-steal cannot publish a victim particle");
        steal.set_vitals(2,{30,480,40,40},100);require(steal.life_steal(caster,foe,*effects::life_steal(1077,12),200),"Bite lethal transfer");require(steal.resources(1)->health==183&&steal.resources(2)->health==0&&packet_count(a->drain(),240)>=1,"Lethal steal remaining health plus1 then death cleanup");
    }
    {
        namespace effects=fw::gw::skills::effects;using Kind=fw::gw::combat::conditions::Kind;
        Instance spells;std::vector<std::shared_ptr<Outbox>> boxes;for(unsigned i=0;i<5;++i)boxes.push_back(std::make_shared<Outbox>());
        spells.join(1,"Caster",0,0,boxes[0],0,{}, {},100,1);spells.join(2,"Source",100,0,boxes[1],0,{}, {},100,2);spells.join(3,"Adjacent",150,0,boxes[2],0,{}, {},100,2);spells.join(4,"Far",267,0,boxes[3],0,{}, {},100,2);spells.join(5,"Ally",140,0,boxes[4],0,{}, {},100,1);
        for(unsigned id=1;id<=5;++id)spells.set_vitals(id,{480,480,40,40},100);const auto caster=*spells.effect_scope(1),target=*spells.effect_scope(2),near=*spells.effect_scope(3);
        for(auto& box:boxes)box->drain();
        require(spells.condition_spell(caster,target,*effects::condition_spell(117,12),100),"Enfeeble real Weakness");require(spells.has_condition(target,Kind::weakness,26099)&&!spells.has_condition(near,Kind::weakness,100),"Enfeeble single foe26seconds");
        const auto enfeeble_fx=fw::gw::combat::detail::integer(21,2,220);
        for(auto& box:boxes){const auto packets=box->drain();require(std::count(packets.begin(),packets.end(),enfeeble_fx)==1,
            "Enfeeble native target resource once to every observer");}
        require(spells.apply_condition(caster,target,Kind::blind,200,10000),"Epidemic source blind");for(auto& box:boxes)box->drain();
        require(spells.condition_spell(caster,target,*effects::condition_spell(78),1000),"Epidemic actual spread");
        require(spells.has_condition(target,Kind::blind,10199)&&spells.has_condition(near,Kind::blind,10199)&&spells.has_condition(near,Kind::weakness,26099)&&!spells.has_condition(*spells.effect_scope(4),Kind::blind,1000)&&!spells.has_condition(*spells.effect_scope(5),Kind::blind,1000),"Epidemic copies remaining durations only adjacent foes retaining source");
        auto packets=boxes[0]->drain();require(packet_count(packets,65)==2&&packet_count(packets,67)==0&&packet_count(packets,162)==0,"Spread exact effect publication no source removal or damage");
        require(packet_count(packets,21)==0,"Epidemic native sentinel cannot invent a target particle");
        for(auto& box:boxes)box->drain();
        auto stale=target;++stale.generation;require(!spells.condition_spell(caster,stale,*effects::condition_spell(78),1100),"Spread rejects recycled target");
        require(!spells.condition_spell(caster,stale,*effects::condition_spell(117,12),1100),"Enfeeble rejects recycled target");
        for(auto& box:boxes)require(box->drain().empty(),"Rejected condition casts publish no effects");
    }
    {
        Instance clocks;auto box=std::make_shared<Outbox>();clocks.join(1,"Clock",0,0,box,0,{}, {},1000,1);clocks.set_vitals(1,{100,480,0,60},100);const auto scope=*clocks.effect_scope(1);
        require(!clocks.health_sacrifice(scope,118,999)&&clocks.resources(1)->health==100,"Pre-entry mutation rejected without first tick");
        clocks.tick(2500);require(clocks.resources(1)->energy==1,"First post-entry tick integrates only elapsed1500ms");clocks.tick(1500);clocks.tick(4000);require(clocks.resources(1)->energy==2,"Old tick cannot rewind energy cursor or duplicate regeneration");
        require(clocks.health_sacrifice(scope,118,4000).has_value(),"Actual post-entry mutation accepted");
    }
    for(unsigned skill:{1691u,311u,295u}){
        namespace effects=fw::gw::skills::effects;using Kind=fw::gw::combat::conditions::Kind;
        Instance clean;auto a=std::make_shared<Outbox>(),b=std::make_shared<Outbox>();clean.join(1,"Monk",0,0,a,0,{}, {},100,1);clean.public_professions(1,3,0);clean.join(2,"Ally",100,0,b,0,{}, {},100,1);clean.set_vitals(1,{100,480,20,40},100);clean.set_vitals(2,{100,480,20,40},100);require(clean.party_action(1,158,2,100)&&clean.party_action(2,154,1,100),"Cleansing owned party");const auto caster=*clean.effect_scope(1),target=*clean.effect_scope(2);
        require(clean.apply_condition(caster,target,Kind::bleeding,100,10000)&&clean.apply_condition(caster,target,Kind::blind,100,5000),"Cleansing real conditions");
        if(skill==1691){const auto plan=*effects::condition_removal(skill,12,12);require(clean.status_mutation(target,169,1,12,100,55100,true),"Dismiss owned enchantment");require(clean.remove_conditions(caster,2,plan,100)&&clean.resources(2)->health==201&&!clean.has_condition(target,Kind::blind,100)&&clean.has_condition(target,Kind::bleeding,100),"Dismiss removes newest and heals enchanted target");require(clean.remove_conditions(caster,2,plan,100)&&clean.remove_conditions(caster,2,plan,100)&&clean.resources(2)->health==403,"Dismiss heals even when no condition removed");}
        else if(skill==311){require(clean.ally_cleansing(caster,target,*effects::ally_cleansing(skill,12,12),100)&&!clean.has_condition(target,Kind::bleeding,100)&&clean.has_condition(caster,Kind::bleeding,100)&&clean.has_condition(caster,Kind::blind,5099)&&clean.resources(1)->health==144&&clean.resources(2)->health==138,"Draw real remaining conditions, caster health gain and target DF bonus");require(!clean.ally_cleansing(caster,caster,*effects::ally_cleansing(skill,12,12),100),"Draw excludes self");}
        else{require(clean.status_mutation(target,101,2,12,100,50100,true),"Purge owned hex");require(clean.ally_cleansing(caster,target,*effects::ally_cleansing(skill),100)&&!clean.has_condition(target,Kind::bleeding,100)&&!clean.has_condition(target,Kind::blind,100)&&clean.status_snapshot(target,100)->empty()&&clean.resources(1)->energy==0&&clean.resources(2)->health==100,"Purge removes all conditions/hexes despite insufficient energy, no signet DF");}
        auto stale=target;++stale.generation;if(skill!=1691)require(!clean.ally_cleansing(caster,stale,*effects::ally_cleansing(skill,12,12),100),"Cleansing lifetime guards");
    }
    for(unsigned skill:{279u,1117u,280u,1119u}){
        namespace effects=fw::gw::skills::effects;const auto plan=*effects::party_heal(skill,12,12);
        Instance area;std::vector<std::shared_ptr<Outbox>> boxes;for(unsigned i=0;i<5;++i)boxes.push_back(std::make_shared<Outbox>());
        area.join(1,"Monk",0,0,boxes[0],0,{}, {},100,1);area.public_professions(1,3,0);area.join(2,"Boundary",plan.range,0,boxes[1],0,{}, {},100,1);area.join(3,"Outside",plan.range+1,0,boxes[2],0,{}, {},100,1);area.join(4,"Foe",10,0,boxes[3],0,{}, {},100,2);area.join(5,"Dead",10,0,boxes[4],0,{}, {},100,1);
        for(unsigned actor=1;actor<=5;++actor)area.set_vitals(actor,{actor==5?0u:100u,480,40,40},100);for(unsigned actor:{2u,3u,5u})require(area.party_action(1,158,actor,100)&&area.party_action(actor,154,1,100),"Area party fixture");
        const auto scope=*area.effect_scope(1);require(area.party_heal(scope,plan,100),"Reviewed area/earshot actual heal");require(area.resources(1)->health==100+plan.base_heal+38&&area.resources(2)->health==100+plan.base_heal&&area.resources(3)->health==100&&area.resources(4)->health==(plan.all_creatures?100+plan.base_heal:100u)&&area.resources(5)->health==0,"Area boundary, foes only for creature heal, caster DF, no resurrection");
        auto forged=plan;forged.all_creatures=!forged.all_creatures;require(!area.party_heal(scope,forged,100),"Cannot forge recipient allegiance policy");
    }
    for(unsigned skill:{158u,1078u,131u}){
        namespace effects=fw::gw::skills::effects;using Kind=fw::gw::combat::conditions::Kind;
        Instance touch;std::vector<std::shared_ptr<Outbox>> boxes;for(unsigned i=0;i<5;++i)boxes.push_back(std::make_shared<Outbox>());
        touch.join(1,"Caster",0,0,boxes[0],0,{}, {},100,1);touch.join(2,"Target",144,0,boxes[1],0,{}, {},100,2);touch.join(3,"Boundary",310,0,boxes[2],0,{}, {},100,2);touch.join(4,"Beyond",311,0,boxes[3],0,{}, {},100,2);touch.join(5,"Ally",144,0,boxes[4],0,{}, {},100,1);
        for(unsigned actor=1;actor<=5;++actor)touch.set_vitals(actor,{480,480,40,40},100);const auto a=*touch.effect_scope(1),b=*touch.effect_scope(2);
        if(skill==131){require(touch.condition_spell(a,b,*effects::condition_spell(skill,12),100),"Barbed real area Bleeding");require(touch.has_condition(b,Kind::bleeding,13099)&&touch.has_condition(*touch.effect_scope(3),Kind::bleeding,100)&&!touch.has_condition(*touch.effect_scope(4),Kind::bleeding,100)&&!touch.has_condition(*touch.effect_scope(5),Kind::bleeding,100),"Barbed exact166 boundary and hostile targets");touch.tick(1100);require(touch.resources(2)->health==474,"Barbed actual three degeneration pips");}
        else{require(touch.shadow_damage(a,b,*effects::shadow_damage(skill,12),20,100)&&touch.resources(2)->health==430,"Touch actual144range shadow50");touch.move(2,145,0,100,1);auto forged=*effects::shadow_damage(skill,12);forged.range=145;require(!touch.shadow_damage(a,b,forged,20,100),"Touch cannot extend reviewed range");}
        const auto loss=touch.health_sacrifice(a,skill,skill==131?1100:100);require(loss&&std::abs(loss->lost-(skill==131?38.4:48.))<0.001,"Touch/signet native percent sacrifice");
    }
    for(unsigned skill:{133u,102u,1067u}){
        namespace effects=fw::gw::skills::effects;
        Instance shadow;auto source=std::make_shared<Outbox>(),target=std::make_shared<Outbox>();shadow.join(1,"Caster",0,0,source,0,{}, {},100,1);shadow.join(2,"Foe",100,0,target,0,{}, {},100,2);
        shadow.set_vitals(1,{100,480,60,60},100);shadow.set_vitals(2,{480,480,40,40},100);shadow.tick(100);source->drain();target->drain();const auto a=*shadow.effect_scope(1),b=*shadow.effect_scope(2);const auto plan=*effects::shadow_damage(skill,12);
        require(shadow.shadow_damage(a,b,plan,20,100),"Shadow works without fabricated armor");require(shadow.resources(2)->health==(skill==133?440u:398u)&&shadow.resources(1)->health==(skill==133?100u:141u),"Actual shadow damage plus conditional health stealing");
        auto packets=source->drain();require(packet_count(packets,162)==(skill==133?1u:3u),"Native shadow delta and independent transfer publication");
        shadow.set_vitals(1,{100,480,60,60},100);shadow.set_vitals(2,{240,480,40,40},100);require(shadow.shadow_damage(a,b,plan,20,100)&&shadow.resources(1)->health==100,"Exactly half target never steals");
        auto stale=b;++stale.generation;require(!shadow.shadow_damage(a,stale,plan,20,100),"Shadow stale target fails");
        auto forged=plan;forged.damage=999;require(!shadow.shadow_damage(a,b,forged,20,100),"Forged shadow metadata fails");
    }
    for(unsigned skill:{118u,2885u}){
        namespace effects=fw::gw::skills::effects;using Kind=fw::gw::combat::conditions::Kind;
        Instance area;std::vector<std::shared_ptr<Outbox>> boxes;for(unsigned i=0;i<5;++i)boxes.push_back(std::make_shared<Outbox>());
        area.join(1,"Caster",0,0,boxes[0],0,{}, {},100,1);area.join(2,"Target",100,0,boxes[1],0,{}, {},100,2);area.join(3,"Boundary",352,0,boxes[2],0,{}, {},100,2);area.join(4,"Beyond",353,0,boxes[3],0,{}, {},100,2);area.join(5,"Ally",100,0,boxes[4],0,{}, {},100,1);
        for(unsigned actor=1;actor<=5;++actor)area.set_vitals(actor,{480,480,40,40},100);
        area.tick(100);const auto caster=*area.effect_scope(1),target=*area.effect_scope(2);const auto plan=*effects::condition_spell(skill,12);
        require(area.condition_spell(caster,target,plan,100),"Blood complete area condition");
        require(area.has_condition(target,Kind::weakness,100+plan.duration-1)&&area.has_condition(*area.effect_scope(3),Kind::weakness,100)&&!area.has_condition(*area.effect_scope(4),Kind::weakness,100)&&!area.has_condition(*area.effect_scope(5),Kind::weakness,100),"Blood exact nearby boundary and allegiance");
        require(!area.has_condition(target,Kind::weakness,100+plan.duration),"Blood PvE/PvP exact expiry");
        const auto loss=area.health_sacrifice(caster,skill,100);require(loss&&std::abs(loss->lost-(skill==118?48.:81.6))<0.001&&area.resources(1)->health==(skill==118?432u:399u),"Blood pinned PvE10/PvP17 percent max-health sacrifice");
    }
    {
        namespace effects=fw::gw::skills::effects;using Kind=fw::gw::combat::conditions::Kind;
        Instance transfer;auto a=std::make_shared<Outbox>(),b=std::make_shared<Outbox>();transfer.join(1,"Necromancer",0,0,a,0,{}, {},100,1);transfer.join(2,"Foe",100,0,b,0,{}, {},100,2);
        transfer.set_vitals(1,{480,480,40,40},100);transfer.set_vitals(2,{480,480,40,40},100);const auto caster=*transfer.effect_scope(1),foe=*transfer.effect_scope(2);
        require(transfer.apply_condition(foe,caster,Kind::blind,100,10000)&&transfer.apply_condition(foe,caster,Kind::weakness,100,20000),"Transfer source setup");a->drain();b->drain();
        require(transfer.condition_transfer(caster,foe,*effects::condition_transfer(154,0),200),"Touch actual newest transfer");
        require(!transfer.has_condition(caster,Kind::weakness,200)&&transfer.has_condition(caster,Kind::blind,200)&&transfer.has_condition(foe,Kind::weakness,20099)&&!transfer.has_condition(foe,Kind::weakness,20100),"Transfer removal and remaining duration");
        const auto packets=b->drain();require(packet_count(packets,65)==1&&packet_count(packets,67)==1&&packet_count(packets,162)==0,"Transfer native effect add/remove no damage");
        const auto transfer_fx=*fw::gw::skills::particles::targeted_body_2022(2,1,154);
        require(std::count(packets.begin(),packets.end(),transfer_fx)==1,"Plague Touch native victim particle once");
        const auto caster_packets=a->drain();require(std::count(caster_packets.begin(),caster_packets.end(),transfer_fx)==1,
            "Plague Touch victim particle also reaches caster");
        auto stale=foe;++stale.generation;require(!transfer.condition_transfer(caster,stale,*effects::condition_transfer(154),300),"Transfer rejects recycled foe");
        require(a->drain().empty()&&b->drain().empty(),"Rejected transfer publishes no victim effects");
        require(transfer.condition_transfer(caster,foe,*effects::condition_transfer(154,12),300)&&!transfer.has_condition(caster,Kind::blind,300)&&transfer.has_condition(foe,Kind::blind,9999),"Touch removes remaining condition");
    }
    {
        namespace effects=fw::gw::skills::effects;using Kind=fw::gw::combat::conditions::Kind;
        Instance control;std::vector<std::shared_ptr<Outbox>> boxes;for(unsigned i=0;i<5;++i)boxes.push_back(std::make_shared<Outbox>());
        control.join(1,"Ranger",0,0,boxes[0],0,{}, {},100,1);control.join(2,"Target",100,0,boxes[1],0,{}, {},100,2);
        control.join(3,"Adjacent",150,0,boxes[2],0,{}, {},100,2);control.join(4,"Far",267,0,boxes[3],0,{}, {},100,2);control.join(5,"Ally",140,0,boxes[4],0,{}, {},100,1);
        for(unsigned id=1;id<=5;++id)control.set_vitals(id,{480,480,40,40},100);
        const auto caster=*control.effect_scope(1),target=*control.effect_scope(2);
        require(control.blindness(caster,target,*effects::blindness(220,12),100),"Flash condition requires no fabricated armor");
        require(control.has_condition(target,Kind::blind,7099)&&!control.has_condition(*control.effect_scope(3),Kind::blind,100),"Flash single foe duration");
        require(control.blindness(caster,target,*effects::blindness(424,12),200),"Dirt touch control");
        require(control.has_condition(target,Kind::blind,13199)&&control.has_condition(*control.effect_scope(3),Kind::blind,13199)&&!control.has_condition(*control.effect_scope(4),Kind::blind,200)&&!control.has_condition(*control.effect_scope(5),Kind::blind,200),"Dirt adjacent foes only");
        require(control.resources(2)->health==480&&control.resources(3)->health==480,"Blindness deals no damage");
        require(control.blindness(caster,target,*effects::blindness(220,0),300)&&control.has_condition(target,Kind::blind,13199),"Shorter blind preserves longer expiry");
        auto stale=target;++stale.generation;require(!control.blindness(caster,stale,*effects::blindness(220),400),"Control rejects stale lifetime");
        auto forged=*effects::blindness(424);forged.radius=9999;require(!control.blindness(caster,target,forged,400),"Control rejects forged area");
        const auto midnight=*effects::blindness(58);
        require(!control.blindness(caster,*control.effect_scope(4),midnight,400)&&!control.has_condition(caster,Kind::blind,400),"Out of touch range does not blind caster");
        require(!control.blindness(caster,stale,midnight,400)&&!control.has_condition(caster,Kind::blind,400),"Stale foe does not blind caster");
        for(auto& box:boxes)box->drain();
        require(control.blindness(caster,target,midnight,400),"Midnight commits both conditions");
        for(auto& box:boxes){const auto packets=box->drain();
            const auto effect=fw::gw::skills::particles::midnight_target_2022(2,1);
            require(std::count(packets.begin(),packets.end(),effect)==1,"Midnight source-aware target visual once");
            require(std::count(packets.begin(),packets.end(),fw::gw::combat::detail::integer(21,1,104))==0,"No invented Midnight self particle");}
        require(control.has_condition(caster,Kind::blind,15399)&&control.has_condition(target,Kind::blind,15399),"Midnight caster and foe blinded for fifteen seconds");
        require(!control.has_condition(caster,Kind::blind,15400)&&!control.has_condition(target,Kind::blind,15400),"Midnight expiry boundary");
        require(!control.has_condition(*control.effect_scope(3),Kind::blind,15399)&&control.resources(1)->health==480&&control.resources(2)->health==480,"Midnight does not spread or deal damage");
    }
    {
        Instance capacity;auto box=std::make_shared<Outbox>();capacity.join(1,"Rune wearer",0,0,box);
        capacity.set_vitals(1,{400,480,20,20},100);const auto scope=*capacity.effect_scope(1);
        require(capacity.health_capacity(scope,405,0)&&capacity.resources(1)->health==325,"rune health penalty preserves deficit");
        require(capacity.health_capacity(scope,480,0)&&capacity.resources(1)->health==400,"removing rune restores capacity deficit");
        capacity.set_vitals(1,{1,480,20,20},100);require(capacity.health_capacity(scope,330,0)&&capacity.resources(1)->health==1,"maximum health decrease living floor1");
        capacity.set_vitals(1,{0,330,20,20},100);require(capacity.health_capacity(scope,480,0)&&capacity.resources(1)->health==0,"equipment cannot resurrect dead actor");
        capacity.leave(1);capacity.join(1,"Replacement",0,0,box);capacity.set_vitals(1,{480,480,20,20},100);
        require(!capacity.health_capacity(scope,405,0)&&capacity.resources(1)->max_health==480,"recycled actor rejects prior equipment lifetime");
    }
    std::cout << "GW world simulation: ground item CAS/ownership/range/partial/concurrent claims, snapshots, authoritative ticks, movement budget, conditions/cleansing/degeneration, real hex removal/cover order, party healing/range/DF, timed health overlap/expiry/lifetimes, server-time replanning, leave cleanup, queue bounds, 400-player fanout and instance isolation PASS\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "GW world simulation: " << error.what() << '\n';
    return 1;
}






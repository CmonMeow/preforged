#include "gw_npc_catalog.h"
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace {
unsigned checks=0;
void check(bool value,std::source_location at=std::source_location::current()){++checks;if(!value)throw std::runtime_error("Northlands World line "+std::to_string(at.line()));}
unsigned word(const agent::Packet& p,unsigned at){return unsigned(p.at(at))|unsigned(p.at(at+1))<<8|unsigned(p.at(at+2))<<16|unsigned(p.at(at+3))<<24;}
bool state(const agent::Packet& p,unsigned value){return p.size()==12&&p[0]==16&&p[1]==1&&p[2]==0xa1&&p[3]==0xc5&&word(p,4)==value&&word(p,8)==2;}
struct Room {
    std::shared_ptr<navigation::Mesh> mesh;std::unique_ptr<world::Instance> w=std::make_unique<world::Instance>();
    std::shared_ptr<world::Outbox> a=std::make_shared<world::Outbox>(),b=std::make_shared<world::Outbox>();
    Room(const std::filesystem::path& path,navigation::Point point=northlands::lever_position,unsigned plane=36,bool second=true){
        mesh=std::make_shared<navigation::Mesh>(navigation::Mesh::load(path));mesh->discard_unverified_portals();w->configure_navigation(mesh);w->configure_northlands(northlands::Scene::south146);
        w->join(1,"Gate player",point.x,point.z,a,0,{},plane,1);
        if(second)w->join(2,"Lever player",northlands::lever_position.x,northlands::lever_position.z,b,0,{},36,1);
        a->drain();b->drain();
    }
    auto actor(unsigned id=1){return *w->effect_scope(id);}auto lever(){return *w->northlands_lever_scope(62000);}
    void open(std::uint64_t now=1000,unsigned id=2){check(w->interact_northlands(actor(id),lever(),now)&&w->northlands_open(now));a->drain();b->drain();}
};
}
int main(int argc,char**argv)try{
    check(argc==2);const std::filesystem::path root=argv[1];const auto south=root/"navigation/113021.fwnav",north=root/"navigation/116025.fwnav";
    {
        Room r(south);check(r.w->northlands_installed()&&!r.w->northlands_open(1));
        check(!r.w->effect_scope(62000)&&!r.w->interaction_scope(62000)&&r.w->select_target(1,62000,0));
        auto stale=r.lever();++stale.generation;check(!r.w->interact_northlands(r.actor(),stale,100));
        check(!r.w->interact_northlands(r.actor(),r.lever(),100,2));
        r.open(1000,1);check(r.w->interact_northlands(r.actor(2),r.lever(),1172)&&!r.w->northlands_open(1172));
        check(r.w->interact_northlands(r.actor(2),r.lever(),1200)&&r.w->northlands_open(1200));
        r.w->tick(31200);check(!r.w->northlands_open(31200));
        auto old=r.actor();r.w->leave(1);r.w->join(1,"Rejoined",northlands::lever_position.x,northlands::lever_position.z,r.a,0,{},36,32000);
        check(r.actor()!=old&&!r.w->interact_northlands(old,r.lever(),32001));
        check(r.w->interact_northlands(r.actor(),r.lever(),32001));
    }
    {
        Room r(south,{-5800,12787},36,false);
        check(r.w->interact_northlands(r.actor(),r.lever(),1000)&&!r.w->northlands_open(1000));
        r.w->tick(1600);check(r.w->northlands_open(1600));
        auto late=std::make_shared<world::Outbox>();r.w->join(2,"Late",-5508,12787,late,0,{},36,1700,0,false);late->drain();
        const auto initial=r.w->npc_entry_packets(2,1700);check(std::any_of(initial.begin(),initial.end(),[](const auto& p){return state(p,0);}));
        check(std::count_if(initial.begin(),initial.end(),[](const auto& p){return p.size()==99&&p[0]==32&&(word(p,6)==1279||word(p,6)==1280);})==2);
        r.w->tick(32000);check(!r.w->northlands_open(32000));
    }
    {
        Room r(south,{-5800,12787},36,false);check(r.w->interact_northlands(r.actor(),r.lever(),1000));
        r.w->move(1,-6000,12787,1100,36);r.w->tick(3000);check(!r.w->northlands_open(3000));
    }
    const navigation::Point from{-5600,12122.3f},to{-5320,12122.7f};
    for(bool held:{false,true}){
        Room r(south,from);r.open();
        if(held)r.w->steer(1,1,.001f,30850);else r.w->move(1,to.x,to.z,30850,37);
        // Const projection cannot extrapolate the stale open graph past its
        // due boundary before a World synchronization/physics tick.
        const auto before=r.w->position(1,33000);check(std::hypot(before.first-from.x,before.second-from.z)<.01);
        r.w->tick(33000);check(!r.w->northlands_open(33000));
        const auto at=r.w->position(1,33000);const auto plane=r.w->plane(1);const auto cell=r.mesh->locate_cell(plane,{at.first,at.second});check(cell.has_value());
        const auto shut=northlands::navigation_view(*r.mesh,northlands::Scene::south146,false);
        check(shut->route({36,20},from,{plane,*cell},{at.first,at.second}).has_value());
        check(!shut->route({plane,*cell},{at.first,at.second},{37,10},to));
        const auto stopped=at;r.w->tick(34000);
        if(!held){const auto later=r.w->position(1,34000);check(std::hypot(later.first-stopped.first,later.second-stopped.second)<.01);}
    }
    {
        Room r(south,from);r.open();r.w->move(1,to.x,to.z,29000,37);r.w->tick(33000);
        check(r.w->plane(1)==37&&!r.w->northlands_open(33000));
        // Already crossed before closure: no timer-based portal/passage veto.
        r.w->move(1,-5479,13797,33001,36);r.w->tick(41000);const auto at=r.w->position(1,41000);
        check(r.w->plane(1)==36&&std::hypot(at.first+5479,at.second-13797)<.1);
    }
    {
        std::ifstream actors(root/"quests/actors/monk-test-givers.txt"),metadata(root/"quests/monk-test-giver-combat.txt");check(bool(actors)&&bool(metadata));
        auto population=world::load_npc_catalog(actors);const std::string hash="14b6cc1b3b97e5ad5b4bcb922486890c0675036440f23e93ff38d6ccb2001b1f";
        world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);auto npcs=population.for_map(146);check(npcs.size()==2);
        for(auto& n:npcs){n.x=n.id==20051?from.x:-5700;n.y=from.z;n.plane=36;}
        auto mesh=std::make_shared<navigation::Mesh>(navigation::Mesh::load(south));mesh->discard_unverified_portals();auto w=std::make_unique<world::Instance>();
        w->configure_navigation(mesh);w->configure_npcs(npcs);w->configure_northlands(northlands::Scene::south146);
        auto owner_box=std::make_shared<world::Outbox>(),lever_box=std::make_shared<world::Outbox>();
        w->join(1,"Escort owner north",to.x,to.z,owner_box,0,{},37,1);w->join(2,"Lever",-5508,12787,lever_box,0,{},36,1);w->set_vitals(1,{100,100,20,20},100);
        const auto owner=*w->effect_scope(1),gwen=*w->effect_scope(20051),ciglo=*w->effect_scope(20050),lever=*w->northlands_lever_scope(62000);
        check(w->restore_completed_escort_objective(owner,gwen,ciglo,146,1));check(w->interact_northlands(*w->effect_scope(2),lever,1000));
        check(w->follow_escort(owner,1,100,10,30850));w->tick(33000);check(!w->northlands_open(33000));
        const auto stopped=w->maintained_range_path(gwen,33000,33000);check(stopped&&!stopped->empty()&&std::hypot(stopped->front().x-from.x,stopped->front().y-from.z)<=15.1);
        check(!w->follow_escort(owner,1,100,10,33001));check(w->interact_northlands(*w->effect_scope(2),lever,33002));
        check(w->follow_escort(owner,1,100,10,33003));w->tick(37000);const auto resumed=w->maintained_range_path(gwen,37000,37000);check(resumed&&!resumed->empty()&&std::hypot(resumed->front().x-to.x,resumed->front().y-to.z)<.1);
    }
    {
        // A skill event can have settled a stationary actor beyond a delayed
        // gate boundary. Closing must preserve that committed health/control
        // state; a historical observation is not an actor/route retirement.
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;
        n.x=-5508;n.y=12787;n.plane=36;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=1;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};
        d.damageable=d.conditions_allowed=true;n.combat_definition=d;
        auto mesh=std::make_shared<navigation::Mesh>(navigation::Mesh::load(south));mesh->discard_unverified_portals();
        auto middle=n,last=n;
        middle.id=middle.definition.id=middle.combat_definition->binding.actor=middle.combat_definition->binding.definition=101;
        last.id=last.definition.id=last.combat_definition->binding.actor=last.combat_definition->binding.definition=102;
        auto w=std::make_unique<world::Instance>();w->configure_navigation(mesh);w->configure_npcs({n,middle,last});w->configure_northlands(northlands::Scene::south146);
        auto box=std::make_shared<world::Outbox>();w->join(1,"Future condition",-5508,12787,box,0,{},36,1);w->set_vitals(1,{100,100,20,20},100);
        const auto owner=*w->effect_scope(1),target=*w->effect_scope(100),lever=*w->northlands_lever_scope(62000);
        check(w->interact_northlands(owner,lever,1000));
        check(w->apply_condition(owner,*w->effect_scope(101),combat::conditions::Kind::burning,30000,10000));
        check(w->apply_condition(owner,*w->effect_scope(102),combat::conditions::Kind::burning,30000,10000));
        check(w->apply_condition(owner,target,combat::conditions::Kind::crippled,34000,10000));
        const auto before=w->npc_combat_snapshot(target);check(before&&before->health);
        w->synchronize_northlands(33000);check(!w->northlands_open(33000));
        const auto after=w->npc_combat_snapshot(target);check(after&&after->health&&after->health->current==before->health->current&&after->effects==before->effects);
        check(w->effect_scope(100)==target);
        for(unsigned id:{101u,102u}){const auto live=w->npc_combat_snapshot(*w->effect_scope(id));check(live&&live->health&&live->health->current<500&&live->health->current>0);}
    }
    {
        auto mesh=std::make_shared<navigation::Mesh>(navigation::Mesh::load(north));mesh->discard_unverified_portals();auto w=std::make_unique<world::Instance>();w->configure_navigation(mesh);w->configure_northlands(northlands::Scene::north147);
        auto box=std::make_shared<world::Outbox>();w->join(1,"Return",-11574,-16170,box,0,{},0,1);check(!w->northlands_lever_scope(62000)&&w->northlands_open(1000000));
        w->move(1,-11711,-16914,1000001,22);w->tick(1006000);check(w->plane(1)==22&&w->northlands_open(1006000));
    }
    std::cout<<"PASS actual World Northlands shared gate, native object publication, approach/cancel, overdue click/held/escort motion, continuation and North return: "<<checks<<" checks\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

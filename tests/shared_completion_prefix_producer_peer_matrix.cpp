// Detached representative actualWorld matrix; requires concrete prefixframe API.
// No synthetic native packets: World-owned assemblies are compared byte-exact.
#include "gw_world_simulation.h"
#include "gw_completion_publication.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;
static void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Producer frame matrix line "+std::to_string(at.line()));}
struct Fixture{
 std::unique_ptr<world::Instance> store=std::make_unique<world::Instance>();world::Instance& w=*store;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope owner,ally,foe;
 Fixture(unsigned primary){world::Instance::Npc n;n.id=n.definition.id=100;n.x=300;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{100,100};d.damageable=true;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Matrix actor",id==1?0:60,0,id==1?box:std::make_shared<world::Outbox>(),0,id==1?std::vector<world::Packet>{{165,0,1,0,0,0,static_cast<unsigned char>(primary),0}}:std::vector<world::Packet>{},{},100,id==3?2:1);w.set_vitals(id,{500,1000,100,100},100);}
  owner=*w.effect_scope(1);ally=*w.effect_scope(2);foe=*w.effect_scope(3);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));box->drain();
 }
};
struct Guard{std::optional<skills::casting::CompletionPublication> frame;~Guard(){if(frame)frame->finish();}void finish(){if(frame){frame->finish();frame.reset();}}};
int main()try{
 for(unsigned kind=0;kind<5;++kind){
  std::vector<world::Packet> baseline;
  for(bool framed:{false,true}){
   Fixture f(kind==1?1u:kind==3?8u:kind==4?4u:3u);const auto npc=*f.w.effect_scope(100);
   if(kind==0)check(f.w.apply_condition(f.foe,f.ally,combat::conditions::Kind::blind,100,5000));
   if(kind==1){check(f.w.apply_condition(f.foe,f.ally,combat::conditions::Kind::crippled,100,5000));check(f.w.apply_condition(f.foe,f.ally,combat::conditions::Kind::blind,100,5000));}
   f.box->drain();Guard guard;if(framed){guard.frame=f.w.prepare_completion_publication(f.owner,200);check(bool(guard.frame));}
   std::function<void()> publication;
   if(kind==0){check(f.w.complete_martyr(f.owner,*effects::martyr(),0,200));check(f.w.has_condition(f.owner,combat::conditions::Kind::blind,200)&&!f.w.has_condition(f.ally,combat::conditions::Kind::blind,200));}
   if(kind==1){check(f.w.stage_charge_allies(f.owner,*effects::charge(0),200,&publication));check(!f.w.has_condition(f.ally,combat::conditions::Kind::crippled,200)&&f.w.has_condition(f.ally,combat::conditions::Kind::blind,200));}
   if(kind==2){check(f.w.complete_aegis(f.owner,*effects::aegis(0),0,200));const auto status=*f.w.status_snapshot(f.ally,200);check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==257;}));}
   if(kind==3){check(f.w.apply_weapon_spell(f.owner,f.ally,*effects::weapon_spell(793,0,8,0),200,nullptr,&publication));const auto status=*f.w.status_snapshot(f.ally,200);check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==793;}));}
   if(kind==4){auto result=f.w.complete_signet_sorrow(f.owner,npc,*effects::signet_sorrow(12),20,200);check(result.accepted&&result.recharge_ms==30000&&result.visuals.empty());publication=std::move(result.publication);check(f.w.npc_combat_snapshot(npc)->health->current==37);}
   if(kind==1||kind==3||kind==4){check(bool(publication));check(f.box->drain().empty());
    if(!framed)publication(); // Framed output MUST already be committed behind P0 without invoking returned noop.
   }
   if(framed){check(f.box->drain().empty());guard.frame->publish_prefix();guard.finish();}
   auto packets=f.box->drain();check(!packets.empty());
   if(!framed)baseline=packets;else check(packets==baseline);
   if(publication){publication();check(f.box->drain().empty());}
   if(kind==0||kind==2){const auto finish=combat::detail::integer(58,1,0);check(std::count(packets.begin(),packets.end(),finish)==1);}
  }
 }
 std::cout<<"PASS actual Martyr/Charge/Aegis/WeaponSpell/Sorrow nonframe-vs-frame packet/state equivalence, assembly and publication noops\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

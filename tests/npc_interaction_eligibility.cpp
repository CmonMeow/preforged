// Shared World eligibility with real native and legacy collector bindings.
#include "gw_world_simulation.h"
#include "gw_crafter_panel.h"
#include "gw_merchant_panel.h"
#include "gw_presearing_collectors.h"
#include "gw_collector_dialogue.h"
#include <iostream>
#include <source_location>
#include <tuple>
using namespace fw::gw;
static void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("interaction eligibility line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance world;skills::effects::Scope player,npc;
 std::shared_ptr<world::Outbox> out=std::make_shared<world::Outbox>();
 Fixture(std::string name){world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=2;n.definition.visual_adjustment={0,0,0,100};n.captured_name=std::move(name);n.x=n.y=0;
  world.configure_npcs({n});world.join(1,"Tester",0,0,out,0,{}, {},100,1);world.set_vitals(1,{100,100,40,40},100);player=*world.effect_scope(1);npc=*world.interaction_scope(100);
 }
 collectors::native::Observation observation(unsigned map,std::uint64_t now){return {100,map,6,npc.instance,npc.generation,10,now,world.can_interact_npc(player,npc,now)};}
 void kill_retaining_range(){check(world.kill(player,600));check(world.resources(1)->health==0);check(world.nearby_npc_name(1,100,600).has_value());check(!world.can_interact_npc(player,npc,600));}
};
int main()try{
 {Fixture f("Arthur Ayala [Weapons]");check(f.world.can_interact_npc(f.player,f.npc,500));auto stale=f.player;++stale.generation;check(!f.world.can_interact_npc(stale,f.npc,500));auto wrong=f.npc;++wrong.instance;check(!f.world.can_interact_npc(f.player,wrong,500));wrong=f.npc;++wrong.generation;check(!f.world.can_interact_npc(f.player,wrong,500));check(!f.world.can_interact_npc(f.player,f.npc,99));}
 // Real crafting catalog/binding retained over authoritative World death.
 {Fixture f("Arthur Ayala [Weapons]");auto owned=inventory::baseline();owned.gold=500;owned.next_item=45;
  for(auto [id,model,element,slot]:{std::tuple{43u,948u,1u,0u},std::tuple{44u,946u,6u,1u}}){inventory::Item item;item.definition.id=id;item.definition.file=1234;item.definition.type=11;item.definition.model=model;item.definition.quantity=10;item.definition.interaction=0x80001;item.definition.modifiers={0x25080000u|(element<<8)};item.bag=1;item.slot=slot;owned.items.emplace(id,item);}
  crafter::Binding menu;collectors::native::MenuIds ids;const auto open=[&](){check(crafter::open_panel(menu,ids,f.observation(148,500),"Arthur Ayala",owned).has_value());return menu.recipes.front().reward.definition.id;};
  auto id=open();crafter::Request request{75,3,100,0,{{43,1},{44,1}},{{id,1}}};check(crafter::authorize(menu,request,owned,f.observation(148,500)).has_value());request.receive[0].item=open();const auto before=inventory::encode(owned);f.kill_retaining_range();check(!crafter::authorize(menu,request,owned,f.observation(148,600)));check(!menu.observed.actor&&inventory::encode(owned)==before);
 }
 // Same shared World gate, actual merchant catalog and admission helper.
 {Fixture f("Sanura [Merchant]");auto owned=inventory::baseline();owned.gold=500;merchant::native::Binding menu;collectors::native::MenuIds ids;
  const auto open=[&](){check(merchant::native::open_panel(menu,ids,f.observation(148,500),"Sanura",owned).has_value());return menu.stock.front().definition.id;};
  auto id=open();merchant::native::Request request{75,1,100,0,{},{{id,1}}};check(merchant::native::authorize(menu,request,owned,f.observation(148,500)).has_value());request.receive[0].item=open();const auto before=inventory::encode(owned);f.kill_retaining_range();check(!merchant::native::authorize(menu,request,owned,f.observation(148,600)));check(!menu.actor&&inventory::encode(owned)==before);
 }
 // The native collector path also receives eligibility, with a live control.
 {Fixture f("Brownlow");auto owned=inventory::baseline();const auto offers=collectors::presearing_offers();const auto& offer=offers.front();inventory::Item trophy;trophy.definition=offer.trophy;trophy.definition.id=43;trophy.definition.quantity=offer.cost;trophy.bag=1;trophy.slot=0;owned.items.emplace(43,trophy);owned.next_item=44;collectors::native::Binding menu;collectors::native::MenuIds ids;
  const auto open=[&](){check(collectors::native::open_panel(menu,ids,offers,f.observation(146,500),"Brownlow").has_value());return menu.offers.front().first;};
  auto id=open();collectors::native::Request request{75,2,0,0,{{43,offer.cost}},{{id,1}}};check(collectors::native::authorize(menu,request,offers,owned,f.observation(146,500)).has_value());request.receive[0].item=open();
  collectors::Menu fallback;check(!collectors::open_menu(fallback,offers,owned,100,146,6,"Brownlow",10,500).empty());
  const auto token=collectors::choose_token+offer.key;check(collectors::select_menu(fallback,offers,token,100,146,6,"Brownlow",10,500));
  check(!collectors::open_menu(fallback,offers,owned,100,146,6,"Brownlow",10,500).empty());
  const auto before=inventory::encode(owned);f.kill_retaining_range();check(!collectors::native::authorize(menu,request,offers,owned,f.observation(146,600)));check(!menu.actor&&inventory::encode(owned)==before);
  const auto nearby=f.world.nearby_npc_name(1,100,600);const bool allowed=f.world.can_interact_npc(f.player,f.npc,600);
  check(!collectors::select_menu(fallback,offers,token,100,146,6,allowed?std::string_view(*nearby):std::string_view{},10,600)&&!fallback.actor);

 }
 std::cout<<"PASS live controls and dead retained native crafter/merchant/collector panels\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

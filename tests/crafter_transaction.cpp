// Exact captured Arthur reward; material definitions below are explicit
// synthetic authority fixtures using reviewed GWCA model/0x2508 slot identity.
#include "../src/runtime/gw_crafter_transaction.h"
#include <iostream>
#include <type_traits>
#include <fstream>
#include <sstream>
int main(int argc,char** argv)try{
 namespace c=fw::gw::crafter;namespace inv=fw::gw::inventory;
 const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 static_assert(!std::is_default_constructible_v<c::Plan>);
 c::Recipe axe;auto& d=axe.reward.definition;d.id=347;d.file=2147493136u;d.type=2;d.unknown1=6;d.materials=489;d.unknown2=1;
 d.interaction=571473921;d.price=17;d.model=12;d.quantity=1;d.encoded_name={8594,58318,39591,29238};
 d.modifiers={0x24b80200,0x240800d9,0x253001b3,0x23480500,0xa4880a05};
 axe.native_index=488;axe.gold=100;axe.materials={{1,4},{6,1}};
 check(c::valid_recipe(axe),"captured recipe/table identity rejected");
 auto owned=inv::baseline();owned.gold=1000;owned.next_item=46;
 for(auto [id,model,element,quantity,slot]:{std::tuple{43u,948u,1u,5u,0u},std::tuple{44u,946u,6u,3u,1u},std::tuple{45u,948u,1u,10u,2u}}){
  inv::Item item;item.definition.id=id;item.definition.file=1234;item.definition.type=11;item.definition.model=model;
  item.definition.quantity=quantity;item.definition.interaction=0x80001;item.definition.modifiers={0x25080000u|(element<<8)};
  item.bag=1;item.slot=static_cast<std::uint8_t>(slot);owned.items.emplace(id,item);
 }
 inv::validate_character(owned);const c::Observation at{10007,148,0,99,7,10,500,true};
 fw::gw::collectors::native::MenuIds ids;c::Binding menu;
 const auto open=[&](const inv::Profile& profile){check(c::bind(menu,ids,at,"Arthur Ayala [Weapons]",profile,std::span(&axe,1)),"trusted opening unavailable");return menu.recipes.front().reward.definition.id;};
 auto selected=open(owned);c::Request request{75,3,100,0,{{43,1},{44,1}},{{selected,1}}};
 auto plan=c::authorize(menu,request,owned,at);check(bool(plan)&&!menu.observed.actor,"craft admission failed/retained opening");
 check(plan->gold()==100&&plan->selected_debits().size()==2&&plan->selected_debits()[0].quantity==4&&plan->selected_debits()[1].quantity==1,"stack marker1 was treated as literal debit1");
 const auto proposal=c::propose(*plan,owned,10);
 check(proposal&&proposal->profile.gold==900&&proposal->profile.items.at(43).definition.quantity==1&&proposal->profile.items.at(44).definition.quantity==2&&proposal->profile.items.size()==4,"exact recipe gold/material reward proposal failed");
 auto changed=owned;changed.items.at(43).definition.price=1;
 check(!c::propose(*plan,changed,10)&&!c::propose(*plan,owned,11),"changed inventory/revision admitted");
 const auto prior=selected;selected=open(owned);request.receive.front().item=prior;
 check(!c::authorize(menu,request,owned,at),"old recipe menu identity reused after reopen");
 for(unsigned bad=0;bad<5;++bad){selected=open(owned);request={75,3,100,0,{{43,1},{44,1}},{{selected,1}}};
  auto profile=owned;switch(bad){case 0:request.give[0].quantity=4;break;case 1:request.give.push_back({43,1});break;
   case 2:request.give.push_back({45,1});break;case 3:request.give_gold=34;break;case 4:profile.items.at(43).definition.modifiers={0x25080600};break;}
  check(!c::authorize(menu,request,profile,at),"forged marker/stack/merchant quote/material admitted");
 }
 // Open over each forged definition, so snapshot-CAS cannot mask the
 // canonical material classifier. Value is never material identity.
 for(unsigned bad=0;bad<5;++bad){auto forged=owned;auto& material=forged.items.at(43).definition;
  switch(bad){case 0:material.model=99999;break;case 1:material.modifiers.clear();break;
   case 2:material.modifiers={0x25080600};break;case 3:material.type=2;break;
   case 4:material.modifiers.push_back(0x25080100);break;}
  material.price=3;selected=open(forged);request={75,3,100,0,{{43,1},{44,1}},{{selected,1}}};
  check(!c::authorize(menu,request,forged,at),"value-only/unknown/ambiguous material authority admitted");
 }
 for(unsigned bad=0;bad<7;++bad){selected=open(owned);request={75,3,100,0,{{43,1},{44,1}},{{selected,1}}};auto scope=at;
  switch(bad){case 0:++scope.actor;break;case 1:++scope.world;break;case 2:++scope.npc_generation;break;
   case 3:++scope.revision;break;case 4:scope.now=at.now+120000;break;case 5:scope.nearby=false;break;case 6:scope.now=at.now-1;break;}
  check(!c::authorize(menu,request,owned,scope),"stale/out-of-range craft scope admitted");
 }
 // Trusted row factors are bounded before multiplication; aggregate quote
 // still must fit the100000 carried-gold protocol policy, not wrap to a quote.
 auto costly=axe;costly.gold=100000;check(c::valid_recipe(costly),"valid unit cap rejected");
 check(c::bind(menu,ids,at,"Arthur Ayala",owned,std::span(&costly,1)),"bounded expensive row rejected");
 request={75,3,200000,0,{{43,1},{45,1},{44,1}},{{menu.recipes.front().reward.definition.id,2}}};
 check(!c::authorize(menu,request,owned,at),"aggregate gold cap bypassed");
 auto invalid=axe;invalid.materials.front().units=UINT32_MAX;check(!c::valid_recipe(invalid),"unbounded material factor admitted");
 invalid=axe;invalid.maximum_quantity=UINT32_MAX;check(!c::valid_recipe(invalid),"unbounded batch factor admitted");
 selected=open(owned);request={75,3,200,0,{{43,1},{45,1},{44,1}},{{selected,2}}};plan=c::authorize(menu,request,owned,at);
 check(plan&&plan->selected_debits().size()==3&&plan->selected_debits()[0].quantity==5&&plan->selected_debits()[1].quantity==3&&plan->selected_debits()[2].quantity==2,"split selected-stack quantities wrong");
 const auto multiple=c::propose(*plan,owned,10);
 check(multiple&&!multiple->profile.items.contains(43)&&multiple->profile.items.at(45).definition.quantity==7&&multiple->profile.items.at(44).definition.quantity==1&&multiple->profile.gold==800&&multiple->profile.items.size()==4,"batch crafting recipe debit/reward mismatch");
 auto full=owned;full.bags.at(1).capacity=3;selected=open(full);request.receive.front().item=selected;plan=c::authorize(menu,request,full,at);
 const auto full_before=inv::encode(full);
 check(plan&&!c::propose(*plan,full,10)&&inv::encode(full)==full_before,"last crafted item capacity failure leaked first reward/debit");
 // A canonical material definition grants no debit authority when physically
 // outside a carried bag, even if a stale/private record places it equipped.
 auto equipped=owned;equipped.items.at(43).bag=2;equipped.items.at(43).slot=0;
 selected=open(equipped);request={75,3,100,0,{{43,1},{44,1}},{{selected,1}}};
 check(!c::authorize(menu,request,equipped,at),"equipped canonical material admitted for craft debit");
 // Reuse real durable/wire identity projection and revision CAS pattern.
 auto durable=owned;durable.inventory=3;inv::identity::Session identities;auto wire=identities.hydrate(durable,fw::gw::storage::baseline());check(wire.character.inventory==1,"durable3 not projected to localwire1");std::uint64_t revision=10;
 selected=open(wire.character);
 request={75,3,100,0,{{*identities.wire_item(inv::identity::Domain::character,43),1},{*identities.wire_item(inv::identity::Domain::character,44),1}},{{selected,1}}};
 plan=c::authorize(menu,request,wire.character,at);check(bool(plan),"wire identity craft admission failed");
 const auto before=inv::encode(wire.character),bank_before=inv::encode(wire.bank);const auto identity_revision=identities.revision();unsigned calls=0;
 auto committed=c::commit(*plan,identities,wire.character,wire.bank,revision,[&](const inv::Profile& profile,std::uint64_t expected)->std::optional<std::uint64_t>{++calls;
  check(expected==10&&profile.inventory==3&&profile.gold==900&&profile.items.at(43).definition.quantity==1&&profile.items.size()==4,"CAS received incomplete recipe transaction");return {};});
 check(!committed&&calls==1&&revision==10&&identities.revision()==identity_revision&&inv::encode(wire.character)==before&&inv::encode(wire.bank)==bank_before,"failed crafting CAS mutated authority");
 std::optional<inv::Profile> saved_profile;
 committed=c::commit(*plan,identities,wire.character,wire.bank,revision,[&](const inv::Profile& profile,std::uint64_t expected)->std::optional<std::uint64_t>{++calls;check(expected==10&&profile.inventory==3&&profile.gold==900,"successful craft lost durableinventory3/gold debit");saved_profile=inv::decode(inv::encode(profile));return 11;});
 check(committed&&calls==2&&revision==11&&wire.character.gold==900&&wire.character.items.size()==4,"successful crafting CAS failed");
 check(saved_profile&&saved_profile->inventory==3&&wire.character.inventory==1,"durable/wire inventory identity conflated");
 inv::identity::Session reconnect;const auto restored=reconnect.hydrate(*saved_profile,fw::gw::storage::baseline());
 check(restored.character.inventory==1&&restored.character.gold==900&&restored.character.items.size()==4,"durable3 crafted reward failed reconnect projection");
 check(!c::commit(*plan,identities,wire.character,wire.bank,revision,[&](const auto&,auto)->std::optional<std::uint64_t>{throw std::runtime_error("replay reached save");}),"craft replay admitted");
 check(argc==2,"expected hash-pinned13recipe fixture path");std::ifstream fixture(argv[1]);std::string line;std::getline(fixture,line);
 check(line=="FWCR1","bad crafter recipe fixture header");unsigned rows=0;
 while(std::getline(fixture,line)){if(line.empty())continue;std::istringstream stream(line);std::vector<unsigned> values;unsigned value;
  while(stream>>value)values.push_back(value);std::size_t cursor=0;const auto take=[&](){check(cursor<values.size(),"truncated recipe row");return values[cursor++];};
  (void)take();c::Recipe row;auto& def=row.reward.definition;def.id=take();def.file=take();def.type=take();def.unknown1=take();def.dye=take();def.materials=take();def.unknown2=take();def.interaction=take();def.price=take();def.model=take();def.quantity=take();
  auto count=take();check(count<=64,"unbounded fixture name");while(count--)def.encoded_name.push_back(static_cast<std::uint16_t>(take()));
  count=take();check(count<=64,"unbounded fixture modifiers");while(count--)def.modifiers.push_back(take());row.native_index=take();row.gold=take();count=take();check(count<=16,"unbounded recipe material list");
  while(count--){auto element=take();auto units=take();row.materials.push_back({element,units});}check(cursor==values.size()&&c::valid_recipe(row),"captured recipe identity rejected");
  auto profile=inv::baseline();profile.gold=1000;profile.next_item=100;unsigned itemid=43,slot=0;c::Request craft{75,3,row.gold,0,{},{}};
  for(auto material:row.materials){const auto found=std::find_if(inv::materials::rows.begin(),inv::materials::rows.end(),[&](auto known){return known.element==material.element;});check(found!=inv::materials::rows.end(),"unreviewed recipe slot");
   inv::Item stack;stack.definition.id=itemid;stack.definition.file=1234;stack.definition.type=11;stack.definition.model=found->model;stack.definition.quantity=material.units*def.unknown2;stack.definition.interaction=0x80001;stack.definition.modifiers={0x25080000u|(material.element<<8)};stack.bag=1;stack.slot=slot++;
   profile.items.emplace(itemid,stack);craft.give.push_back({itemid++,1});
  }
  check(c::bind(menu,ids,at,"Arthur Ayala",profile,std::span(&row,1)),"captured recipe opening rejected");craft.receive={{menu.recipes.front().reward.definition.id,1}};
  const auto admitted=c::authorize(menu,craft,profile,at);check(bool(admitted),"captured recipe transaction rejected");const auto proposed=c::propose(*admitted,profile,10);
  check(proposed&&proposed->profile.gold==1000-row.gold&&proposed->profile.items.size()==1,"captured recipe net materials/gold/reward mismatch");
  auto actual=proposed->profile.items.begin()->second.definition;actual.id=def.id;check(fw::gw::items::definition_2022(actual)==fw::gw::items::definition_2022(def),"crafted reward definition mutated");++rows;
 }
 check(rows==13,"expected all13 window-associated recipe candidates");
 std::cout<<"PASS exact native crafter recipes, stack selection, batch capacity and CAS\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

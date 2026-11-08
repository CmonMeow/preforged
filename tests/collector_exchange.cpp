#include "gw_presearing_collectors.h"
#include "gw_collector_dialogue.h"
#include <iostream>
#include <iterator>
#include <stdexcept>
int main()try{
 namespace c=fw::gw::collectors;namespace inv=fw::gw::inventory;
 const auto check=[](bool v,const char* s){if(!v)throw std::runtime_error(s);};
 const auto offers=c::presearing_offers();check(offers.size()==24,"captured catalog offer count");
 for(const auto& o:offers){
  auto p=inv::baseline();p.next_item=3;inv::Item t;t.definition=o.trophy;t.definition.id=1;t.definition.quantity=1;t.bag=1;t.slot=0;p.items.emplace(1,t);
  t.definition.id=2;t.definition.quantity=o.cost-1;t.slot=1;p.items.emplace(2,t);
  const auto before=inv::encode(p);auto proposal=c::exchange(p,o,o.map,6,u"Collector Tester");
  check(bool(proposal),"all genuine catalog entries exchange");check(proposal->profile.items.size()==1&&proposal->reward==3,"debit split stacks and issue one owned reward");
  check(inv::encode(p)==before,"exchange leaves live inventory untouched");check(proposal->profile.items.at(3).definition.model==o.reward.definition.model,"real offer definition preserved");
  check(proposal->packets.size()>=6,"debit detach/delete precede reward publication");
  check(!c::exchange(p,o,o.map+1,6,u"Collector Tester"),"wrong map denied");
  p.items.at(2).definition.quantity--;if(!p.items.at(2).definition.quantity)p.items.erase(2);
  check(!c::exchange(p,o,o.map,6,u"Collector Tester"),"missing trophy denied");
 }
 const auto& pouch=offers[0];check(pouch.collector=="Brownlow"&&pouch.cost==5&&pouch.reward.definition.model==33,"Brownlow native pouch binding");
 auto p=inv::baseline();p.bags.at(1).capacity=1;p.next_item=2;inv::Item t;t.definition=pouch.trophy;t.definition.id=1;t.definition.quantity=5;t.bag=1;p.items.emplace(1,t);
 check(bool(c::exchange(p,pouch,146,6,u"Tester")),"full inventory exchange can reuse fully consumed slot");
 p.items.at(1).definition.quantity=6;const auto before=inv::encode(p);
 check(!c::exchange(p,pouch,146,6,u"Tester")&&inv::encode(p)==before,"partial stack cannot release reward slot; failure rolls back");
 p.bags.at(1).capacity=20;
 auto result=c::exchange(p,pouch,146,6,u"Tester");check(result&&result->profile.items.at(1).definition.quantity==1,"partial stack debited exactly");
 p.items.at(1).definition.file++;check(!c::exchange(p,pouch,146,6,u"Tester"),"model lookalike rejected");p.items.at(1).definition.file--;
 p.items.at(1).definition.encoded_name=u"Forged";check(!c::exchange(p,pouch,146,6,u"Tester"),"encoded name lookalike rejected");p.items.at(1).definition.encoded_name=pouch.trophy.encoded_name;
 p.next_item=65535;check(!c::exchange(p,pouch,146,6,u"Tester"),"identity exhaustion cannot debit trophies");
 {
  const auto& egg=offers[1];check(egg.reward.definition.interaction&0x80000,"captured Devourer Egg is stackable");
  auto packed=inv::baseline();packed.bags.at(1).capacity=2;packed.next_item=3;
  inv::Item fins;fins.definition=egg.trophy;fins.definition.id=1;fins.definition.quantity=6;fins.bag=1;fins.slot=0;
  auto eggs=egg.reward;eggs.definition.id=2;eggs.definition.quantity=249;eggs.bag=1;eggs.slot=1;
  packed.items={{1,fins},{2,eggs}};const auto original=inv::encode(packed);
  auto merged=c::exchange(packed,egg,146,6,u"Tester");
  check(merged&&merged->reward==2&&merged->profile.items.size()==2&&merged->profile.next_item==3&&
        merged->profile.items.at(1).definition.quantity==1&&merged->profile.items.at(2).definition.quantity==250,
        "full backpack Brownlow exchange must merge exact egg reward");
  check(merged->packets.size()==2&&inv::encode(packed)==original,"stack reward publishes two quantities and leaves source unchanged");
  inv::identity::Session stack_ids;auto stack_wire=stack_ids.hydrate(packed,fw::gw::storage::baseline());std::uint64_t stack_revision=8;
  const auto existing_reward=std::find_if(stack_wire.character.items.begin(),stack_wire.character.items.end(),
      [](const auto& row){return row.second.definition.model==2556;})->first;
  auto committed=c::commit_exchange(stack_ids,stack_wire.character,stack_wire.bank,stack_revision,egg,146,6,u"Tester",
      [&](const inv::Profile& candidate,std::uint64_t expected)->std::optional<std::uint64_t>{
       check(expected==8&&candidate.items.size()==2&&candidate.next_item==3&&candidate.items.at(2).definition.quantity==250,
             "collector stack CAS receives exact durable debit and merge");return 9;});
  check(committed&&committed->reward==existing_reward&&stack_revision==9&&committed->packets.size()==2,
        "live collector commit preserves existing reward wire identity when merging");
  packed.items.at(2).definition.quantity=250;
  check(!c::exchange(packed,egg,146,6,u"Tester"),"full egg stack cannot exceed native250 limit");
  packed.items.at(2).definition.quantity=1;packed.items.at(2).definition.file++;
  check(!c::exchange(packed,egg,146,6,u"Tester"),"reward model lookalike cannot absorb collector reward");
  packed.items.at(2).definition.file--;packed.items.at(2).customized=u"Someone Else";
  check(!c::exchange(packed,egg,146,6,u"Tester"),"reward metadata mismatch cannot absorb collector reward");
 }
 {
  // Exercise actual session exhaustion: reloading successive durable identities
  // retires wire IDs, as in inventory_identity_test. A final carried egg stack
  // still has room although the connection cannot allocate another owned ID.
  namespace identity=inv::identity;
  const auto& egg=offers[1];auto durable=inv::baseline();durable.bags.at(1).capacity=2;
  inv::Item fins;fins.definition=egg.trophy;fins.definition.id=1;fins.definition.quantity=6;fins.bag=1;fins.slot=0;
  durable.items.emplace(1,fins);durable.next_item=2;
  identity::Session exhausted;auto wire=exhausted.hydrate(durable,fw::gw::storage::baseline());
  for(unsigned id=2;id<=identity::owned_last;++id){
   if(durable.items.size()>1)durable.items.erase(std::prev(durable.items.end()));
   auto eggs=egg.reward;eggs.definition.id=id;eggs.definition.quantity=249;eggs.bag=1;eggs.slot=1;
   durable.items.emplace(id,eggs);durable.next_item=id+1;
   wire=exhausted.hydrate(durable,fw::gw::storage::baseline());
  }
  check(wire.character.next_item==identity::owned_last+1,"collector regression did not exhaust real session allocator");
  const auto before=inv::encode(wire.character);std::uint64_t revision=12;unsigned saves=0;
  auto failed=c::commit_exchange(exhausted,wire.character,wire.bank,revision,egg,146,6,u"Tester",
      [&](const inv::Profile&,std::uint64_t)->std::optional<std::uint64_t>{++saves;return {};});
  check(!failed&&saves==1&&revision==12&&inv::encode(wire.character)==before,
        "exhausted allocator merge failure must preserve inventory and session state");
  auto merged=c::commit_exchange(exhausted,wire.character,wire.bank,revision,egg,146,6,u"Tester",
      [&](const inv::Profile& candidate,std::uint64_t expected)->std::optional<std::uint64_t>{
       ++saves;check(expected==12&&candidate.next_item==durable.next_item&&candidate.items.at(1).definition.quantity==1&&
             candidate.items.at(identity::owned_last).definition.quantity==250,"exhausted merge changed durable identity allocator");return 13;});
  check(merged&&merged->reward==identity::owned_last&&revision==13&&saves==2&&
        wire.character.next_item==identity::owned_last+1,"exhausted session rejected allocation-free collector reward");
  // The same exhausted cursor still rejects a reward requiring a fresh identity
  // before invoking persistence, even if consuming all trophies frees its slot.
  wire.character.items.at(1).definition.quantity=5;
  auto denied=c::commit_exchange(exhausted,wire.character,wire.bank,revision,pouch,146,6,u"Tester",
      [&](const inv::Profile&,std::uint64_t)->std::optional<std::uint64_t>{++saves;return 14;});
  check(!denied&&saves==2&&revision==13,"exhausted session allocated collector reward from reserved wire pool");
 }
 const auto armor=std::find_if(offers.begin(),offers.end(),[](const auto& o){return o.collector=="Savich";});check(armor!=offers.end()&&armor->profession==6,"native346 armor profession preserved");
 {
  // Exact catalog rewards exercise pack admission without inventing recipes.
  for(const auto& offer:{pouch,offers[1]}){
   auto packed=inv::baseline();packed.bags.at(1).capacity=1;packed.bags.emplace(42,inv::Bag{42,1,4,5,0});packed.next_item=2;
   inv::Item trophy;trophy.definition=offer.trophy;trophy.definition.id=1;trophy.definition.quantity=offer.cost+1;trophy.bag=1;
   packed.items.emplace(1,trophy);const auto before=inv::encode(packed);
   check(!c::exchange(packed,offer,offer.map,6,u"Tester")&&inv::encode(packed)==before,
         "pouch and egg rewards cannot count empty equipment pack as capacity or debit partial trophy stack");
   inv::identity::Session ids;auto wire=ids.hydrate(packed,fw::gw::storage::baseline());std::uint64_t revision=10;unsigned saves=0;
   const auto wire_before=inv::encode(wire.character);const auto identity_before=ids.revision();
   const auto denied=c::commit_exchange(ids,wire.character,wire.bank,revision,offer,offer.map,6,u"Tester",
       [&](const inv::Profile&,std::uint64_t)->std::optional<std::uint64_t>{++saves;return 11;});
   check(!denied&&!saves&&revision==10&&ids.revision()==identity_before&&inv::encode(wire.character)==wire_before,
         "pack capacity rejection never invokes collector CAS or changes session identities");
   if(offer.reward.definition.interaction&0x80000){
    auto legacy=offer.reward;legacy.definition.id=2;legacy.definition.quantity=100;legacy.bag=42;legacy.slot=0;
    packed.items.emplace(2,legacy);packed.next_item=3;const auto legacy_before=inv::encode(packed);
    check(!c::exchange(packed,offer,offer.map,6,u"Tester")&&inv::encode(packed)==legacy_before,
          "legacy egg stack in equipment pack cannot absorb collector reward");
   }
   packed.bags.emplace(43,inv::Bag{43,1,2,1,0});
   const auto ordinary=c::exchange(packed,offer,offer.map,6,u"Tester");
   check(ordinary&&ordinary->profile.items.at(ordinary->reward).bag==43&&ordinary->profile.items.at(1).definition.quantity==1,
         "collector skips disallowed pack and places reward in later ordinary bag with exact debit");
  }
  auto packed=inv::baseline();packed.bags.at(1).capacity=1;packed.bags.emplace(42,inv::Bag{42,1,4,5,0});packed.next_item=2;
  inv::Item trophy;trophy.definition=armor->trophy;trophy.definition.id=1;trophy.definition.quantity=armor->cost+1;trophy.bag=1;packed.items.emplace(1,trophy);
  const auto own=c::exchange(packed,*armor,armor->map,6,u"Tester");
  check(own&&own->profile.items.at(2).bag==42&&own->profile.items.at(2).customized==u"Tester"&&own->profile.items.at(1).definition.quantity==1,
        "collector applies actual owner customization before checking armor pack admission");
  check(inv::decode(inv::encode(own->profile)).items.at(2).customized==u"Tester",
        "collector pack armor owner and placement survive persistence");
 }
 p=inv::baseline();p.next_item=2;t.definition=armor->trophy;t.definition.id=1;t.definition.quantity=3;p.items={{1,t}};
 check(!c::exchange(p,*armor,161,1,u"Tester"),"wrong profession armor denied");check(!c::exchange(p,*armor,161,6,u""),"armor owner required");
 result=c::exchange(p,*armor,161,6,u"Tester");check(result&&result->profile.items.at(2).customized==u"Tester","armor customized on issued item");
 check(result->profile.items.at(2).definition.modifiers==std::vector<std::uint32_t>({0x62c80500,0xa3c80700}),"exact armor7 energy5 modifiers retained");
 c::Menu menu{42,146,7,100,"Brownlow",{1,2}};check(menu.accepts(1,42,146,7,99),"current selected menu allowed");
 check(!menu.accepts(3,42,146,7,99)&&!menu.accepts(1,43,146,7,99)&&!menu.accepts(1,42,161,7,99)&&!menu.accepts(1,42,146,8,99)&&!menu.accepts(1,42,146,7,100),"menu offer actor map revision expiry guards");
 menu.clear();check(!menu.accepts(1,42,146,7,99),"closed menu rejects replay");
 {
  // Exercise the same dialogue -> identity projection -> durable CAS path
  // used by gw_auth, including a bank identity collision and failed save.
  auto durable=inv::baseline();durable.next_item=2;t.definition=pouch.trophy;t.definition.id=1;t.definition.quantity=5;t.bag=1;t.slot=0;durable.items={{1,t}};
  auto bank=fw::gw::storage::baseline();bank.next_item=2;t.bag=100;bank.items={{1,t}};
  inv::identity::Session ids;auto wire=ids.hydrate(durable,bank);std::uint64_t revision=4;
  const auto original=inv::encode(wire.character),original_bank=inv::encode(wire.bank);
  c::Menu selected;auto packets=c::open_menu(selected,offers,wire.character,42,146,6,"Brownlow [Collector]",revision,100);
  check(packets.size()==5&&selected.offers==std::vector<unsigned>({1,2}),"shared native dialogue contains exact Brownlow choices and cancel");
  fw::gw::items::Packet click;fw::gw::items::put(click,57,2);fw::gw::items::put(click,c::choose_token+1,4);
  const auto token=fw::gw::dialogue::selection(click);
  const auto* chosen=c::select_menu(selected,offers,token,42,146,6,"Brownlow [Collector]",revision,101);
  check(chosen&&chosen->key==1&&!selected.actor,"native selection consumes active collector authority");
  unsigned saves=0;auto failed=c::commit_exchange(ids,wire.character,wire.bank,revision,*chosen,146,6,u"Tester",
   [&](const inv::Profile& proposed,std::uint64_t expected)->std::optional<std::uint64_t>{++saves;check(expected==4&&proposed.items.size()==1&&proposed.items.begin()->second.definition.model==33,"save sees durable complete exchange");return {};});
  check(!failed&&saves==1&&revision==4&&inv::encode(wire.character)==original&&inv::encode(wire.bank)==original_bank,"failed CAS publishes nothing and changes no live identities or ownership");
  check(!c::select_menu(selected,offers,token,42,146,6,"Brownlow",revision,102),"failed CAS cannot reuse consumed dialogue click");
  c::open_menu(selected,offers,wire.character,42,146,6,"Brownlow",revision,110);
  check(!c::select_menu(selected,offers,token,42,146,6,"Jacobs [Collector]",revision,111),"reused actor with shared collector model but different name denied");
  c::open_menu(selected,offers,wire.character,42,146,6,"Brownlow",revision,120);
  check(!c::select_menu(selected,offers,token,42,146,6,"Brownlow",revision+1,121),"intervening inventory mutation invalidates menu");
  c::open_menu(selected,offers,wire.character,42,146,6,"Brownlow",revision,130);
  chosen=c::select_menu(selected,offers,token,42,146,6,"Brownlow",revision,131);
  auto committed=c::commit_exchange(ids,wire.character,wire.bank,revision,*chosen,146,6,u"Tester",
   [&](const inv::Profile& proposed,std::uint64_t expected)->std::optional<std::uint64_t>{check(expected==4,"CAS expected revision unchanged after failures");durable=proposed;return 5;});
  check(committed&&revision==5&&committed->packets.size()==5&&wire.character.items.size()==1,"successful CAS publishes exact debit and credited item");
  check(ids.origin(committed->reward)->logical==2&&durable.items.contains(2)&&durable.items.at(2).definition.model==33,"new owned reward binds durable allocator identity");
  auto after_bank=wire.bank;after_bank.next_item=fw::gw::inventory::decode(original_bank).next_item;
  check(inv::encode(after_bank)==original_bank,"same logical bank trophy remains untouched except shared wire cursor");
  c::open_menu(selected,offers,wire.character,44,146,6,"Kaylee  [Collector]",revision,140);
  check(selected.offers==std::vector<unsigned>({6,7,8,9}),"captured double-space Kaylee name maps exactly");
  check(c::open_menu(selected,offers,wire.character,44,161,1,"Savich [Collector]",revision,150).empty()&&!selected.actor,"unsupported armor profession exposes no invented offers");
 }
 std::cout<<"PASS 22 collector offers, exact trophy debit, rollback, armor permission and menu lifecycle\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

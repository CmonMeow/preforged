#include "gw_collector_transaction.h"
#include "gw_presearing_collectors.h"
#include <fstream>
#include <iostream>
#include <map>

int main(int argc,char** argv)try{
 namespace n=fw::gw::collectors::native;namespace c=fw::gw::collectors;namespace inv=fw::gw::inventory;
 const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 check(argc==2,"native transaction fixture path required");
 std::ifstream input(argv[1]);std::map<std::string,std::vector<std::uint8_t>> fixtures;
 std::string name,hex;while(input>>name>>hex){std::vector<std::uint8_t> bytes;
  for(std::size_t i=0;i<hex.size();i+=2)bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
  fixtures.emplace(name,std::move(bytes));}
 check(fixtures.size()==4,"four original sender fixtures required");
 const auto full=n::decode(fixtures.at("whole_stack"));
 check(full.opcode==72&&full.type==2&&full.give.size()==1&&full.give[0].item==42&&
       !full.give[0].quantity&&full.receive[0].item==585,"native whole stack layout");
 const auto partial=n::decode(fixtures.at("partial_split_stacks"));
 check(partial.opcode==75&&partial.give.size()==2&&partial.give[0].quantity==5&&
       partial.give[1].quantity==2&&partial.receive[0].quantity==1,"native split quantities layout");
 auto rejects=[&](std::vector<std::uint8_t> bytes){bool rejected=false;try{n::decode(bytes);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"malformed request accepted");};
 auto bytes=fixtures.at("partial_stack");bytes[13]=0;rejects(bytes); // quantity count differs from IDs
 bytes=fixtures.at("partial_stack");bytes[15]=0;rejects(bytes); // zero quantity
 bytes=fixtures.at("whole_stack");bytes.pop_back();rejects(bytes);
 bytes=fixtures.at("whole_stack");bytes.push_back(0);rejects(bytes);
 bytes=fixtures.at("whole_stack");bytes[7]=17;rejects(bytes);
 const auto offers=c::presearing_offers();auto profile=inv::baseline();profile.next_item=45;
 for(const auto [id,quantity,slot]:{std::tuple{42u,8u,0u},std::tuple{43u,2u,1u},std::tuple{44u,5u,2u}}){
  inv::Item item;item.definition=offers[0].trophy;item.definition.id=id;item.definition.quantity=quantity;
  item.bag=1;item.slot=static_cast<std::uint8_t>(slot);profile.items.emplace(id,item);}
 const n::Binding original{100,146,99,7,10,1000,{{585,offers[0].key}}};
 const n::Observation observation{100,146,6,99,7,10,500,true};
 auto whole=full;whole.give={{44,0}};
 auto menu=original;const auto before=inv::encode(profile);
 const auto plan=n::authorize(menu,whole,offers,profile,observation);
 check(plan&&plan->offer->key==offers[0].key&&plan->debit.size()==1&&plan->debit[0].item==44&&
       plan->debit[0].quantity==5&&!menu.actor&&inv::encode(profile)==before,"whole selected stack exact authoritative debit");
 auto split=partial;split.give={{42,3},{43,2}};menu=original;
 const auto split_plan=n::authorize(menu,split,offers,profile,observation);
 check(split_plan&&split_plan->debit[0].quantity==3&&split_plan->debit[1].quantity==2,"partial selected stacks exact cost");
 const auto denied=[&](n::Request request,n::Observation current){auto active=original;
  check(!n::authorize(active,request,offers,profile,current)&&!active.actor,"forged/stale attempt must consume menu");};
 denied(full,observation); // whole stack42 is eight trophies, not one or five
 auto forged=whole;forged.give={{44,0},{44,0}};denied(forged,observation);
 forged=split;forged.give={{42,1}};denied(forged,observation);
 forged=split;forged.give[0].quantity=9;denied(forged,observation);
 forged=whole;forged.receive[0].item=999;denied(forged,observation);
 forged=whole;forged.give_gold=1;denied(forged,observation);
 forged=whole;forged.receive_gold=1;denied(forged,observation);
 forged=whole;forged.type=1;denied(forged,observation);
 forged=whole;forged.give[0].item=999;denied(forged,observation);
 for(unsigned changed=0;changed<7;++changed){auto current=observation;
  switch(changed){case 0:++current.actor;break;case 1:++current.map;break;case 2:++current.world;break;
   case 3:++current.npc_generation;break;case 4:++current.revision;break;case 5:current.now=1000;break;case 6:current.nearby=false;break;}
  denied(whole,current);}
 auto altered=profile;altered.items.at(44).definition.model++;menu=original;
 check(!n::authorize(menu,whole,offers,altered,observation),"wrong trophy metadata accepted");
 altered=profile;altered.bags.at(1).type=2;menu=original;
 check(!n::authorize(menu,whole,offers,altered,observation),"non-carried inventory accepted");
 menu=original;menu.offers.push_back(menu.offers[0]);
 check(!n::authorize(menu,whole,offers,profile,observation),"ambiguous menu mapping accepted");
 menu=original;check(n::authorize(menu,whole,offers,profile,observation).has_value()&&
  !n::authorize(menu,whole,offers,profile,observation),"menu replay accepted");
 menu=original;bytes=fixtures.at("whole_stack");bytes.pop_back();
 check(!n::authorize_wire(menu,bytes,offers,profile,observation)&&!menu.actor,"malformed wire retained menu authority");
 const auto selected=n::exchange_selected(*plan,profile,146,6,10,u"Tester");
 check(selected&&selected->profile.items.at(42).definition.quantity==8&&
       selected->profile.items.at(43).definition.quantity==2&&!selected->profile.items.contains(44)&&
       selected->profile.items.at(selected->reward).definition.file==offers[0].reward.definition.file&&
       inv::encode(profile)==before,"selected whole stack must be debited instead of earlier matching trophies");
 auto changed_profile=profile;changed_profile.items.at(42).definition.quantity--;
 check(!n::exchange_selected(*plan,changed_profile,146,6,10,u"Tester")&&
       !n::exchange_selected(*plan,profile,146,6,11,u"Tester"),"inventory or revision mutation after admission accepted");
 auto packed=profile;packed.bags.at(1).capacity=3;
 auto partial_one=split;partial_one.give={{42,5}};menu=original;
 const auto packed_plan=n::authorize(menu,partial_one,offers,packed,observation);
 check(bool(packed_plan),"packed partial debit should be admissible before reward allocation");
 const auto packed_before=inv::encode(packed);
 check(!n::exchange_selected(*packed_plan,packed,146,6,10,u"Tester")&&inv::encode(packed)==packed_before,
       "reward capacity failure leaked selected debit");
 // Hydrate authentic wire/durable ownership domains before exercising CAS.
 inv::identity::Session identities;auto wire=identities.hydrate(profile,fw::gw::storage::baseline());
 auto wire_request=whole;
 for(const auto& [id,item]:wire.character.items)if(item.definition.quantity==5)wire_request.give={{id,0}};
 menu=original;const auto commit_plan=n::authorize(menu,wire_request,offers,wire.character,observation);
 check(bool(commit_plan),"hydrated selected debit rejected");
 std::uint64_t revision=10;unsigned saves=0;const auto wire_before=inv::encode(wire.character);
 const auto bank_before=inv::encode(wire.bank);auto expected_bank=wire.bank;const auto ids_before=identities.revision();
 const auto failed=n::commit_selected_exchange(*commit_plan,identities,wire.character,wire.bank,revision,146,6,u"Tester",
  [&](const inv::Profile&,std::uint64_t expected)->std::optional<std::uint64_t>{++saves;check(expected==10,"wrong CAS revision");return {};});
 check(!failed&&saves==1&&revision==10&&identities.revision()==ids_before&&
       inv::encode(wire.character)==wire_before&&inv::encode(wire.bank)==bank_before,"failed CAS changed live inventory or identities");
 const auto committed=n::commit_selected_exchange(*commit_plan,identities,wire.character,wire.bank,revision,146,6,u"Tester",
  [&](const inv::Profile& durable,std::uint64_t expected)->std::optional<std::uint64_t>{++saves;
   check(expected==10&&durable.items.at(42).definition.quantity==8&&durable.items.at(43).definition.quantity==2&&
         !durable.items.contains(44)&&durable.items.at(45).definition.file==offers[0].reward.definition.file,
         "selected wire debit or trusted reward lost during durable projection");return 11;});
 // Both wire profiles share one monotonic allocator cursor. A new character
 // item advances that cursor in the bank projection without changing its
 // durable ownership, contents, bags, weapons, gold or material quota.
 expected_bank.next_item=wire.character.next_item;
 check(committed&&committed->reward&&saves==2&&revision==11&&identities.revision()==ids_before+1&&
       inv::encode(wire.bank)==inv::encode(expected_bank),"selected collector commit did not publish atomic projection");
 check(!n::commit_selected_exchange(*commit_plan,identities,wire.character,wire.bank,revision,146,6,u"Tester",
  [&](const inv::Profile&,std::uint64_t)->std::optional<std::uint64_t>{++saves;return 12;})&&saves==2,
  "stale admitted plan replay reached persistence");
 std::cout<<"PASS original72/75 framing, whole/partial selected debits, active-menu authority and replay rejection\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

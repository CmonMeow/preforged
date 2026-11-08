#include "gw_quest_message_from_friend.h"
#include "gw_quest_catalog.h"
#include "gw_quest_text_binding.h"
#include "gw_quest_menu.h"
#include <iostream>
#include <source_location>
int main(int argc,char** argv)try{
 using namespace fw::gw::quests;namespace message=fw::gw::quests::message_from_friend;
 unsigned count=0;auto check=[&](bool ok,const std::source_location where=std::source_location::current()){
  ++count;if(!ok)throw std::runtime_error("quest80 invariant line "+std::to_string(where.line()));};
 check(argc==2);const auto source=load_catalog(std::filesystem::path(argv[1]));
 check(source.size()==1&&source.at(80).captured_character_argument==std::optional<std::u16string>{u"Logger"});
 const auto catalog=message::bind_markers(bind_catalog_character(source,u"Other Character"),148);
 check(catalog.at(80).description!=source.at(80).description&&source.at(80).captured_character_argument.has_value());
 const message::Eligibility eligible{true,true};
 const message::Interaction crier{{1,10002,1},{1,10002,1},"Town Crier",148,true};
 const message::Interaction tydus{{1,10001,1},{1,10001,1},"Sir Tydus",148,true};
 for(auto primary:{1u,2u,3u,4u,5u,6u}){
  const auto offers=presearing::choices({},catalog,primary,0,148,"Town Crier");
  check(offers.size()==1&&offers[0].quest==80&&!offers[0].reward);
 }
 check(presearing::choices({},catalog,1,0,148,"Sir Tydus").empty());
 check(!message::accept({},0,eligible,crier));check(!message::accept({},2,eligible,crier));
 const auto active=*message::accept({},1,eligible,crier);
 check(message::valid_transition({},active));
 const auto turnin=presearing::choices(active,catalog,1,0,148,"Sir Tydus");
 check(turnin.size()==1&&turnin[0].reward&&turnin[0].cycle==1);
 const auto packets=restore_2022(decode(encode(active)),catalog);
 check(packets.size()==2&&packets[0][0]==72&&packets[1][0]==75);
 check(!message::reward(active,2,eligible,tydus));check(!message::reward(active,1,eligible,crier));
 for(auto npc:{crier,tydus})for(unsigned fault=0;fault<5;++fault){
  auto stale=npc;
  switch(fault){case 0:++stale.current.generation;break;case 1:++stale.current.instance;break;case 2:stale.reachable=false;break;
   case 3:stale.map=146;break;case 4:stale.name="Other NPC";break;}
  check(!message::accept({},1,eligible,stale));check(!message::reward(active,1,eligible,stale));
 }
 for(auto gate:{message::Eligibility{false,true},message::Eligibility{true,false}}){
  check(!message::accept({},1,gate,crier));check(!message::reward(active,1,gate,tydus));
 }
 for(auto quest:{35u,217u,218u,219u,220u,221u,222u})for(auto stage:{Stage::active,Stage::completed,Stage::rewarded,Stage::abandoned}){
  Profile preparation;preparation.records.emplace(quest,Progress{1,stage});
  const bool allowed=stage==Stage::active||stage==Stage::abandoned;
  check(message::accept(preparation,1,eligible,crier).has_value()==allowed);
  check(presearing::choices(preparation,catalog,1,0,148,"Town Crier").empty()!=allowed);
 }
 auto later=active;later.records.emplace(35,Progress{1,Stage::rewarded});
 check(message::reward(later,1,eligible,tydus).has_value()); // Admission gate is not retroactive revocation.
 const auto rewarded=*message::reward(active,1,eligible,tydus);
 check(rewarded.records.at(80).stage==Stage::rewarded&&!message::valid_transition(active,rewarded));
 check(!message::reward(rewarded,1,eligible,tydus));check(!message::accept(rewarded,2,eligible,crier));
 check(restore_2022(decode(encode(rewarded)),catalog).empty());
 auto injected=active;injected.records.at(80).stage=Stage::completed;
 check(!message::valid_transition(active,injected));
 const auto abandoned=*abandon(active,80,1);check(message::valid_transition(active,abandoned));
 const auto retried=*message::accept(abandoned,2,eligible,crier);check(message::valid_transition(abandoned,retried));
 check(!message::reward(retried,1,eligible,tydus));check(message::reward(retried,2,eligible,tydus).has_value());
 const auto lake=message::bind_markers(catalog,146);
 check(lake.at(80).entry.x==7559&&lake.at(80).entry.y==5596&&lake.at(80).entry.plane==0);
 check(catalog.at(80).entry.x==11715&&catalog.at(80).entry.y==3517&&catalog.at(80).entry.plane==26);
 for(auto map:{0u,145u,147u,160u,161u,162u,163u,164u,165u}){
  const auto unknown=message::bind_markers(catalog,map);
  check(marker_reply_2022(active,unknown,{80}).outcome==MarkerOutcome::unresolved);
 }
 std::cout<<"PASS quest80 "<<count<<" admission, NPC, cycle, binding, restore and marker checks\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

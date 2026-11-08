#include "gw_quest_path_to_glory.h"
#include "gw_quest_catalog.h"
#include "gw_quest_text_binding.h"
#include "gw_quest_menu.h"
#include <iostream>
#include <source_location>
int main(int argc,char** argv)try{
 using namespace fw::gw::quests;namespace glory=fw::gw::quests::path_to_glory;
 unsigned count=0;auto check=[&](bool ok,const std::source_location where=std::source_location::current()){
  ++count;if(!ok)throw std::runtime_error("quest89 invariant line "+std::to_string(where.line()));};
 check(argc==2);const auto source=load_catalog(std::filesystem::path(argv[1]));
 check(source.size()==1&&source.at(89).captured_character_argument==std::optional<std::u16string>{u"Logger"});
 const auto catalog=glory::bind_markers(bind_catalog_character(source,u"Different Hero"),148);
 check(catalog.at(89).description!=source.at(89).description&&source.at(89).captured_character_argument.has_value());
 const glory::Interaction armin{{1,10021,1},{1,10021,1},"Armin Saberlin",148,true};
 const glory::Interaction tydus{{1,10001,1},{1,10001,1},"Sir Tydus",148,true};
 constexpr unsigned tests[]{0,85,86,84,82,81,83};
 for(unsigned primary=1;primary<=6;++primary){
  const glory::Eligibility e{true,true,primary};
  check(!glory::accept({},1,e,armin));check(presearing::choices({},catalog,primary,0,148,"Armin Saberlin").empty());
  Profile seed;seed.records.emplace(tests[primary],Progress{1,Stage::completed});
  check(!glory::accept(seed,1,e,armin));seed.records.at(tests[primary]).stage=Stage::rewarded;
  check(!seed.records.contains(79)); // Neither79 nor a selected secondary is a prerequisite.
  check(presearing::choices(seed,catalog,primary,0,148,"Armin Saberlin").size()==1);
  check(!glory::accept(seed,0,e,armin));check(!glory::accept(seed,2,e,armin));
  const auto active=*glory::accept(seed,1,e,armin);check(glory::valid_transition(seed,active,e));
  const auto options=presearing::choices(active,catalog,primary,0,148,"Sir Tydus");
  check(options.size()==1&&options[0].reward&&options[0].quest==89);
  check(!glory::reward(active,2,e,tydus));check(!glory::reward(active,1,e,armin));
  for(unsigned fault=0;fault<5;++fault){auto wrong=tydus;
   switch(fault){case 0:++wrong.current.generation;break;case 1:++wrong.current.instance;break;
    case 2:wrong.reachable=false;break;case 3:wrong.map=146;break;case 4:wrong.name="Other NPC";break;}
   check(!glory::reward(active,1,e,wrong));
  }
  const auto restored=decode(encode(active));check(restored==active);
  const auto packets=restore_2022(restored,catalog);check(packets.size()==2&&packets[0][0]==72&&packets[1][0]==75);
  const auto rewarded=*glory::reward(restored,1,e,tydus);
  check(rewarded.records.at(89).stage==Stage::rewarded&&rewarded.records.at(tests[primary])==seed.records.at(tests[primary]));
  check(!glory::valid_transition(active,rewarded,e));check(!glory::reward(rewarded,1,e,tydus));
  check(restore_2022(decode(encode(rewarded)),catalog).empty());
  auto injected=active;injected.records.at(89).stage=Stage::completed;check(!glory::valid_transition(active,injected,e));
  const auto abandoned=*abandon(active,89,1);check(glory::valid_transition(active,abandoned,e));
  const auto retry=*glory::accept(abandoned,2,e,armin);check(glory::valid_transition(abandoned,retry,e));
  check(!glory::reward(retry,1,e,tydus));check(glory::reward(retry,2,e,tydus).has_value());
  for(auto bad:{glory::Eligibility{false,true,primary},glory::Eligibility{true,false,primary},glory::Eligibility{true,true,0},glory::Eligibility{true,true,7}}){
   check(!glory::accept(seed,1,bad,armin));check(!glory::reward(active,1,bad,tydus));
  }
 }
 Profile active;active.records.emplace(89,Progress{});
 for(auto map:{0u,145u,146u,147u,160u,161u,162u,163u,164u,165u}){
  const auto unobserved=glory::bind_markers(catalog,map);check(marker_reply_2022(active,unobserved,{89}).outcome==MarkerOutcome::unresolved);
 }
 check(catalog.at(89).entry.x==11715&&catalog.at(89).entry.y==3517&&catalog.at(89).entry.plane==26);
 std::cout<<"PASS quest89 "<<count<<" prerequisite, NPC, cycle, binding, persistence and marker checks\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

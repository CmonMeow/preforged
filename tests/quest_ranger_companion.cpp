#include "gw_quest_ranger_companion.h"
#include "gw_quest_skills.h"
#include "gw_quest_catalog.h"
#include "gw_presearing_items.h"
#include <iostream>
#include <source_location>
int main(int argc,char** argv)try{
 using namespace fw::gw;using namespace fw::gw::quests;namespace ranger=quests::ranger_companion;
 unsigned count=0;auto check=[&](bool ok,const std::source_location where=std::source_location::current()){
  ++count;if(!ok)throw std::runtime_error("quest76 invariant line "+std::to_string(where.line()));};
 check(argc==2);const auto catalog=load_catalog(std::filesystem::path(argv[1]));
 check(catalog.size()==1&&catalog.at(76).completed_variant&&catalog.at(76).completed_variant->objectives.has_value());
 const ranger::Interaction nente{{1,23001,1},{1,23001,1},"Master Ranger Nente",162,true};
 pets::Profile no_pet,stalker;stalker.pet=pets::Pet{};
 constexpr unsigned tests[]{0,85,86,84,82,81,83};
 for(unsigned primary=1;primary<=6;++primary){
  ranger::Eligibility e{true,true,primary,0};Profile seed;seed.records.emplace(tests[primary],Progress{1,Stage::completed});
  check(!ranger::accept(seed,1,e,no_pet,nente));seed.records.at(tests[primary]).stage=Stage::rewarded;
  check(!ranger::accept(seed,1,e,stalker,nente));check(!ranger::accept(seed,0,e,no_pet,nente));
  const auto active=*ranger::accept(seed,1,e,no_pet,nente);check(ranger::valid_transition(seed,active,e,no_pet));
  check(!ranger::complete(active,1,e,no_pet));check(!ranger::complete(active,2,e,stalker));
  const auto complete=*ranger::complete(active,1,e,stalker);check(ranger::valid_transition(active,complete,e,stalker));
  check(!ranger::valid_transition(seed,complete,e,stalker));check(!ranger::valid_transition(active,complete,e,no_pet));
  check(restore_2022(decode(encode(active)),catalog).size()==2);
  const auto complete_packets=restore_2022(decode(encode(complete)),catalog);check(complete_packets.size()==2);
  auto stale=nente;++stale.current.generation;check(!ranger::reward(complete,1,e,stalker,stale));
  stale=nente;stale.reachable=false;check(!ranger::accept(seed,1,e,no_pet,stale));
  const auto rewarded=*ranger::reward(complete,1,e,stalker,nente);check(!ranger::valid_transition(complete,rewarded,e,stalker));
  check(!ranger::reward(rewarded,1,e,stalker,nente));check(restore_2022(rewarded,catalog).empty());
  const auto abandoned=*abandon(complete,76,1);check(ranger::valid_transition(complete,abandoned,e,stalker));
  check(!ranger::accept(abandoned,2,e,stalker,nente));
  const auto retry=*ranger::accept(abandoned,2,e,no_pet,nente);check(!ranger::complete(retry,1,e,stalker));
  check(ranger::complete(retry,2,e,stalker).has_value());
  skills::Profile build;build.primary=static_cast<std::uint8_t>(primary);build.profession_mask=1u<<primary;
  if(primary==2){build.learned.emplace(446,1);build.bar[0]={446,0};}
  const auto taught=quests::teach_acceptance(build,76,ranger::grants,true,primary!=2);
  for(auto skill:ranger::grants)check(taught.learned.contains(skill));
  check(taught.secondary==0&&taught.profession_mask==build.profession_mask);
  if(primary!=2){check(taught.trial_skills==std::set<std::uint32_t>{436,446,411});
   const auto unlocked=quests::unlock_trial(taught,76);check(unlocked.profession_mask==(build.profession_mask|4u)&&unlocked.secondary==0);}
  else check(taught.bar[0]==skills::Slot{446,0}&&taught.trial_skills.empty());
 }
 {
  Profile seed;seed.records.emplace(85,Progress{1,Stage::rewarded});
  check(!ranger::accept(seed,1,{false,true,1,0},no_pet,nente));
  check(!ranger::accept(seed,1,{true,false,1,0},no_pet,nente));
  check(!ranger::accept(seed,1,{true,true,1,3},no_pet,nente));
  check(ranger::accept(seed,1,{true,true,1,2},no_pet,nente).has_value());
  auto wrong_map=nente;wrong_map.map=146;check(!ranger::accept(seed,1,{true,true,1,0},no_pet,wrong_map));
  auto artemis=nente;artemis.name="Artemis the Ranger";check(!ranger::accept(seed,1,{true,true,1,0},no_pet,artemis));
 }
 const auto base=inventory::baseline();const auto before=inventory::encode(base);
 const auto grant=ranger::grant_bow(base);check(grant&&grant->item);
 const auto& bow=grant->profile.items.at(*grant->item);
 check(bow.definition.file==2147493028u&&bow.definition.model==477&&bow.definition.type==5&&bow.definition.quantity==1);
 check(bow.definition.modifiers==std::vector<std::uint32_t>{639107840,616038656,2760377603}&&bow.profession==2&&!bow.customized);
 check(grant->packets.size()==3&&grant->packets[0]==items::definition_2022(ranger::starter_bow(*grant->item)));
 check(inventory::encode(base)==before&&ranger::has_starter_bow(grant->profile));
 const auto again=ranger::grant_bow(grant->profile);check(again&&!again->item&&again->packets.empty()&&inventory::encode(again->profile)==inventory::encode(grant->profile));
 const auto creation=inventory::presearing::creation_weapon(base,2,u"Ranger Hero");check(creation&&ranger::has_starter_bow(*creation));
 check(!ranger::grant_bow(*creation)->item);
 auto full=grant->profile;full.bags.at(1).capacity=1;full.items.at(*grant->item).definition.model=999;
 check(!ranger::grant_bow(full));
 Profile active;active.records.emplace(76,Progress{});
 for(auto map:{0u,145u,146u,148u,160u,161u,163u,164u,165u}){
  const auto unobserved=ranger::bind_markers(catalog,map);check(marker_reply_2022(active,unobserved,{76}).outcome==MarkerOutcome::unresolved);
 }
 std::cout<<"PASS quest76 "<<count<<" primary/trial, postacceptance adoption, exact bow, cycle and restore checks\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

#include "gw_quest_encounter_admission.h"
#include <array>
#include <utility>
#include <iostream>
#include <stdexcept>
int main()try{
 namespace q=fw::gw::quests::presearing;
 const auto check=[](bool ok){if(!ok)throw std::runtime_error("quest encounter offer admission");};
 for(const auto [quest,map]:std::array<std::pair<unsigned,unsigned>,6>{{{42,162},{71,146},{75,160},{81,146},{85,146},{86,146}}}){
  check(q::requires_kill_encounter(quest));
  check(!q::kill_encounter_offer_available(quest,map,{}));
  check(!q::kill_encounter_offer_available(quest,map,0));
  check(!q::kill_encounter_offer_available(quest,map,map+1));
  check(q::kill_encounter_offer_available(quest,map,map));
 }
 for(const auto [quest,offer,objective]:std::array<std::array<unsigned,3>,3>{{{37,165,161},{43,160,145},{49,165,161}}}){
  check(q::requires_kill_encounter(quest));
  check(!q::kill_encounter_offer_available(quest,offer,{}));
  check(q::kill_encounter_offer_available(quest,offer,objective));
 }
 // Dialogue, corpse, escort, loot and wave encounters have their own checks.
 for(unsigned quest:{55u,82u,84u,83u,73u}){
  check(!q::requires_kill_encounter(quest));
  check(q::kill_encounter_offer_available(quest,146,{}));
 }
 std::cout<<"PASS all transactional kill quest offers require installed encounter; cross-zone quests retain offer/objective separation\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

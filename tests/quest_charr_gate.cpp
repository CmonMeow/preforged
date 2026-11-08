#include "../src/runtime/gw_quest_charr_gate_reward.h"
#include "../src/runtime/gw_quest_catalog.h"
#include <fstream>
#include <iostream>
#include <type_traits>
int main(int argc,char** argv)try{
 namespace q=fw::gw::quests;namespace c=q::charr_gate;namespace inv=fw::gw::inventory;
 const auto check=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
 check(argc==3,"quest46 fixture and staged content directory required");
 static_assert(!std::is_default_constructible_v<c::Attempt>);
 std::ifstream fixture(argv[1]);std::string label,hex;fixture>>label>>hex;
 auto wire=fw::gw::items::definition_2022(c::long_sword(42));wire[1]|=0x80;
 std::string actual;constexpr char digits[]="0123456789abcdef";for(auto byte:wire){actual+=digits[byte>>4];actual+=digits[byte&15];}
 check(label=="reward"&&hex==actual,"captured reward differs from original2022 serializer");
 for(const char* phase:{"active","objective-zone","completed"}){
  std::ifstream source(std::filesystem::path(argv[2])/(std::string("quest46-")+phase+".fwqc"));
  const auto catalog=q::read_catalog(source);check(catalog.size()==1&&catalog.contains(46),"staged catalog framing failed");
 }
 const c::Scope owner{99,1,10},escort{99,35,20};
 const std::array<c::Scope,4> enemies{{{99,50,30},{99,51,31},{99,52,32},{99,53,33}}};
 const c::Eligibility eligible{true,true,2};const c::Interaction offer{{99,45,2},{99,45,2},"Prince Rurik",148,true};
 const c::Interaction recipient{escort,escort,"Prince Rurik",146,true};
 q::Profile state;const auto accepted=c::accept(state,1,eligible,offer);check(bool(accepted),"captured offer rejected");state=*accepted;
 auto wrong=offer;wrong.current.generation++;
 check(!c::accept({},1,{true,true,1},offer)&&!c::accept({},1,eligible,wrong)&&!c::accept({},2,eligible,offer)&&!c::accept({},1,{false,true},offer),"invalid acceptance admitted");
 auto attempt=c::Attempt::bind(state,owner,1,146,escort,escort,enemies);check(bool(attempt),"trusted roster admission failed");
 check(!c::Attempt::bind(state,owner,1,146,escort,{99,35,21},enemies),"stale selected escort admitted");
 auto duplicate=enemies;duplicate[3]=enemies[2];check(!c::Attempt::bind(state,owner,1,146,escort,escort,duplicate),"duplicate lifetime roster admitted");
 for(unsigned collision=0;collision<3;++collision){auto reused=enemies;
  reused[3]=collision==0?c::Scope{99,enemies[0].actor,999}:collision==1?c::Scope{99,owner.actor,999}:c::Scope{99,escort.actor,999};
  check(!c::Attempt::bind(state,owner,1,146,escort,escort,reused),"same actor under another generation admitted in roster");
 }
 const c::Scope recycled_escort{99,owner.actor,999};
 check(!c::Attempt::bind(state,owner,1,146,recycled_escort,recycled_escort,enemies),"owner actor reused as escort under another generation");
 auto foreign=enemies;foreign[2].instance++;
 check(!c::Attempt::bind(state,owner,1,146,escort,escort,foreign)&&!c::Attempt::bind(state,owner,1,148,escort,escort,enemies),"foreign objective roster admitted");
 auto inventory=inv::baseline();check(!c::reward(state,inventory,1,eligible,recipient),"active quest rewarded without completion authority");
 auto bare=state;bare.records.at(46).stage=q::Stage::completed;
 check(!c::reward(bare,inventory,1,eligible,recipient),"bare completed flag rewarded without trusted credits");
 check(!attempt->death(state,{99,1,11},enemies[0])&&!attempt->death(state,owner,{99,50,31}),"stale owner/enemy lifetime credited");
 // Trusted deaths can be caused by allies; no client-selected killer is input.
 state=*attempt->death(state,owner,enemies[0]);state=q::decode(q::encode(state));
 const auto after_first=c::remaining_enemies(state.records.at(46));
 check(after_first&&after_first->size()==3&&(*after_first)[0].index==1&&(*after_first)[0].credit==0x4603&&
       (*after_first)[1].index==2&&(*after_first)[2].index==3,"restored first credit selected wrong enemy identities");
 auto non_leading=*accepted;
 non_leading.records.at(46).objective_credits.insert(c::enemy_credits[2]);
 const auto middle=c::remaining_enemies(non_leading.records.at(46));
 check(middle&&middle->size()==3&&(*middle)[0].index==0&&(*middle)[1].index==1&&(*middle)[2].index==3&&
       (*middle)[2].credit==c::enemy_credits[3],"non-leading durable credit restored wrong enemy identity");
 non_leading.records.at(46).objective_credits.insert(0x4606);
 check(!c::remaining_enemies(non_leading.records.at(46)),"invalid durable enemy credit admitted");
 check(!attempt->death(state,owner,enemies[0]),"duplicate death credited");
 auto remaining=enemies;remaining[0]={};
 auto restored=c::Attempt::bind(state,owner,1,146,escort,escort,remaining);
 check(restored.has_value()&&!c::Attempt::bind(state,owner,1,146,escort,escort,enemies),"reconnect recreated already killed actor");
 for(unsigned i=1;i<3;++i){const auto credited=restored->death(state,owner,enemies[i]);check(bool(credited),"remaining scoped death rejected");state=*credited;}
 check(state.records.at(46).stage==q::Stage::active&&!c::reward(state,inventory,1,eligible,recipient),"three deaths bypassed fourth objective");
 state=*restored->death(state,owner,enemies[3]);state=q::decode(q::encode(state));
 check(state.records.at(46).stage==q::Stage::completed&&c::valid_progress(state.records.at(46)),"four scoped deaths did not complete without an invented arrival gate");
 auto legacy=state;legacy.records.at(46).objective_credits.insert(0x4601);
 check(!c::reward(legacy,inventory,1,eligible,recipient),"legacy invented fifth credit accepted");
 check(!restored->death(state,owner,enemies[3]),"completed attempt accepted another event");
 wrong=recipient;wrong.reachable=false;check(!c::reward(state,inventory,1,eligible,wrong),"out-of-range reward admitted");
 check(!c::reward(state,inventory,2,eligible,recipient),"old quest cycle rewarded");
 const auto reward=c::reward(state,inventory,1,eligible,recipient);check(reward&&reward->xp==200&&reward->inventory.items.size()==1,"exact item/XP proposal absent");
 const auto& sword=reward->inventory.items.at(reward->item);
 check(sword.definition.modifiers==c::long_sword(1).modifiers&&!sword.profession&&!sword.customized,"opaque reward or unknown metadata changed");
 auto full=inventory;full.bags.at(1).capacity=1;auto filler=sword;filler.definition.id=1;filler.bag=1;filler.slot=0;full.items.emplace(1,filler);full.next_item=2;
 const auto full_before=inv::encode(full);
 check(!c::reward(state,full,1,eligible,recipient)&&inv::encode(full)==full_before,"full bag reward mutated live state");
 inv::identity::Session identities;auto projected=identities.hydrate(inventory,fw::gw::storage::baseline());
 c::Revisions revisions{4,7};const auto quest_before=q::encode(state),item_before=inv::encode(projected.character),bank_before=inv::encode(projected.bank);
 const auto identity_before=identities.revision();unsigned calls=0,xp=100;
 for(auto invalid:{c::Revisions{0,7},c::Revisions{4,0},c::Revisions{INT64_MAX,7},c::Revisions{4,UINT64_MAX}}){
  check(!c::commit(identities,state,projected.character,projected.bank,invalid,1,eligible,recipient,
   [&](const auto&,const auto&,unsigned,auto)->std::optional<c::Revisions>{throw std::runtime_error("invalid revision reached save");}),"invalid/exhausted revision admitted");
 }
 auto result=c::commit(identities,state,projected.character,projected.bank,revisions,1,eligible,recipient,
  [&](const q::Profile& quests,const inv::Profile& owned,unsigned amount,c::Revisions expected)->std::optional<c::Revisions>{++calls;
   check(expected==c::Revisions{4,7}&&amount==200&&owned.items.size()==1&&quests.records.at(46).stage==q::Stage::rewarded,"CAS did not receive atomic reward proposal");return {};});
 check(!result&&calls==1&&xp==100&&q::encode(state)==quest_before&&inv::encode(projected.character)==item_before&&
       inv::encode(projected.bank)==bank_before&&identities.revision()==identity_before&&revisions==c::Revisions{4,7},"failed CAS mutated reward state");
 q::Profile durable_quests;inv::Profile durable_inventory;
 result=c::commit(identities,state,projected.character,projected.bank,revisions,1,eligible,recipient,
  [&](const q::Profile& quests,const inv::Profile& owned,unsigned amount,c::Revisions expected)->std::optional<c::Revisions>{++calls;
   check(expected==c::Revisions{4,7}&&amount==200,"successful CAS lost expected revisions/XP");
   durable_quests=q::decode(q::encode(quests));durable_inventory=inv::decode(inv::encode(owned));xp+=amount;return c::Revisions{5,8};});
 check(result&&result->xp==200&&xp==300&&calls==2&&state.records.at(46).stage==q::Stage::rewarded&&projected.character.items.size()==1&&
       revisions==c::Revisions{5,8},"successful reward projection failed");
 auto after_bank=projected.bank;after_bank.next_item=inv::decode(bank_before).next_item;
 check(inv::encode(after_bank)==bank_before,"durable bank ownership changed with reward");
 check(!c::commit(identities,state,projected.character,projected.bank,revisions,1,eligible,recipient,
  [&](const auto&,const auto&,unsigned,auto)->std::optional<c::Revisions>{throw std::runtime_error("replay reached persistence");}),"reward replay succeeded");
 inv::identity::Session reconnect;const auto reloaded=reconnect.hydrate(durable_inventory,fw::gw::storage::baseline());
 check(!c::reward(durable_quests,reloaded.character,1,eligible,recipient)&&xp==300,"reconnect rewarded twice");
 auto abandoned=*q::abandon(*accepted,46,1);const auto reaccepted=c::accept(abandoned,2,eligible,offer);
 check(reaccepted&&!attempt->death(*reaccepted,owner,enemies[0]),"prior attempt credited reaccepted cycle");
 std::cout<<"PASS quest46 staged content, scoped completion, exact reward and atomic CAS/reconnect\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

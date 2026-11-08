#include "accounts.h"
#include "common.h"
#include "framework/module.h"
#include "gw_quest_state.h"
#include "gw_quest_timed_state.h"
#include "gw_quest_further_adventures.h"
#include "gw_quest_new_elementalist_trainer.h"
#include "gw_quest_charr_gate_reward.h"
#include "quest_state.h"
#include "inventory_state.h"
#include "gw_presearing_items.h"
#include "gw_presearing_collection.h"
#include "gw_weapon_intent.h"
#include "gw_profession.h"
#include "gw_quest_skills.h"
#include "gw_quest_menu.h"
#include <sodium.h>
#include <filesystem>
#include <iostream>
#include <source_location>
using namespace fw;
void check(bool ok,const std::source_location where=std::source_location::current()){if(!ok)throw std::runtime_error("account persistence invariant at line "+std::to_string(where.line()));}
std::string hex(std::span<const std::uint8_t> bytes){std::string out(bytes.size()*2+1,'\0');sodium_bin2hex(out.data(),out.size(),bytes.data(),bytes.size());out.pop_back();return out;}
int main(int argc,char** argv)try{
 check(argc==2&&sodium_init()>=0);auto dir=std::filesystem::absolute(argv[1]).lexically_normal();check(dir.filename()=="quest-accounts");std::filesystem::create_directories(dir);auto db=dir/"state.db";
 {
    auto inventory=gw::inventory::baseline();check(!gw::inventory::presearing::consume_shimmering_scale(inventory));
    check(gw::quests::presearing::restore_scale_encounter({1,gw::quests::Stage::active},inventory));
    check(gw::quests::presearing::restore_scale_encounter({1,gw::quests::Stage::completed},inventory));
    check(!gw::quests::presearing::restore_scale_encounter({1,gw::quests::Stage::rewarded},inventory));
    check(!gw::quests::presearing::restore_scale_encounter({1,gw::quests::Stage::abandoned},inventory));
    gw::inventory::Item scale;scale.bag=1;scale.slot=0;auto& d=scale.definition;
    d.id=inventory.next_item++;d.model=2566;d.file=23704;d.type=21;d.quantity=1;
    d.interaction=537399809;d.encoded_name={9558,52505,35705,32767};
    inventory.items.emplace(d.id,scale);gw::inventory::validate_character(inventory);
    check(!gw::quests::presearing::restore_scale_encounter({1,gw::quests::Stage::active},inventory));
    check(!gw::quests::presearing::restore_scale_encounter({1,gw::quests::Stage::completed},inventory));
    const auto consumed=gw::inventory::presearing::consume_shimmering_scale(inventory);
    check(consumed&&!consumed->items.contains(d.id)&&inventory.items.contains(d.id));
    check(gw::inventory::presearing::carried_shimmering_scale(inventory)==d.id);
    auto equipped=inventory;equipped.items.at(d.id).bag=equipped.bags.rbegin()->first;
    check(equipped.bags.at(equipped.items.at(d.id).bag).type==2);
    check(!gw::inventory::presearing::carried_shimmering_scale(equipped));
    auto wrong=inventory;wrong.items.at(d.id).definition.model=429;check(!gw::inventory::presearing::consume_shimmering_scale(wrong));
    wrong=inventory;wrong.items.at(d.id).definition.encoded_name={9558};check(!gw::inventory::presearing::consume_shimmering_scale(wrong));
    // Captured scales are individually carried quest items; no invented stack
    // interaction or captured numeric item ID is required for matching.
    auto alternate=inventory;auto renamed=alternate.items.extract(d.id);renamed.key()=alternate.next_item++;renamed.mapped().definition.id=renamed.key();alternate.items.insert(std::move(renamed));
    check(gw::inventory::presearing::consume_shimmering_scale(alternate).has_value());
 }
 for(auto suffix:{"","-wal","-shm","-journal"}){std::error_code error;std::filesystem::remove(db.string()+suffix,error);check(!error);}
 std::string owner,character,snapshot;
 {Accounts a(db.string());auto call=[&](std::string op,std::string payload,unsigned expected=FW_OK){auto r=a.execute(op,payload,[]{return true;});check(r.status==expected);return r.payload;};
 owner=split(call("register","QuestAlice\t71756573742d666978747572652d70617373776f7264"))[0];
 auto other=split(call("register","QuestBob\t71756573742d666978747572652d70617373776f7264"))[0];
 character=split(call("gw-character-create",owner+"\tQuest Character"))[0];auto key=owner+"\t"+character;
 const gwserver::OwnedCharacter owned{gwserver::Identity::from_hex(owner),gwserver::Identity::from_hex(character)};
 {
  namespace charr=gw::quests::charr_gate;
  const auto child=split(call("gw-character-create",owner+"\tCharr Reward Test"))[0];
  const gwserver::OwnedCharacter target{owned.account,gwserver::Identity::from_hex(child)};
  const auto baseline=gw::inventory::baseline();
  check(a.saveInventory({target,0,baseline},[]{return true;}).revision==1);
  gw::quests::Profile bare;bare.records.emplace(46,gw::quests::Progress{1,gw::quests::Stage::completed});
  check(a.saveQuests({target,0,bare},[]{return true;}).revision==1);
  auto complete=bare;auto& credits=complete.records.at(46).objective_credits;
  credits.insert(charr::enemy_credits.begin(),charr::enemy_credits.end());
  const charr::Interaction npc{{1,35,1},{1,35,1},"Prince Rurik",146,true};
  const auto proposed=charr::reward(complete,baseline,1,{true,true,2},npc);check(proposed.has_value());
  const auto before=a.characterNumber(target,false,[]{return true;});check(before.value.has_value());
  gwserver::RewardQuestExperience reward{target,1,46,1,200};reward.expected_inventory_revision=1;
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_INVALID); // no XP-only46 path
  reward.reward_inventory=proposed->inventory;
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_INVALID); // bare completed flag
  auto legacy=complete;legacy.records.at(46).objective_credits.insert(0x4601);
  check(a.saveQuests({target,1,legacy},[]{return true;}).revision==2);reward.expected_revision=2;
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_INVALID); // no guessed legacy fifth-credit acceptance
  check(a.saveQuests({target,2,complete},[]{return true;}).revision==3);reward.expected_revision=3;
  const auto unchanged=[&]{
   const auto quests=a.characterQuests(target,[]{return true;});const auto inventory=a.characterInventory(target,[]{return true;});
   check(quests.record->revision==3&&quests.record->profile==complete&&inventory.record->revision==1&&
         gw::inventory::encode(inventory.record->profile)==gw::inventory::encode(baseline)&&
         a.characterNumber(target,false,[]{return true;}).value==before.value);
  };
  for(unsigned field=0;field<6;++field){auto wrong=reward;
   switch(field){case 0:wrong.experience=201;break;case 1:wrong.learned_skills={194};break;
    case 2:++wrong.reward_inventory->items.begin()->second.definition.model;break;
    case 3:wrong.reward_inventory->items.begin()->second.profession=0;break;
    case 4:wrong.reward_inventory->items.begin()->second.customized=u"";break;
    case 5:wrong.reward_inventory->gold=1;break;}
   check(a.rewardQuestExperience(wrong,[]{return true;}).status==FW_INVALID);unchanged();
  }
  auto stale=reward;stale.expected_revision=2;
  check(a.rewardQuestExperience(stale,[]{return true;}).status==FW_CONFLICT);unchanged();
  stale=reward;stale.expected_inventory_revision=2;
  check(a.rewardQuestExperience(stale,[]{return true;}).status==FW_CONFLICT);unchanged();
  unsigned permission_checks=0;
  check(a.rewardQuestExperience(reward,[&]{return ++permission_checks<3;}).status==FW_TIMEOUT);unchanged();
  {Accounts reopened(db.string());check(reopened.characterQuests(target,[]{return true;}).record->profile==complete&&
    reopened.characterInventory(target,[]{return true;}).record->profile.items.empty()&&
    reopened.characterNumber(target,false,[]{return true;}).value==before.value);}
  check(a.rewardQuestExperience(reward,[]{return true;}).revision==4);
  check(a.characterNumber(target,false,[]{return true;}).value==*before.value+200);
  const auto issued=a.characterInventory(target,[]{return true;});
  check(issued.record->revision==2&&issued.record->profile.items.size()==1&&
        gw::inventory::encode(issued.record->profile)==gw::inventory::encode(proposed->inventory));
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  reward.expected_revision=4;reward.expected_inventory_revision=2;
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  {Accounts reopened(db.string());check(reopened.characterQuests(target,[]{return true;}).record->profile.records.at(46).stage==gw::quests::Stage::rewarded&&
    reopened.characterInventory(target,[]{return true;}).record->profile.items.size()==1&&
    reopened.characterNumber(target,false,[]{return true;}).value==*before.value+200);}
  // Exercise the actual existing storage XP-overflow boundary with a separate
  // trusted fixture character; quest46 itself still requests exactly200.
  const auto overflow_child=split(call("gw-character-create",owner+"\tCharr Overflow Test"))[0];
  const gwserver::OwnedCharacter overflow{owned.account,gwserver::Identity::from_hex(overflow_child)};
  auto high=complete;high.records.emplace(777,gw::quests::Progress{1,gw::quests::Stage::completed});
  check(a.saveQuests({overflow,0,high},[]{return true;}).revision==1);
  check(a.saveInventory({overflow,0,baseline},[]{return true;}).revision==1);
  check(a.rewardQuestExperience({overflow,1,777,1,UINT32_MAX-100},[]{return true;}).revision==2);
  gwserver::RewardQuestExperience overflow_reward{overflow,2,46,1,200};
  overflow_reward.expected_inventory_revision=1;overflow_reward.reward_inventory=proposed->inventory;
  check(a.rewardQuestExperience(overflow_reward,[]{return true;}).status==FW_INVALID);
  check(a.characterNumber(overflow,false,[]{return true;}).value==UINT32_MAX-100&&
        a.characterQuests(overflow,[]{return true;}).record->profile.records.at(46).stage==gw::quests::Stage::completed&&
        a.characterInventory(overflow,[]{return true;}).record->profile.items.empty());
 }
 {
  namespace further=fw::gw::quests::further_adventures;
  const auto child=split(call("gw-character-create",owner+"\tFurther Quest Test"))[0];
  const gwserver::OwnedCharacter target{owned.account,gwserver::Identity::from_hex(child)};
  const further::Eligibility eligible{true,true,1};
  const further::Interaction haversdan{{1,47,1},{1,47,1},"Haversdan",146,true};
  const further::Interaction devona{{1,112,1},{1,112,1},"Devona",146,true};
  gw::quests::Profile state;state.records.emplace(85,gw::quests::Progress{1,gw::quests::Stage::rewarded});
  state=*further::plan(state,further::Action::accept,1,eligible,haversdan);
  check(a.saveQuests({target,0,state},[]{return true;}).revision==1);
  const auto before=a.characterNumber(target,false,[]{return true;});check(before.value.has_value());
  gwserver::RewardQuestExperience reward{target,1,54,1,further::experience};
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  state=*further::plan(state,further::Action::talk_devona,1,eligible,devona);
  check(a.saveQuests({target,1,state},[]{return true;}).revision==2);
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  reward.expected_revision=2;
  unsigned checks=0;
  check(a.rewardQuestExperience(reward,[&]{return ++checks<3;}).status==FW_TIMEOUT);
  check(a.characterNumber(target,false,[]{return true;}).value==before.value);
  const auto pending=a.characterQuests(target,[]{return true;});
  check(pending.record->revision==2&&pending.record->profile==state);
  {Accounts reopened(db.string());
   check(reopened.characterQuests(target,[]{return true;}).record->profile==state);
   check(reopened.characterNumber(target,false,[]{return true;}).value==before.value);}
  check(a.rewardQuestExperience(reward,[]{return true;}).revision==3);
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  reward.expected_revision=3;
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  {Accounts reopened(db.string());
   check(reopened.characterQuests(target,[]{return true;}).record->profile.records.at(54).stage==gw::quests::Stage::rewarded);
   check(reopened.characterNumber(target,false,[]{return true;}).value==*before.value+250);}
 }
 {
  namespace trainer=fw::gw::quests::new_elementalist_trainer;
  const auto child=split(call("gw-character-create",owner+"\tTrainer Quest Test"))[0];
  const gwserver::OwnedCharacter target{owned.account,gwserver::Identity::from_hex(child)};
  const trainer::Eligibility eligible{true,true,6};
  const trainer::Interaction haversdan{{1,39,1},{1,39,1},"Haversdan",146,true};
  const trainer::Interaction aziure{{2,18,1},{2,18,1},"Elementalist Aziure",161,true};
  gw::quests::Profile state;state.records.emplace(54,gw::quests::Progress{1,gw::quests::Stage::rewarded});
  state=*trainer::plan(state,trainer::Action::accept,1,eligible,haversdan);
  check(a.saveQuests({target,0,state},[]{return true;}).revision==1);
  const auto before=a.characterNumber(target,false,[]{return true;});check(before.value.has_value());
  gwserver::RewardQuestExperience reward{target,1,65,1,trainer::experience};
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  state=*trainer::plan(state,trainer::Action::talk_aziure,1,eligible,aziure);
  check(a.saveQuests({target,1,state},[]{return true;}).revision==2);
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  reward.expected_revision=2;
  unsigned checks=0;
  check(a.rewardQuestExperience(reward,[&]{return ++checks<3;}).status==FW_TIMEOUT);
  check(a.characterNumber(target,false,[]{return true;}).value==before.value);
  check(a.characterQuests(target,[]{return true;}).record->profile==state);
  {Accounts reopened(db.string());
   check(reopened.characterQuests(target,[]{return true;}).record->revision==2&&
         reopened.characterQuests(target,[]{return true;}).record->profile==state);}
  check(a.rewardQuestExperience(reward,[]{return true;}).revision==3);
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  reward.expected_revision=3;
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  {Accounts reopened(db.string());
   check(reopened.characterQuests(target,[]{return true;}).record->profile.records.at(65).stage==gw::quests::Stage::rewarded&&
         reopened.characterQuests(target,[]{return true;}).record->profile.records.at(54).stage==gw::quests::Stage::rewarded);
   check(reopened.characterNumber(target,false,[]{return true;}).value==*before.value+250);}
 }
 {
  const auto child=split(call("gw-character-create",owner+"\tMesmer Trial Test"))[0];
  const gwserver::OwnedCharacter target{owned.account,gwserver::Identity::from_hex(child)};
  gw::skills::Profile learner;learner.primary=1;learner.profession_mask=2;
  call("gw-character-skills-import",owner+"\t"+child+"\t0\t"+hex(gw::skills::encode_profile(learner))+"\t"+std::string(64,'a'));
  gw::quests::Profile state;state.records.emplace(85,gw::quests::Progress{1,gw::quests::Stage::rewarded});
  gw::quests::Catalog menu_content;menu_content.emplace(71,gw::quests::Content{});
  check(gw::quests::presearing::choices({},menu_content,1,0,146,"Lady Althea",2).empty());
  const auto offered=gw::quests::presearing::choices(state,menu_content,1,0,146,"Lady Althea",2);
  check(offered.size()==1&&offered.front().quest==71&&!offered.front().reward);
  check(a.saveQuests({target,0,state},[]{return true;}).revision==1);
  state=*gw::quests::accept(state,71);
  gwserver::SaveQuests acceptance{target,1,state,1,{31,40,76}};
  acceptance.acceptance_fill_skillbar=true;acceptance.acceptance_trial=true;
  check(a.saveQuests(acceptance,[]{return true;}).revision==2);
  const auto trial=a.characterSkills(target,[]{return true;}).record;
  check(trial&&trial->profile.trial_skills==std::set<std::uint32_t>{31,40,76}&&trial->profile.profession_mask==2);
  check(gw::professions::propose({42,5,0},trial->profile,{}, {42,3,0x1e,1,false,true}).status==gw::professions::Reason::locked);
  // The World encounter supplies the stable Rogue Bull credit. This test
  // exercises the real reward/storage boundary rather than fabricating an NPC kill.
  state=*gw::quests::credit_objective(state,71,1,0x7101,1);
  const auto turnin=gw::quests::presearing::choices(state,menu_content,1,0,146,"Lady Althea",2);
  check(turnin.size()==1&&turnin.front().reward);
  check(a.saveQuests({target,2,state},[]{return true;}).revision==3);
  const auto before=a.characterNumber(target,false,[]{return true;});check(before.value.has_value());
  gwserver::RewardQuestExperience reward{target,3,71,1,250,trial->revision,{},0,0,false,false,true};
  auto stale=reward;++stale.expected_skill_revision;
  check(a.rewardQuestExperience(stale,[]{return true;}).status==FW_CONFLICT);
  unsigned checks=0;check(a.rewardQuestExperience(reward,[&]{return ++checks<3;}).status==FW_TIMEOUT);
  check(a.characterQuests(target,[]{return true;}).record->profile==state&&
        gw::skills::encode_profile(a.characterSkills(target,[]{return true;}).record->profile)==gw::skills::encode_profile(trial->profile)&&
        a.characterNumber(target,false,[]{return true;}).value==before.value);
  check(a.rewardQuestExperience(reward,[]{return true;}).revision==4);
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  // Story79 acceptance before secondary selection must not treat trial unlock
  // or a failed selected-profession save as a completed objective.
  const auto story_before=a.characterQuests(target,[]{return true;}).record;
  const auto story_active=*gw::quests::accept(story_before->profile,79);
  auto injected=story_active;injected.records.at(79).stage=gw::quests::Stage::completed;
  check(a.saveQuests({target,story_before->revision,injected},[]{return true;}).status==FW_INVALID);
  check(a.saveQuests({target,story_before->revision,story_active},[]{return true;}).revision==5);
  check(a.saveQuests({target,5,injected},[]{return true;}).status==FW_INVALID);
  {Accounts reopened(db.string());
   const auto skills=reopened.characterSkills(target,[]{return true;}).record;
   check(reopened.characterQuests(target,[]{return true;}).record->profile.records.at(71).stage==gw::quests::Stage::rewarded);
   check(reopened.characterNumber(target,false,[]{return true;}).value==*before.value+250);
   check(skills&&skills->profile.secondary==0&&skills->profile.profession_mask==(2u|32u)&&
         skills->profile.learned==trial->profile.learned&&skills->profile.bar==trial->profile.bar);
   const auto selected=gw::professions::propose({42,5,0},skills->profile,{}, {42,3,0x1e,1,false,true});
   check(selected.accepted());
   gwserver::SaveSecondaryProfession selection{target,skills->revision,0,selected.skill_profile,selected.attribute_profile};
   auto stale_selection=selection;++stale_selection.skill_revision;
   check(reopened.saveSecondaryProfession(stale_selection,[]{return true;}).status==FW_CONFLICT);
   check(reopened.saveQuests({target,5,injected},[]{return true;}).status==FW_INVALID);
   check(reopened.saveSecondaryProfession(selection,[]{return true;}).status==FW_OK);
  }
  {Accounts reopened(db.string());const auto skills=reopened.characterSkills(target,[]{return true;}).record;
   check(skills&&skills->profile.secondary==5&&skills->profile.trial_skills.empty()&&
         skills->profile.learned==trial->profile.learned&&skills->profile.bar==trial->profile.bar);
   check(reopened.characterNumber(target,false,[]{return true;}).value==*before.value+250);
   check(reopened.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
   const auto active=reopened.characterQuests(target,[]{return true;}).record;
   check(active->revision==5&&active->profile.records.at(79).stage==gw::quests::Stage::active);
   const auto completed=*gw::quests::second_profession::plan(active->profile,gw::quests::second_profession::Action::complete,1,{true,true,1,5});
   check(reopened.saveQuests({target,4,completed},[]{return true;}).status==FW_CONFLICT);
   unsigned completion_checks=0;
   check(reopened.saveQuests({target,5,completed},[&]{return ++completion_checks<3;}).status==FW_TIMEOUT);
   check(reopened.characterQuests(target,[]{return true;}).record->profile==active->profile);
   check(reopened.saveQuests({target,5,completed},[]{return true;}).revision==6);
   gwserver::RewardQuestExperience story_reward{target,6,79,1,250};
   for(unsigned mutation=0;mutation<5;++mutation){auto wrong=story_reward;
    switch(mutation){case 0:wrong.experience=251;break;case 1:wrong.gold=1;break;
     case 2:wrong.learned_skills={31};break;case 3:wrong.unlock_trial=true;break;case 4:wrong.fill_skillbar=true;break;}
    check(reopened.rewardQuestExperience(wrong,[]{return true;}).status==FW_INVALID);
   }
   unsigned reward_checks=0;
   check(reopened.rewardQuestExperience(story_reward,[&]{return ++reward_checks<3;}).status==FW_TIMEOUT);
   check(reopened.characterNumber(target,false,[]{return true;}).value==*before.value+250);
   check(reopened.characterQuests(target,[]{return true;}).record->profile.records.at(79).stage==gw::quests::Stage::completed);
   check(reopened.rewardQuestExperience(story_reward,[]{return true;}).revision==7);
   check(reopened.rewardQuestExperience(story_reward,[]{return true;}).status==FW_CONFLICT);
   {Accounts again(db.string());
    check(again.characterQuests(target,[]{return true;}).record->profile.records.at(79).stage==gw::quests::Stage::rewarded);
    check(again.characterNumber(target,false,[]{return true;}).value==*before.value+500);
    check(again.characterSkills(target,[]{return true;}).record->profile.secondary==5);
   }
  }
 }
 {
  const auto child=split(call("gw-character-create",owner+"\tMessage Quest Test"))[0];
  const gwserver::OwnedCharacter target{owned.account,gwserver::Identity::from_hex(child)};
  const auto baseline=gw::inventory::baseline();
  check(a.saveInventory({target,0,baseline},[]{return true;}).revision==1);
  namespace message=gw::quests::message_from_friend;
  const message::Interaction crier{{1,10002,1},{1,10002,1},"Town Crier",148,true};
  const auto active=*message::accept({},1,{true,true},crier);
  check(a.saveQuests({target,0,active},[]{return true;}).revision==1);
  for(auto stage:{gw::quests::Stage::completed,gw::quests::Stage::rewarded}){
   auto injected=active;injected.records.at(80).stage=stage;
   check(a.saveQuests({target,1,injected},[]{return true;}).status==FW_INVALID);
  }
  const auto before=a.characterNumber(target,false,[]{return true;});check(before.value.has_value());
  gwserver::RewardQuestExperience reward{target,1,80,1,100};reward.gold=10;reward.expected_inventory_revision=1;
  for(unsigned mutation=0;mutation<6;++mutation){auto wrong=reward;
   switch(mutation){case 0:wrong.experience=101;break;case 1:wrong.gold=9;break;case 2:wrong.gold=0;break;
    case 3:wrong.learned_skills={1};break;case 4:wrong.unlock_trial=true;break;case 5:wrong.reward_inventory=baseline;break;}
   check(a.rewardQuestExperience(wrong,[]{return true;}).status==FW_INVALID);
  }
  auto stale=reward;++stale.expected_revision;
  check(a.rewardQuestExperience(stale,[]{return true;}).status==FW_CONFLICT);
  stale=reward;++stale.expected_inventory_revision;
  check(a.rewardQuestExperience(stale,[]{return true;}).status==FW_CONFLICT);
  auto full=baseline;full.gold=99995;
  check(a.saveInventory({target,1,full},[]{return true;}).revision==2);
  reward.expected_inventory_revision=2;
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  check(a.characterQuests(target,[]{return true;}).record->profile==active&&a.characterNumber(target,false,[]{return true;}).value==before.value);
  check(a.saveInventory({target,2,baseline},[]{return true;}).revision==3);
  reward.expected_inventory_revision=3;unsigned checks=0;
  check(a.rewardQuestExperience(reward,[&]{return ++checks<3;}).status==FW_TIMEOUT);
  {Accounts reopened(db.string());
   check(reopened.characterQuests(target,[]{return true;}).record->profile==active);
   check(reopened.characterInventory(target,[]{return true;}).record->revision==3&&reopened.characterInventory(target,[]{return true;}).record->profile.gold==0);
   check(reopened.characterNumber(target,false,[]{return true;}).value==before.value);
  }
  check(a.rewardQuestExperience(reward,[]{return true;}).revision==2);
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  {Accounts reopened(db.string());
   check(reopened.characterQuests(target,[]{return true;}).record->profile.records.at(80).stage==gw::quests::Stage::rewarded);
   check(reopened.characterInventory(target,[]{return true;}).record->revision==4&&reopened.characterInventory(target,[]{return true;}).record->profile.gold==10);
   check(reopened.characterNumber(target,false,[]{return true;}).value==*before.value+100);
  }
 }
 {
  namespace glory=gw::quests::path_to_glory;
  const auto glory_account=split(call("register","QuestGlory\t71756573742d666978747572652d70617373776f7264"))[0];
  const auto child=split(call("gw-character-create",glory_account+"\tGlory Quest Test"))[0];
  const gwserver::OwnedCharacter target{gwserver::Identity::from_hex(glory_account),gwserver::Identity::from_hex(child)};
  gw::skills::Profile learner;learner.primary=3;learner.profession_mask=8;
  call("gw-character-skills-import",glory_account+"\t"+child+"\t0\t"+hex(gw::skills::encode_profile(learner))+"\t"+std::string(64,'a'));
  gw::quests::Profile state;state.records.emplace(84,gw::quests::Progress{1,gw::quests::Stage::completed});
  check(a.saveQuests({target,0,state},[]{return true;}).revision==1);
  const auto premature=*gw::quests::accept(state,89);
  check(a.saveQuests({target,1,premature},[]{return true;}).status==FW_INVALID);
  state.records.at(84).stage=gw::quests::Stage::rewarded;
  check(a.saveQuests({target,1,state},[]{return true;}).revision==2);
  const glory::Interaction armin{{1,10021,1},{1,10021,1},"Armin Saberlin",148,true};
  state=*glory::accept(state,1,{true,true,3},armin);
  check(a.saveQuests({target,2,state},[]{return true;}).revision==3);
  for(auto stage:{gw::quests::Stage::completed,gw::quests::Stage::rewarded}){auto injected=state;injected.records.at(89).stage=stage;
   check(a.saveQuests({target,3,injected},[]{return true;}).status==FW_INVALID);}
  const auto before=a.characterNumber(target,false,[]{return true;});const auto map_before=a.characterNumber(target,true,[]{return true;});
  check(before.value.has_value()&&map_before.value.has_value());
  gwserver::RewardQuestExperience reward{target,3,89,1,100};
  for(unsigned mutation=0;mutation<6;++mutation){auto wrong=reward;
   switch(mutation){case 0:wrong.experience=101;break;case 1:wrong.gold=1;break;case 2:wrong.learned_skills={1};break;
    case 3:wrong.unlock_trial=true;break;case 4:wrong.fill_skillbar=true;break;case 5:wrong.reward_inventory=gw::inventory::baseline();break;}
   check(a.rewardQuestExperience(wrong,[]{return true;}).status==FW_INVALID);
  }
  auto stale=reward;stale.expected_revision=2;check(a.rewardQuestExperience(stale,[]{return true;}).status==FW_CONFLICT);
  unsigned checks=0;check(a.rewardQuestExperience(reward,[&]{return ++checks<3;}).status==FW_TIMEOUT);
  check(a.characterNumber(target,false,[]{return true;}).value==before.value&&a.characterQuests(target,[]{return true;}).record->profile==state);
  const auto abandoned=*gw::quests::abandon(state,89,1);
  check(a.saveQuests({target,3,abandoned},[]{return true;}).revision==4);
  state=*glory::accept(abandoned,2,{true,true,3},armin);
  check(a.saveQuests({target,4,state},[]{return true;}).revision==5);
  reward.expected_revision=5;check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  reward.cycle=2;check(a.rewardQuestExperience(reward,[]{return true;}).revision==6);
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  {Accounts reopened(db.string());
   const auto quests=reopened.characterQuests(target,[]{return true;}).record;
   check(quests&&quests->profile.records.at(89).stage==gw::quests::Stage::rewarded&&quests->profile.records.at(89).cycle==2);
   check(reopened.characterNumber(target,false,[]{return true;}).value==*before.value+100);
   check(reopened.characterNumber(target,true,[]{return true;}).value==map_before.value);
   check(reopened.characterSkills(target,[]{return true;}).record->profile.secondary==0&&!quests->profile.records.contains(79));
   check(!reopened.characterInventory(target,[]{return true;}).record); // XP-only reward requires no inventory creation.
  }
 }
 {
  const auto timer_account=split(call("register","QuestTimer\t71756573742d666978747572652d70617373776f7264"))[0];
  const auto timer_character=split(call("gw-character-create",timer_account+"\tTimed Quest Test"))[0];
  const gwserver::OwnedCharacter timer_owner{gwserver::Identity::from_hex(timer_account),gwserver::Identity::from_hex(timer_character)};
  auto timer=*gw::quests::arm_marksmanship(*gw::quests::accept({},42),1,100000);
  gw::skills::Profile learner;learner.primary=6;learner.profession_mask=1u<<6;
  const auto timer_key=timer_account+"\t"+timer_character;
  call("gw-character-skills-import",timer_key+"\t0\t"+hex(gw::skills::encode_profile(learner))+"\t"+std::string(64,'a'));
  const auto baseline=gw::inventory::baseline();
  check(a.saveInventory({timer_owner,0,baseline},[]{return true;}).revision==1);
  const auto bow=*gw::inventory::presearing::grant_starter_bow(baseline,u"Timed Quest Test");
  gwserver::SaveQuests acceptance{timer_owner,0,timer,1,{407,432}};
  acceptance.acceptance_fill_skillbar=true;acceptance.acceptance_trial=true;
  acceptance.acceptance_inventory=bow;acceptance.expected_inventory_revision=2;
  check(a.saveQuests(acceptance,[]{return true;}).status==FW_CONFLICT);
  check(!a.characterQuests(timer_owner,[]{return true;}).record&&a.characterSkills(timer_owner,[]{return true;}).record->profile.learned.empty());
  acceptance.expected_inventory_revision=1;
  check(a.saveQuests(acceptance,[]{return true;}).revision==1);
  check(gw::inventory::encode(a.characterInventory(timer_owner,[]{return true;}).record->profile)==gw::inventory::encode(bow));
  check(a.characterSkills(timer_owner,[]{return true;}).record->profile.trial_skills.contains(432));
  {Accounts reopened(db.string());const auto loaded=reopened.characterQuests(timer_owner,[]{return true;});
   check(loaded.record&&loaded.record->profile==timer&&gw::quests::marksmanship_elapsed(loaded.record->profile.records.at(42),135000)==35000);}
  const auto credited_timer=gw::quests::credit_objective(timer,42,1,1,5,140000);
  check(credited_timer.has_value());timer=*credited_timer;
  check(a.saveQuests({timer_owner,1,timer},[]{return true;}).revision==2);
  auto expired=*gw::quests::expire_marksmanship(timer,1,190000);
  check(a.saveQuests({timer_owner,2,expired},[]{return true;}).revision==3);
  check(a.saveQuests({timer_owner,2,timer},[]{return true;}).status==FW_CONFLICT);
  {Accounts reopened(db.string());const auto loaded=reopened.characterQuests(timer_owner,[]{return true;});
   check(loaded.record&&loaded.record->revision==3&&loaded.record->profile==expired&&
         !gw::quests::marksmanship_elapsed(loaded.record->profile.records.at(42),200000)&&
         !gw::quests::arm_marksmanship(loaded.record->profile,1,200000));}
 }
 check(a.characterSkills(owned,[]{return true;}).status==FW_OK&&!a.characterSkills(owned,[]{return true;}).record);
 check(a.saveSkillbar({owned,0,{}},[]{return true;}).status==FW_NOT_FOUND);
 gw::skills::Profile skill_fixture;skill_fixture.primary=8;skill_fixture.secondary=7;skill_fixture.profession_mask=0x6fe;
 skill_fixture.learned={{3,81},{1239,1}};skill_fixture.bar[0]={3,80};skill_fixture.bar[1]={1239,0};
 call("gw-character-skills-import",key+"\t0\t"+hex(gw::skills::encode_profile(skill_fixture))+"\t"+std::string(64,'a'));
 check(a.characterProgression(owned,[]{return true;}).status==FW_OK&&!a.characterProgression(owned,[]{return true;}).record);
 check(a.characterProgression({gwserver::Identity::from_hex(other),owned.character},[]{return true;}).status==FW_NOT_FOUND);
 unsigned progression_checks=0;
 check(a.saveProgression({owned,0,{}},[&]{return ++progression_checks<3;}).status==FW_TIMEOUT);
 check(progression_checks==3&&!a.characterProgression(owned,[]{return true;}).record);
 check(a.saveProgression({owned,0,{}},[]{return true;}).revision==1);
 check(a.saveProgression({owned,0,{}},[]{return true;}).status==FW_CONFLICT);
 check(a.characterProgression(owned,[]{return true;}).record->revision==1);
 check(a.accountFactions(owned.account,[]{return true;}).status==FW_OK&&!a.accountFactions(owned.account,[]{return true;}).record);
 check(a.accountFactions({},[]{return true;}).status==FW_NOT_FOUND);
 unsigned faction_checks=0;
 check(a.saveFactions({owned.account,0,{}},[&]{return ++faction_checks<3;}).status==FW_TIMEOUT);
 check(faction_checks==3&&!a.accountFactions(owned.account,[]{return true;}).record);
 check(a.saveFactions({owned.account,0,{}},[]{return true;}).revision==1);
 check(a.saveFactions({owned.account,0,{}},[]{return true;}).status==FW_CONFLICT);
 check(a.accountFactions(owned.account,[]{return true;}).record->revision==1);
 const auto initial_attributes=a.characterAttributes(owned,[]{return true;});check(initial_attributes.status==FW_OK&&!initial_attributes.record);
 check(a.characterAttributes({gwserver::Identity::from_hex(other),owned.character},[]{return true;}).status==FW_NOT_FOUND);
 check(a.characterAttributes(owned,[]{return false;}).status==FW_TIMEOUT);
 gw::attributes::Profile allocation;allocation.ranks[33]=1;
 auto forbidden_allocation=allocation;forbidden_allocation.quest_bonus=30;
 check(a.saveAttributes({owned,0,forbidden_allocation},[]{return true;}).status==FW_INVALID);
 unsigned attribute_checks=0;check(a.saveAttributes({owned,0,allocation},[&]{return ++attribute_checks<3;}).status==FW_TIMEOUT);
 check(attribute_checks==3&&!a.characterAttributes(owned,[]{return true;}).record);
 check(a.saveAttributes({owned,0,allocation},[]{return true;}).revision==1);
 check(a.saveAttributes({owned,0,allocation},[]{return true;}).status==FW_CONFLICT);
 check(a.saveAttributes({owned,1,forbidden_allocation},[]{return true;}).status==FW_INVALID);
 allocation.ranks[33]=2;attribute_checks=0;
 check(a.saveAttributes({owned,1,allocation},[&]{return ++attribute_checks<3;}).status==FW_TIMEOUT);
 const auto unchanged_attributes=a.characterAttributes(owned,[]{return true;});
 check(unchanged_attributes.record->revision==1&&unchanged_attributes.record->profile.ranks[33]==1);
 const auto initial_skills=a.characterSkills(owned,[]{return true;});
 check(initial_skills.status==FW_OK&&initial_skills.record&&initial_skills.record->revision==1);
 check(a.characterSkills({gwserver::Identity::from_hex(other),owned.character},[]{return true;}).status==FW_NOT_FOUND);
 check(a.characterSkills(owned,[]{return false;}).status==FW_TIMEOUT);
 const gwserver::SaveSkillbar skill_save{owned,1,initial_skills.record->profile.bar};
 check(a.saveSkillbar({skill_save.owner,0,skill_save.bar},[]{return true;}).status==FW_CONFLICT);
 check(a.saveSkillbar({{gwserver::Identity::from_hex(other),owned.character},1,skill_save.bar},[]{return true;}).status==FW_NOT_FOUND);
 auto invalid_bar=skill_save;invalid_bar.bar[0]={0xffffffffu,0};
 check(a.saveSkillbar(invalid_bar,[]{return true;}).status==FW_INVALID);
 unsigned skill_checks=0;
 check(a.saveSkillbar(skill_save,[&]{return ++skill_checks<3;}).status==FW_TIMEOUT);
 check(skill_checks==3&&a.characterSkills(owned,[]{return true;}).record->revision==1);
 check(a.saveSkillbar(skill_save,[]{return true;}).revision==2);
 const auto saved_skills=a.characterSkills(owned,[]{return true;});
 check(saved_skills.status==FW_OK&&saved_skills.record->revision==2&&gw::skills::encode_profile(saved_skills.record->profile)==gw::skills::encode_profile(initial_skills.record->profile));
 const gwserver::SaveSecondaryProfession profession_save{owned,2,1,saved_skills.record->profile,{}};
 check(a.saveSecondaryProfession({profession_save.owner,1,1,profession_save.skills,profession_save.attributes},[]{return true;}).status==FW_CONFLICT);
 unsigned profession_checks=0;
 check(a.saveSecondaryProfession(profession_save,[&]{return ++profession_checks<3;}).status==FW_TIMEOUT);
 check(profession_checks==3&&a.characterSkills(owned,[]{return true;}).record->revision==2&&a.characterAttributes(owned,[]{return true;}).record->revision==1);
 check(a.characterAttributes(owned,[]{return true;}).record->profile.ranks[33]==1);
 const auto profession_saved=a.saveSecondaryProfession(profession_save,[]{return true;});
 check(profession_saved.status==FW_OK&&profession_saved.skill_revision==3&&profession_saved.attribute_revision==2);
 check(a.characterAttributes(owned,[]{return true;}).record->profile.ranks[33]==0);
 check(a.characterQuests(owned,[]{return true;}).status==FW_OK&&!a.characterQuests(owned,[]{return true;}).record);
 check(call("gw-character-quests-get",key)=="0");
 check(a.execute(gwserver::Operation::characterQuests,key,[]{return true;}).payload=="0");
 check(a.execute(static_cast<gwserver::Operation>(9999),key,[]{return true;}).status==FW_NOT_FOUND);
 for(unsigned n=0;n<=static_cast<unsigned>(gwserver::Operation::maintenanceLogoutAll);++n){
  const auto operation=static_cast<gwserver::Operation>(n);const auto name=gwserver::storage_operation(operation);
  check(name&&gwserver::parse_storage_operation(*name)==operation);
 }
 check(!gwserver::parse_storage_operation("gw-character-quests-get-extra"));
 auto profile=*gw::quests::accept({},101);auto bytes=hex(gw::quests::encode(profile));
 call("gw-character-quests-save",other+"\t"+character+"\t0\t"+bytes,FW_NOT_FOUND);
 check(call("gw-character-quests-save",key+"\t0\t"+bytes)=="1");snapshot=call("gw-character-quests-get",key);check(snapshot=="1\t"+bytes);
 const auto typed=a.characterQuests(owned,[]{return true;});check(typed.record&&typed.record->revision==1&&typed.record->profile==profile);
 check(a.saveQuests({owned,0,profile},[]{return true;}).status==FW_CONFLICT);
 check(a.saveQuests({{gwserver::Identity::from_hex(other),owned.character},1,profile},[]{return true;}).status==FW_NOT_FOUND);
 auto invalid=profile;invalid.records.at(101).cycle=0;
 check(a.saveQuests({owned,1,invalid},[]{return true;}).status==FW_INVALID);
 check(a.characterQuests(owned,[]{return false;}).status==FW_TIMEOUT);
 call("gw-character-quests-save",key+"\t0\t"+bytes,FW_CONFLICT);
 call("gw-character-quests-save",key+"\t1\t00",FW_INVALID);
 unsigned checks=0;auto cancelled=a.execute(gwserver::Operation::saveQuests,key+"\t1\t"+bytes,[&]{return ++checks<3;});check(cancelled.status==FW_TIMEOUT);
 check(call("gw-character-quests-get",key)==snapshot);
 unsigned typed_checks=0;check(a.saveQuests({owned,1,profile},[&]{return ++typed_checks<3;}).status==FW_TIMEOUT);
 check(a.characterQuests(owned,[]{return true;}).record->revision==1);
 // Reward and XP must survive together; cancellation after writes rolls both back.
 check(a.rewardQuestExperience({owned,1,101,1,100},[]{return true;}).status==FW_CONFLICT);
 auto completed=*gw::quests::advance(profile,101,1,gw::quests::Stage::active,gw::quests::Stage::completed);
 check(a.saveQuests({owned,1,completed},[]{return true;}).revision==2);
 const auto xp_before=a.characterNumber(owned,false,[]{return true;});check(xp_before.value.has_value());
 unsigned reward_checks=0;
 check(a.rewardQuestExperience({owned,2,101,1,100},[&]{return ++reward_checks<3;}).status==FW_TIMEOUT);
 check(a.characterQuests(owned,[]{return true;}).record->profile==completed);
 check(a.characterNumber(owned,false,[]{return true;}).value==xp_before.value);
 check(a.rewardQuestExperience({owned,2,101,2,100},[]{return true;}).status==FW_CONFLICT);
 const auto skill_revision=a.characterSkills(owned,[]{return true;}).record->revision;
 auto reward_inventory=fw::gw::inventory::baseline();reward_inventory.gold=20;
 check(a.saveInventory({owned,0,reward_inventory},[]{return true;}).revision==1);
 const gwserver::RewardQuestExperience combined_reward{owned,2,101,1,100,skill_revision,{194},1,50};
 auto overflow_reward=combined_reward;overflow_reward.gold=100000;
 check(a.rewardQuestExperience(overflow_reward,[]{return true;}).status==FW_CONFLICT);
 check(a.rewardQuestExperience({owned,2,101,1,100,skill_revision-1,{194}},[]{return true;}).status==FW_CONFLICT);
 unsigned combined_checks=0;
 check(a.rewardQuestExperience(combined_reward,[&]{return ++combined_checks<3;}).status==FW_TIMEOUT);
 check(a.characterInventory(owned,[]{return true;}).record->profile.gold==20);
 check(!a.characterSkills(owned,[]{return true;}).record->profile.learned.contains(194));
 check(a.rewardQuestExperience(combined_reward,[]{return true;}).revision==3);
 check(a.characterInventory(owned,[]{return true;}).record->profile.gold==70);
 check(a.characterSkills(owned,[]{return true;}).record->profile.learned.at(194)==1);
 check(a.characterInventory(owned,[]{return true;}).record->profile.gold==70);
 check(a.characterNumber(owned,false,[]{return true;}).value==*xp_before.value+100);
 check(a.rewardQuestExperience({owned,3,101,1,100},[]{return true;}).status==FW_CONFLICT);
 check(a.characterNumber(owned,false,[]{return true;}).value==*xp_before.value+100);
 snapshot=call("gw-character-quests-get",key);
 const auto accepted_with_skill=*gw::quests::accept(a.characterQuests(owned,[]{return true;}).record->profile,73);
 const auto acceptance_revision=a.characterSkills(owned,[]{return true;}).record->revision;
 const gwserver::SaveQuests acceptance{owned,3,accepted_with_skill,acceptance_revision,{197,180,200}};
 unsigned acceptance_checks=0;
 check(a.saveQuests(acceptance,[&]{return ++acceptance_checks<3;}).status==FW_TIMEOUT);
 check(!a.characterQuests(owned,[]{return true;}).record->profile.records.contains(73));
 check(!a.characterSkills(owned,[]{return true;}).record->profile.learned.contains(197));
 check(a.saveQuests(acceptance,[]{return true;}).revision==4);
 check(a.characterSkills(owned,[]{return true;}).record->profile.learned.at(197)==1);
 check(a.saveQuests(acceptance,[]{return true;}).status==FW_CONFLICT);
 const auto inventory_before=a.characterInventory(owned,[]{return true;});
 const auto sword_plan=gw::inventory::presearing::grant_starter_sword(inventory_before.record->profile,u"Quest Character");
 check(sword_plan.has_value());
 const auto& sword=sword_plan->items.at(inventory_before.record->profile.next_item);
 check(sword.definition.model==2982&&sword.definition.file==9454&&sword.definition.type==27&&
       sword.definition.modifiers==std::vector<std::uint32_t>{616038912,2760377090}&&
       sword.customized==std::optional<std::u16string>{u"Quest Character"});
 check(gw::inventory::item_packets(sword).size()==3);
 for(unsigned primary:{1u,6u}){
  gw::equipment::proposal::Policy policy;policy.allowed=true;policy.primary=primary;policy.character="Quest Character";
  policy.reviewed=[&](const auto& definition){return gw::equipment::metadata::reviewed(*sword_plan,definition);};
  gw::inventory::Request equip;equip.change=gw::inventory::Change::equip;equip.item=sword.definition.id;
  const auto worn=gw::equipment::proposal::propose(*sword_plan,equip,policy);check(worn.accepted());
  gw::combat::weapon::Policy attack;attack.primary=primary;attack.character=policy.character;attack.mastery_attribute=20;
  const auto weapon=gw::combat::weapon::owned_weapon(worn.profile,1,attack);
  check(weapon&&weapon->damage.minimum==2&&weapon->damage.maximum==3&&weapon->damage.raw_damage_type==2&&weapon->customized);
  attack.character="Other Character";check(!gw::combat::weapon::owned_weapon(worn.profile,1,attack));
 }
 auto full=gw::inventory::baseline();for(unsigned n=0;n<20;++n)full=*gw::inventory::presearing::grant_starter_sword(full,u"Quest Character");
 check(!gw::inventory::presearing::grant_starter_sword(full,u"Quest Character"));
 const auto skills_before=a.characterSkills(owned,[]{return true;});
 gwserver::SaveQuests with_sword{owned,4,*gw::quests::accept(accepted_with_skill,75),skills_before.record->revision,{382,384,1},
     inventory_before.record->revision,*sword_plan};
 auto stale_inventory=with_sword;++stale_inventory.expected_inventory_revision;
 const auto stale_result=a.saveQuests(stale_inventory,[]{return true;});
 if(stale_result.status!=FW_CONFLICT)std::cerr<<"inventory acceptance: "<<stale_result.message<<'\n';
 check(stale_result.status==FW_CONFLICT);
 auto mutated_inventory=with_sword;++mutated_inventory.acceptance_inventory->gold;
 check(a.saveQuests(mutated_inventory,[]{return true;}).status==FW_INVALID);
 unsigned sword_checks=0;check(a.saveQuests(with_sword,[&]{return ++sword_checks<3;}).status==FW_TIMEOUT);
 check(a.characterQuests(owned,[]{return true;}).record->revision==4&&
       a.characterSkills(owned,[]{return true;}).record->revision==skills_before.record->revision&&
       gw::inventory::encode(a.characterInventory(owned,[]{return true;}).record->profile)==gw::inventory::encode(inventory_before.record->profile));
 check(a.saveQuests(with_sword,[]{return true;}).revision==5);
 check(a.characterInventory(owned,[]{return true;}).record->revision==inventory_before.record->revision+1&&
       gw::inventory::encode(a.characterInventory(owned,[]{return true;}).record->profile)==gw::inventory::encode(*sword_plan));
 check(a.characterSkills(owned,[]{return true;}).record->profile.learned.contains(382));
 check(a.saveQuests(with_sword,[]{return true;}).status==FW_CONFLICT);
 const auto grawl_complete=*gw::quests::advance(a.characterQuests(owned,[]{return true;}).record->profile,75,1,gw::quests::Stage::active,gw::quests::Stage::completed);
 check(a.saveQuests({owned,5,grawl_complete},[]{return true;}).revision==6);
 const auto grawl_xp=*a.characterNumber(owned,false,[]{return true;}).value;
 const gwserver::RewardQuestExperience grawl_reward{owned,6,75,1,250};
 check(a.rewardQuestExperience(grawl_reward,[]{return true;}).revision==7);
 check(a.rewardQuestExperience(grawl_reward,[]{return true;}).status==FW_CONFLICT);
 check(a.characterNumber(owned,false,[]{return true;}).value==grawl_xp+250);
 check(gw::inventory::encode(a.characterInventory(owned,[]{return true;}).record->profile)==gw::inventory::encode(*sword_plan));
 const auto air_skills=a.characterSkills(owned,[]{return true;}).record;
 const auto air_profile=*gw::quests::accept(a.characterQuests(owned,[]{return true;}).record->profile,37);
 check(a.saveQuests({owned,7,air_profile,air_skills->revision,{230,220}},[]{return true;}).revision==8);
 check(a.characterSkills(owned,[]{return true;}).record->profile.learned.contains(230)&&
       a.characterSkills(owned,[]{return true;}).record->profile.learned.contains(220));
 const auto air_complete=*gw::quests::advance(air_profile,37,1,gw::quests::Stage::active,gw::quests::Stage::completed);
 check(a.saveQuests({owned,8,air_complete},[]{return true;}).revision==9);
 const gwserver::RewardQuestExperience air_reward{owned,9,37,1,250};
 check(a.rewardQuestExperience(air_reward,[]{return true;}).revision==10);
 check(a.rewardQuestExperience(air_reward,[]{return true;}).status==FW_CONFLICT);
 // Explicit player abandonment after a failed encounter, then a fresh attempt.
 // This does not assert that retail automatically abandons on guardian death.
 const auto before_retry=a.characterQuests(owned,[]{return true;}).record;
 const auto abandoned_experiment=*gw::quests::abandon(before_retry->profile,73,1);
 check(a.saveQuests({owned,10,abandoned_experiment},[]{return true;}).revision==11);
 const auto fresh_experiment=*gw::quests::accept(abandoned_experiment,73);
 check(fresh_experiment.records.at(73).cycle==2);
 check(a.saveQuests({owned,11,fresh_experiment},[]{return true;}).revision==12);
 check(!gw::quests::advance(fresh_experiment,73,1,gw::quests::Stage::active,gw::quests::Stage::completed));
 const auto delayed_completion=*gw::quests::advance(before_retry->profile,73,1,gw::quests::Stage::active,gw::quests::Stage::completed);
 check(a.saveQuests({owned,10,delayed_completion},[]{return true;}).status==FW_CONFLICT);
 check(a.rewardQuestExperience({owned,12,73,1,250},[]{return true;}).status==FW_CONFLICT);
 snapshot=call("gw-character-quests-get",key);
 }
 {Accounts a(db.string());auto key=owner+"\t"+character;check(a.execute("gw-character-quests-get",key,[]{return true;}).payload==snapshot);
 const gwserver::OwnedCharacter owned{gwserver::Identity::from_hex(owner),gwserver::Identity::from_hex(character)};
 check(a.characterQuests(owned,[]{return true;}).record->profile.records.at(101).stage==gw::quests::Stage::rewarded);
 check(a.characterQuests(owned,[]{return true;}).record->profile.records.at(73).stage==gw::quests::Stage::active);
 check(a.characterQuests(owned,[]{return true;}).record->profile.records.at(73).cycle==2);
 check(a.characterQuests(owned,[]{return true;}).record->profile.records.at(75).stage==gw::quests::Stage::rewarded);
 check(a.characterInventory(owned,[]{return true;}).record->profile.items.begin()->second.definition.model==2982);
 check(a.characterSkills(owned,[]{return true;}).record->profile.learned.at(197)==1);
 check(a.characterNumber(owned,false,[]{return true;}).value==600);
 check(a.characterQuests(owned,[]{return true;}).record->profile.records.at(37).stage==gw::quests::Stage::rewarded);
 check(a.characterSkills(owned,[]{return true;}).record->profile.learned.at(194)==1);
 const auto completed_experiment=*gw::quests::advance(a.characterQuests(owned,[]{return true;}).record->profile,
     73,2,gw::quests::Stage::active,gw::quests::Stage::completed);
 check(a.saveQuests({owned,12,completed_experiment},[]{return true;}).revision==13);
 snapshot=a.execute("gw-character-quests-get",key,[]{return true;}).payload;
 }
 {Accounts a(db.string());const auto key=owner+"\t"+character;
 const gwserver::OwnedCharacter owned{gwserver::Identity::from_hex(owner),gwserver::Identity::from_hex(character)};
 check(a.execute("gw-character-quests-get",key,[]{return true;}).payload==snapshot);
 check(a.characterQuests(owned,[]{return true;}).record->profile.records.at(73).stage==gw::quests::Stage::completed);
 check(a.characterQuests(owned,[]{return true;}).record->profile.records.at(73).cycle==2);
 check(a.characterNumber(owned,false,[]{return true;}).value==600);
 const gwserver::RewardQuestExperience reward{owned,13,73,2,250};
 check(a.rewardQuestExperience(reward,[]{return true;}).revision==14);
 check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
 check(a.characterNumber(owned,false,[]{return true;}).value==850);
 snapshot=a.execute("gw-character-quests-get",key,[]{return true;}).payload;
 }
 {Accounts a(db.string());const auto key=owner+"\t"+character;
 const gwserver::OwnedCharacter owned{gwserver::Identity::from_hex(owner),gwserver::Identity::from_hex(character)};
 check(a.execute("gw-character-quests-get",key,[]{return true;}).payload==snapshot);
 check(a.characterQuests(owned,[]{return true;}).record->profile.records.at(73).stage==gw::quests::Stage::rewarded);
 check(a.characterNumber(owned,false,[]{return true;}).value==850);
 // Elementalist scale consumption shares the reward transaction with XP,
 // learned skills and completed->rewarded progress.
 auto quests=a.characterQuests(owned,[]{return true;});
 auto accepted=*gw::quests::accept(quests.record->profile,83,false);
 auto revision=a.saveQuests({owned,quests.record->revision,accepted},[]{return true;}).revision;
 auto completed=*gw::quests::advance(accepted,83,1,gw::quests::Stage::active,gw::quests::Stage::completed);
 revision=a.saveQuests({owned,revision,completed},[]{return true;}).revision;
 auto inventory=a.characterInventory(owned,[]{return true;});
 auto skills=a.characterSkills(owned,[]{return true;});
 gwserver::RewardQuestExperience turnin{owned,revision,83,1,500,skills.record->revision,{194,180},inventory.record->revision,0,true};
 check(a.rewardQuestExperience(turnin,[]{return true;}).status==FW_CONFLICT);
 check(a.characterNumber(owned,false,[]{return true;}).value==850);
 auto with_scale=inventory.record->profile;gw::inventory::Item scale;scale.bag=1;scale.slot=19;
 auto& definition=scale.definition;definition.id=with_scale.next_item++;definition.model=2566;definition.file=23704;
 definition.type=21;definition.quantity=1;definition.interaction=537399809;definition.encoded_name={9558,52505,35705,32767};
 with_scale.items.emplace(definition.id,scale);
 const auto inventory_revision=a.saveInventory({owned,inventory.record->revision,with_scale},[]{return true;}).revision;
 check(inventory_revision==inventory.record->revision+1);
 check(a.rewardQuestExperience(turnin,[]{return true;}).status==FW_CONFLICT); // stale inventory fence
 turnin.expected_inventory_revision=inventory_revision;
 unsigned turnin_checks=0;check(a.rewardQuestExperience(turnin,[&]{return ++turnin_checks<3;}).status==FW_TIMEOUT);
 check(a.characterInventory(owned,[]{return true;}).record->profile.items.contains(definition.id));
 check(a.characterSkills(owned,[]{return true;}).record->revision==skills.record->revision);
 check(a.characterQuests(owned,[]{return true;}).record->profile.records.at(83).stage==gw::quests::Stage::completed);
 check(a.characterNumber(owned,false,[]{return true;}).value==850);
 check(a.rewardQuestExperience(turnin,[]{return true;}).revision==revision+1);
 check(a.characterNumber(owned,false,[]{return true;}).value==1350);
 check(a.characterSkills(owned,[]{return true;}).record->revision==skills.record->revision+1);
 check(!a.characterInventory(owned,[]{return true;}).record->profile.items.contains(definition.id));
 check(a.characterInventory(owned,[]{return true;}).record->revision==inventory_revision+1);
 check(a.rewardQuestExperience(turnin,[]{return true;}).status==FW_CONFLICT);
 {Accounts reopened(db.string());check(reopened.characterQuests(owned,[]{return true;}).record->profile.records.at(83).stage==gw::quests::Stage::rewarded);
 check(reopened.characterNumber(owned,false,[]{return true;}).value==1350);
 check(!reopened.characterInventory(owned,[]{return true;}).record->profile.items.contains(definition.id));}
 {
  const auto current=a.characterQuests(owned,[]{return true;});
  auto counted=*gw::quests::accept(current.record->profile,43);
  auto revision=a.saveQuests({owned,current.record->revision,counted},[]{return true;}).revision;
  counted=*gw::quests::credit_objective(counted,43,1,0x10000001eull,3);
  check(a.saveQuests({owned,revision,counted},[]{return true;}).revision==revision+1);
  check(a.saveQuests({owned,revision,counted},[]{return true;}).status==FW_CONFLICT);++revision;
  {Accounts reopened(db.string());const auto restored=reopened.characterQuests(owned,[]{return true;});
   check(restored.record->profile==counted&&restored.record->revision==revision);
   check(!gw::quests::credit_objective(restored.record->profile,43,1,0x10000001eull,3));}
  for(auto credit:{0x100000027ull,0x100000031ull}){
   counted=*gw::quests::credit_objective(counted,43,1,credit,3);
   check(a.saveQuests({owned,revision,counted},[]{return true;}).revision==revision+1);++revision;
  }
  const gwserver::RewardQuestExperience reward{owned,revision,43,1,250};
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_OK);
  check(a.rewardQuestExperience(reward,[]{return true;}).status==FW_CONFLICT);
  {Accounts reopened(db.string());const auto restored=reopened.characterQuests(owned,[]{return true;});
   check(restored.record->profile.records.at(43).stage==gw::quests::Stage::rewarded);
   check(restored.record->profile.records.at(43).objective_credits.size()==3);
   check(reopened.characterNumber(owned,false,[]{return true;}).value==1600);}
 }
 check(a.execute("gw-character-delete",key,[]{return true;}).status==FW_OK);
 check(a.execute("gw-character-quests-get",key,[]{return true;}).status==FW_NOT_FOUND);}
 std::cout<<"PASS quest ownership, revision, rollback, restart and deletion\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

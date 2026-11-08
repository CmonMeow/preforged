gwserver::QuestsResult read_quests(sqlite3* db,const gwserver::OwnedCharacter& owned,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);
 const auto account=gwserver::storage_hex(owned.account.bytes),character=gwserver::storage_hex(owned.character.bytes);
 Query owner(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");owner.text(1,account);owner.text(2,character);
 if(!owner.row())return {FW_NOT_FOUND,"owned quest character missing",{}};
 Query current(db,"SELECT revision,profile FROM fw_gw_quests WHERE character=?");current.text(1,character);
 if(!current.row()){allowed(permitted);return {FW_OK,{},{}};}
 const auto revision=current.number(0);if(revision<=0)throw SqlError("invalid quest revision");
 auto profile=gw::quests::decode(current.blob(1));allowed(permitted);
 return {FW_OK,{},gwserver::VersionedProfile<gw::quests::Profile>{static_cast<std::uint64_t>(revision),std::move(profile)}};
}
gwserver::RevisionResult reward_quest_experience(sqlite3* db,const gwserver::RewardQuestExperience& request,const std::function<bool()>& permitted){
 if(request.id==76&&(request.experience!=gw::quests::ranger_companion::experience||request.gold||request.consume_shimmering_scale||
    request.reward_inventory||!request.learned_skills.empty()||request.fill_skillbar))
  return {FW_INVALID,"Ranger Companion requires exactly250XP and its reviewed trial unlock"};
 if(request.id==89&&(request.experience!=gw::quests::path_to_glory::experience||request.gold||request.consume_shimmering_scale||
    request.reward_inventory||!request.learned_skills.empty()||request.fill_skillbar||request.unlock_trial))
  return {FW_INVALID,"Path to Glory requires exactly100XP and no other reward changes"};
 if(request.id==80&&(request.experience!=gw::quests::message_from_friend::experience||request.gold!=gw::quests::message_from_friend::gold||
    request.consume_shimmering_scale||request.reward_inventory||!request.learned_skills.empty()||request.fill_skillbar||request.unlock_trial))
  return {FW_INVALID,"Message from a Friend requires exactly100XP and10gold"};
 if(request.id==79&&(request.experience!=gw::quests::second_profession::experience||request.gold||request.consume_shimmering_scale||
    request.reward_inventory||!request.learned_skills.empty()||request.fill_skillbar||request.unlock_trial))
  return {FW_INVALID,"Second Profession requires exactly250XP and no other reward changes"};
 if(request.fill_skillbar&&request.learned_skills.empty())return {FW_INVALID,"reward bar assignment requires skills"};
 if(request.consume_shimmering_scale&&request.id!=83)return {FW_INVALID,"scale turn-in requires Elementalist Test"};
 if(request.reward_inventory&&(request.gold||request.consume_shimmering_scale))return {FW_INVALID,"item reward cannot combine inventory mutations"};
 if(request.id==78&&(request.experience!=500||request.gold||request.consume_shimmering_scale||request.reward_inventory||
    !request.learned_skills.empty()||request.fill_skillbar||request.unlock_trial))
  return {FW_INVALID,"Opposition requires exactly500XP and no other reward changes"};
 if(request.id==62&&(!request.reward_inventory||request.experience!=250||request.gold||request.consume_shimmering_scale||
    !request.learned_skills.empty()||request.fill_skillbar||request.unlock_trial))
  return {FW_INVALID,"Unsettling Rumors requires its exact item and XP reward"};
 if(request.id==55&&(!request.reward_inventory||request.experience!=250||!request.learned_skills.empty()||request.unlock_trial))
  return {FW_INVALID,"Wayward Wizard requires its exact item and XP reward"};
 if(request.id==46&&(!request.reward_inventory||request.experience!=gw::quests::charr_gate::experience||
    request.gold||request.consume_shimmering_scale||!request.learned_skills.empty()||request.fill_skillbar||request.unlock_trial))
  return {FW_INVALID,"Charr at the Gate requires its exact item and XP reward"};
 allowed(permitted);gwserver::check_revision(request.expected_revision);
 const auto account=gwserver::storage_hex(request.owner.account.bytes),character=gwserver::storage_hex(request.owner.character.bytes);
 Transaction tx(db);Query owner(db,"SELECT experience,summary FROM fw_gw_characters WHERE account=? AND id=?");owner.text(1,account);owner.text(2,character);
 if(!owner.row())return {FW_NOT_FOUND,"owned quest character missing"};
 if(sqlite3_column_type(owner.statement,0)==SQLITE_NULL)return {FW_UNAVAILABLE,"character experience baseline unavailable"};
 const auto experience=owner.number(0);
 if(experience<0||experience>UINT32_MAX)throw SqlError("invalid character experience");
 if(request.experience>UINT32_MAX-experience)return {FW_INVALID,"quest experience overflow"};
 Query current(db,"SELECT revision,profile FROM fw_gw_quests WHERE character=?");current.text(1,character);
 if(!current.row())return {FW_NOT_FOUND,"quest not active"};
 const auto revision=current.number(0);
 if(revision<=0||static_cast<std::uint64_t>(revision)!=request.expected_revision)return {FW_CONFLICT,"quest revision changed"};
 const auto current_quests=gw::quests::decode(current.blob(1));
 if(request.id==76){
  Query chosen(db,"SELECT profile FROM fw_gw_character_skills WHERE character=?");chosen.text(1,character);
  if(!chosen.row())return {FW_NOT_FOUND,"Ranger Companion skill baseline missing"};
  const auto skills=gw::skills::decode_profile(chosen.blob(0));
  Query pet(db,"SELECT profile FROM fw_gw_pets WHERE character=?");pet.text(1,character);
  const auto pets=pet.row()?gw::pets::decode(pet.blob(0)):gw::pets::Profile{};
  const auto progress=current_quests.records.find(76);
  if(progress==current_quests.records.end()||!gw::quests::ranger_companion::valid_progress(progress->second)||
     !gw::quests::ranger_companion::objective_ready(current_quests,{true,true,skills.primary,skills.secondary},pets)||
     request.unlock_trial!=(skills.primary!=2&&!skills.secondary))
   return {FW_INVALID,"Ranger Companion requires persisted Stalker, eligible profession and exact unlock"};
  for(auto skill:gw::quests::ranger_companion::grants)if(!skills.learned.contains(skill))
   return {FW_INVALID,"Ranger Companion acceptance grants missing"};
 }
 if(request.id==89){
  Query chosen(db,"SELECT profile FROM fw_gw_character_skills WHERE character=?");chosen.text(1,character);
  if(!chosen.row())return {FW_NOT_FOUND,"Path to Glory profession baseline missing"};
  const auto skills=gw::skills::decode_profile(chosen.blob(0));
  if(!gw::quests::path_to_glory::eligible(current_quests,{true,true,skills.primary}))
   return {FW_INVALID,"Path to Glory requires the rewarded primary test"};
 }
 if(request.id==79){
  Query chosen(db,"SELECT profile FROM fw_gw_character_skills WHERE character=?");chosen.text(1,character);
  if(!chosen.row())return {FW_NOT_FOUND,"Second Profession skill baseline missing"};
  const auto skills=gw::skills::decode_profile(chosen.blob(0));
  const auto progress=current_quests.records.find(79);
  if(progress==current_quests.records.end()||!gw::quests::second_profession::valid_progress(progress->second)||
     !gw::quests::second_profession::objective_ready(current_quests,{true,true,skills.primary,skills.secondary}))
   return {FW_INVALID,"Second Profession requires a committed trained secondary"};
 }
 if(request.id==78&&(!current_quests.records.contains(78)||!gw::quests::opposition::valid_progress(current_quests.records.at(78))))
  return {FW_INVALID,"Opposition requires three distinct eligible witnesses"};
 if(request.id==62){
  Query chosen(db,"SELECT profile FROM fw_gw_character_skills WHERE character=?");chosen.text(1,character);
  if(!chosen.row())return {FW_NOT_FOUND,"Unsettling Rumors profession baseline missing"};
  const auto skills=gw::skills::decode_profile(chosen.blob(0));
  if(!gw::quests::path_to_glory::eligible(current_quests,{true,true,skills.primary})||
     !current_quests.records.contains(62)||!gw::quests::unsettling_rumors::valid_progress(current_quests.records.at(62)))
   return {FW_INVALID,"Unsettling Rumors ordered objectives or primary test incomplete"};
 }
 if(request.id==55&&(!current_quests.records.contains(55)||!gw::quests::wayward_wizard::valid_progress(current_quests.records.at(55))))
  return {FW_INVALID,"Wayward Wizard ordered objectives incomplete"};
 if(request.id==46&&(!current_quests.records.contains(46)||!gw::quests::charr_gate::valid_progress(current_quests.records.at(46))))
  return {FW_INVALID,"Charr at the Gate trusted objectives incomplete"};
 const auto proposed=request.id==80?gw::quests::message_from_friend::reward_progress(current_quests,request.cycle):
     request.id==89?gw::quests::path_to_glory::reward_progress(current_quests,request.cycle):
     gw::quests::advance(current_quests,request.id,request.cycle,gw::quests::Stage::completed,gw::quests::Stage::rewarded);
 if(!proposed)return {FW_CONFLICT,"quest attempt cannot be rewarded"};
 std::vector<std::uint8_t> skill_bytes;
 if(!request.learned_skills.empty()||request.unlock_trial){
  gwserver::check_revision(request.expected_skill_revision);
  Query skills(db,"SELECT revision,profile FROM fw_gw_character_skills WHERE character=?");skills.text(1,character);
  if(!skills.row())return {FW_NOT_FOUND,"quest skill baseline missing"};
  if(skills.number(0)<=0||static_cast<std::uint64_t>(skills.number(0))!=request.expected_skill_revision)return {FW_CONFLICT,"quest skill revision changed"};
  auto profile=gw::skills::decode_profile(skills.blob(1));
  try{profile=gw::quests::teach(profile,request.learned_skills,request.fill_skillbar);
      if(request.unlock_trial)profile=gw::quests::unlock_trial(profile,request.id);}
  catch(const std::invalid_argument& e){return {FW_INVALID,e.what()};}
  skill_bytes=gw::skills::encode_profile(profile);
 }
 const auto packed=gw::quests::encode(*proposed);allowed(permitted);
 std::vector<std::uint8_t> inventory_bytes;
 if(request.gold||request.consume_shimmering_scale||request.reward_inventory){
  gwserver::check_revision(request.expected_inventory_revision);
  Query inventory(db,"SELECT revision,profile FROM fw_gw_inventory WHERE character=?");inventory.text(1,character);
  if(!inventory.row())return {FW_NOT_FOUND,"quest inventory baseline missing"};
  if(inventory.number(0)<=0||static_cast<std::uint64_t>(inventory.number(0))!=request.expected_inventory_revision)return {FW_CONFLICT,"quest inventory revision changed"};
  auto profile=gw::inventory::decode(inventory.blob(1));gw::inventory::validate_character(profile);
  if(request.reward_inventory){
   auto retained=*request.reward_inventory;gw::inventory::validate_character(retained);
   unsigned introduced=0;
   for(auto it=retained.items.begin();it!=retained.items.end();){
    if(!profile.items.contains(it->first)){
     if(it->first!=profile.next_item||it->second.definition.quantity!=1||gw::inventory::installed(it->second)||
        !retained.bags.contains(it->second.bag)||retained.bags.at(it->second.bag).type!=1)
      return {FW_INVALID,"invalid reward item identity or quantity"};
     if(request.id==62&&(gw::items::definition_2022(it->second.definition)!=gw::items::definition_2022(gw::quests::unsettling_rumors::tall_shield(it->first))||
        it->second.profession||it->second.customized||it->second.reviewed_armor))
      return {FW_INVALID,"Unsettling Rumors reward item differs from capture"};
     if(request.id==55&&(gw::items::definition_2022(it->second.definition)!=gw::items::definition_2022(gw::quests::wayward_wizard::air_staff(it->first))||
        it->second.profession||it->second.customized||it->second.reviewed_armor))
      return {FW_INVALID,"Wayward Wizard reward item differs from capture"};
     if(request.id==46&&(gw::items::definition_2022(it->second.definition)!=gw::items::definition_2022(gw::quests::charr_gate::long_sword(it->first))||
        it->second.profession||it->second.customized||it->second.reviewed_armor))
      return {FW_INVALID,"Charr at the Gate reward item differs from capture"};
     ++introduced;it=retained.items.erase(it);
    }else ++it;
   }
   if(retained.next_item!=profile.next_item+1)return {FW_INVALID,"invalid reward item allocator"};
   retained.next_item=profile.next_item;
   if(introduced!=1||gw::inventory::encode(retained)!=gw::inventory::encode(profile))
    return {FW_INVALID,"reward must add one item and preserve inventory"};
   profile=*request.reward_inventory;
  }
  if(request.consume_shimmering_scale){
   const auto consumed=gw::inventory::presearing::consume_shimmering_scale(profile);
   if(!consumed)return {FW_CONFLICT,"Shimmering Scale not carried"};
   profile=*consumed;
  }
  if(request.gold>100000u-profile.gold)return {FW_CONFLICT,"quest gold exceeds carrying capacity"};
  profile.gold+=request.gold;inventory_bytes=gw::inventory::encode(profile);
 }
 const auto summary=owner.blob(1);std::vector<std::uint8_t> next_summary(summary.begin(),summary.end());
 if(!next_summary.empty())next_summary=gw::experience::advance(gw::CharacterProfile::decode(next_summary),
     static_cast<std::uint32_t>(experience+request.experience)).encode();
 Query reward(db,"UPDATE fw_gw_characters SET experience=?,summary=? WHERE account=? AND id=?");
 reward.number(1,experience+request.experience);reward.blob(2,next_summary);reward.text(3,account);reward.text(4,character);reward.row();
 Query write(db,"UPDATE fw_gw_quests SET revision=?,profile=? WHERE character=?");
 write.number(1,revision+1);write.blob(2,packed);write.text(3,character);write.row();
 if(!skill_bytes.empty()){
  Query skills(db,"UPDATE fw_gw_character_skills SET revision=?,profile=? WHERE character=?");
  skills.number(1,request.expected_skill_revision+1);skills.blob(2,skill_bytes);skills.text(3,character);skills.row();
 }
 if(!inventory_bytes.empty()){
  Query inventory(db,"UPDATE fw_gw_inventory SET revision=?,profile=? WHERE character=?");
  inventory.number(1,request.expected_inventory_revision+1);inventory.blob(2,inventory_bytes);inventory.text(3,character);inventory.row();
 }
 allowed(permitted);tx.commit();return {FW_OK,{},static_cast<std::uint64_t>(revision+1)};
}

gwserver::RevisionResult write_quests(sqlite3* db,const gwserver::SaveQuests& request,const std::function<bool()>& permitted){
 allowed(permitted);gwserver::check_revision(request.expected_revision);
 if(request.acceptance_fill_skillbar&&request.acceptance_skills.empty())return {FW_INVALID,"quest bar assignment requires acceptance skills"};
 if(request.acceptance_trial&&request.acceptance_skills.empty())return {FW_INVALID,"trial requires quest skill grants"};
 if(request.abandon_trial_quest&&(!request.acceptance_skills.empty()||request.acceptance_inventory||request.acceptance_trial||request.acceptance_fill_skillbar))
  return {FW_INVALID,"trial abandonment cannot grant skills/items"};
 const auto bytes=gw::quests::encode(request.profile);
 const auto account=gwserver::storage_hex(request.owner.account.bytes),character=gwserver::storage_hex(request.owner.character.bytes);
 Transaction tx(db);Query owner(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");owner.text(1,account);owner.text(2,character);
 if(!owner.row())return {FW_NOT_FOUND,"owned quest character missing"};
 Query current(db,"SELECT revision,profile FROM fw_gw_quests WHERE character=?");current.text(1,character);
 const auto revision=current.row()?current.number(0):0;
 if(revision<0||static_cast<std::uint64_t>(revision)!=request.expected_revision)return {FW_CONFLICT,"quest revision changed"};
 {
  const auto previous=revision?gw::quests::decode(current.blob(1)):gw::quests::Profile{};
  {
   const auto old=previous.records.find(76),next=request.profile.records.find(76);
   const bool changed=(old==previous.records.end())!=(next==request.profile.records.end())||
       (old!=previous.records.end()&&next!=request.profile.records.end()&&old->second!=next->second);
   if(changed){
    Query chosen(db,"SELECT profile FROM fw_gw_character_skills WHERE character=?");chosen.text(1,character);
    if(!chosen.row())return {FW_NOT_FOUND,"Ranger Companion skill baseline missing"};
    const auto skills=gw::skills::decode_profile(chosen.blob(0));
    Query pet(db,"SELECT profile FROM fw_gw_pets WHERE character=?");pet.text(1,character);
    const auto pets=pet.row()?gw::pets::decode(pet.blob(0)):gw::pets::Profile{};
    if(!gw::quests::ranger_companion::valid_transition(previous,request.profile,{true,true,skills.primary,skills.secondary},pets))
     return {FW_INVALID,"Ranger Companion requires pet-free acceptance or persisted Stalker completion"};
    if(next!=request.profile.records.end()&&next->second.stage==gw::quests::Stage::active){
     const std::vector<std::uint32_t> grants(gw::quests::ranger_companion::grants.begin(),gw::quests::ranger_companion::grants.end());
     if(request.acceptance_skills!=grants||!request.acceptance_fill_skillbar||request.acceptance_trial!=(skills.primary!=2&&!skills.secondary))
      return {FW_INVALID,"Ranger Companion requires exact acceptance skills and trial policy"};
     gwserver::check_revision(request.expected_inventory_revision);
     Query inventory(db,"SELECT revision,profile FROM fw_gw_inventory WHERE character=?");inventory.text(1,character);
     if(!inventory.row())return {FW_NOT_FOUND,"Ranger Companion inventory baseline missing"};
     if(inventory.number(0)<=0||static_cast<std::uint64_t>(inventory.number(0))!=request.expected_inventory_revision)
      return {FW_CONFLICT,"Ranger Companion inventory revision changed"};
     const auto before=gw::inventory::decode(inventory.blob(1));
     const auto bow=gw::quests::ranger_companion::grant_bow(before);
     if(!bow||bool(request.acceptance_inventory)!=bool(bow->item)||
        (bow->item&&gw::inventory::encode(*request.acceptance_inventory)!=gw::inventory::encode(bow->profile)))
      return {FW_INVALID,"Ranger Companion requires its exact conditional captured bow"};
    }else if(!request.acceptance_skills.empty()||request.acceptance_inventory||request.acceptance_fill_skillbar||request.acceptance_trial)
     return {FW_INVALID,"Ranger Companion completion/abandonment cannot grant acceptance benefits"};
   }
  }
  {
   const auto old=previous.records.find(62),next=request.profile.records.find(62);
   const bool changed=(old==previous.records.end())!=(next==request.profile.records.end())||
       (old!=previous.records.end()&&next!=request.profile.records.end()&&old->second!=next->second);
   if(changed){
    Query chosen(db,"SELECT profile FROM fw_gw_character_skills WHERE character=?");chosen.text(1,character);
    if(!chosen.row())return {FW_NOT_FOUND,"Unsettling Rumors profession baseline missing"};
    const auto skills=gw::skills::decode_profile(chosen.blob(0));
    if(!gw::quests::unsettling_rumors::valid_transition(previous,request.profile,gw::quests::path_to_glory::eligible(previous,{true,true,skills.primary})))
     return {FW_INVALID,"Unsettling Rumors requires ordered dialogue and atomic item reward"};
    if(!request.acceptance_skills.empty()||request.acceptance_inventory||request.acceptance_fill_skillbar||request.acceptance_trial)
     return {FW_INVALID,"Unsettling Rumors has no acceptance grants"};
   }
  }
  if(!gw::quests::opposition::valid_transition(previous,request.profile))
   return {FW_INVALID,"Opposition requires distinct witness transitions and atomic reward"};
  {
   const auto old=previous.records.find(78),next=request.profile.records.find(78);
   const bool changed=(old==previous.records.end())!=(next==request.profile.records.end())||
       (old!=previous.records.end()&&next!=request.profile.records.end()&&old->second!=next->second);
   if(changed&&(!request.acceptance_skills.empty()||request.acceptance_inventory||request.acceptance_fill_skillbar||request.acceptance_trial))
    return {FW_INVALID,"Opposition has no acceptance grants"};
  }
  if(!gw::quests::message_from_friend::valid_transition(previous,request.profile))
   return {FW_INVALID,"Message from a Friend requires legal acceptance/abandonment or atomic turn-in"};
  {
   const auto old_glory=previous.records.find(89),next_glory=request.profile.records.find(89);
   const bool changed_glory=(old_glory==previous.records.end())!=(next_glory==request.profile.records.end())||
       (old_glory!=previous.records.end()&&next_glory!=request.profile.records.end()&&old_glory->second!=next_glory->second);
   if(changed_glory){
    Query chosen(db,"SELECT profile FROM fw_gw_character_skills WHERE character=?");chosen.text(1,character);
    if(!chosen.row())return {FW_NOT_FOUND,"Path to Glory profession baseline missing"};
    const auto skills=gw::skills::decode_profile(chosen.blob(0));
    if(!gw::quests::path_to_glory::valid_transition(previous,request.profile,{true,true,skills.primary}))
     return {FW_INVALID,"Path to Glory requires legal primary-test-gated acceptance or atomic turn-in"};
   }
  }
  const auto before=previous.records.find(79),after=request.profile.records.find(79);
  const bool changed=(before==previous.records.end())!=(after==request.profile.records.end())||
      (before!=previous.records.end()&&after!=request.profile.records.end()&&before->second!=after->second);
  if(changed){
   Query chosen(db,"SELECT profile FROM fw_gw_character_skills WHERE character=?");chosen.text(1,character);
   if(!chosen.row())return {FW_NOT_FOUND,"Second Profession skill baseline missing"};
   const auto skills=gw::skills::decode_profile(chosen.blob(0));
   if(!gw::quests::second_profession::valid_transition(previous,request.profile,{true,true,skills.primary,skills.secondary}))
    return {FW_INVALID,"Second Profession transition requires persisted primary/training/secondary authority"};
  }
 }
 std::vector<std::uint8_t> skill_bytes,inventory_bytes;
 std::uint32_t accepted_quest=0;
 if(request.abandon_trial_quest){
  const auto previous=revision?gw::quests::decode(current.blob(1)):gw::quests::Profile{};
  const auto record=previous.records.find(request.abandon_trial_quest);
  const auto planned=record==previous.records.end()?std::optional<gw::quests::Profile>{}:
      gw::quests::abandon(previous,request.abandon_trial_quest,record->second.cycle);
  if(!planned||*planned!=request.profile)return {FW_INVALID,"trial cleanup requires exact quest abandonment"};
  gwserver::check_revision(request.expected_skill_revision);
  Query skills(db,"SELECT revision,profile FROM fw_gw_character_skills WHERE character=?");skills.text(1,character);
  if(!skills.row())return {FW_NOT_FOUND,"trial skill baseline missing"};
  if(skills.number(0)<=0||static_cast<std::uint64_t>(skills.number(0))!=request.expected_skill_revision)
   return {FW_CONFLICT,"trial skill revision changed"};
  skill_bytes=gw::skills::encode_profile(gw::quests::prune_trials(gw::skills::decode_profile(skills.blob(1)),request.profile));
 }
 if(!request.acceptance_skills.empty()||request.acceptance_inventory){
  const auto previous=revision?gw::quests::decode(current.blob(1)):gw::quests::Profile{};
  unsigned changed=0;bool acceptance=false;
  for(const auto& [id,progress]:request.profile.records){
   const auto old=previous.records.find(id);
   if(old==previous.records.end()||old->second!=progress){
    ++changed;accepted_quest=id;auto planned=gw::quests::accept(previous,id,true);
    if(id==42&&planned&&progress.timed)
        planned=gw::quests::arm_marksmanship(*planned,planned->records.at(id).cycle,progress.timed->started_utc_ms);
    acceptance=planned&&*planned==request.profile;
   }
  }
  if(changed!=1||!acceptance)return {FW_INVALID,"grants require one quest acceptance"};
 }
 if(!request.acceptance_skills.empty()){
  gwserver::check_revision(request.expected_skill_revision);
  Query skills(db,"SELECT revision,profile FROM fw_gw_character_skills WHERE character=?");skills.text(1,character);
  if(!skills.row())return {FW_NOT_FOUND,"acceptance skill baseline missing"};
  if(skills.number(0)<=0||static_cast<std::uint64_t>(skills.number(0))!=request.expected_skill_revision)return {FW_CONFLICT,"acceptance skill revision changed"};
  auto profile=gw::skills::decode_profile(skills.blob(1));
  try{profile=gw::quests::teach_acceptance(profile,accepted_quest,request.acceptance_skills,request.acceptance_fill_skillbar,request.acceptance_trial);}
  catch(const std::invalid_argument& e){return {FW_INVALID,e.what()};}
  skill_bytes=gw::skills::encode_profile(profile);
 }
 if(request.acceptance_inventory){
  gwserver::check_revision(request.expected_inventory_revision);
  Query inventory(db,"SELECT revision,profile FROM fw_gw_inventory WHERE character=?");inventory.text(1,character);
  if(!inventory.row())return {FW_NOT_FOUND,"acceptance inventory baseline missing"};
  if(inventory.number(0)<=0||static_cast<std::uint64_t>(inventory.number(0))!=request.expected_inventory_revision)
   return {FW_CONFLICT,"acceptance inventory revision changed"};
  const auto previous=gw::inventory::decode(inventory.blob(1));
  auto retained=*request.acceptance_inventory;gw::inventory::validate_character(retained);
  unsigned introduced=0;
  for(auto it=retained.items.begin();it!=retained.items.end();){
   if(!previous.items.contains(it->first)){
    if(it->first<previous.next_item||it->second.definition.quantity!=1||gw::inventory::installed(it->second))
     return {FW_INVALID,"invalid acceptance item identity or quantity"};
    ++introduced;it=retained.items.erase(it);
   }else ++it;
  }
  retained.next_item=previous.next_item;
  if(introduced!=1||gw::inventory::encode(retained)!=gw::inventory::encode(previous))
   return {FW_INVALID,"acceptance grant must add one item and preserve inventory"};
  inventory_bytes=gw::inventory::encode(*request.acceptance_inventory);
 }
 allowed(permitted);Query write(db,"INSERT INTO fw_gw_quests VALUES(?,?,?) ON CONFLICT(character) DO UPDATE SET revision=excluded.revision,profile=excluded.profile");
 write.text(1,character);write.number(2,revision+1);write.blob(3,bytes);write.row();
 if(!skill_bytes.empty()){
  Query skills(db,"UPDATE fw_gw_character_skills SET revision=?,profile=? WHERE character=?");
  skills.number(1,request.expected_skill_revision+1);skills.blob(2,skill_bytes);skills.text(3,character);skills.row();
 }
 if(!inventory_bytes.empty()){
  Query inventory(db,"UPDATE fw_gw_inventory SET revision=?,profile=? WHERE character=?");
  inventory.number(1,request.expected_inventory_revision+1);inventory.blob(2,inventory_bytes);inventory.text(3,character);inventory.row();
 }
 allowed(permitted);tx.commit();
 return {FW_OK,{},static_cast<std::uint64_t>(revision+1)};
}

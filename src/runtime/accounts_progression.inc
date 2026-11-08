std::optional<AccountResult> progression_execute(sqlite3* db,gwserver::Operation op,const std::vector<std::string>& f,const std::function<bool()>& permitted){
 if(op==gwserver::Operation::characterQuests||op==gwserver::Operation::saveQuests){
  const bool get=op==gwserver::Operation::characterQuests;
  if(f.size()!=(get?2:4)||!social_id(f[0])||!social_id(f[1]))return AccountResult{FW_INVALID,"quest profile fields"};
  const gwserver::OwnedCharacter owned{gwserver::Identity::from_hex(f[0]),gwserver::Identity::from_hex(f[1])};
  if(get){const auto result=read_quests(db,owned,permitted);if(result.status!=FW_OK)return AccountResult{result.status,result.message};
   return AccountResult{FW_OK,result.record?std::to_string(result.record->revision)+"\t"+hex_bytes(gw::quests::encode(result.record->profile)):"0"};}
  const auto& h=f[3];if(h.empty()||h.size()%2||h.size()>98320)return AccountResult{FW_INVALID,"quest profile bound"};
  std::vector<std::uint8_t> bytes(h.size()/2);
  if(sodium_hex2bin(bytes.data(),bytes.size(),h.data(),h.size(),nullptr,nullptr,nullptr))return AccountResult{FW_INVALID,"quest profile encoding"};
  const auto result=write_quests(db,{owned,static_cast<std::uint64_t>(number(f[2],INT64_MAX-1)),gw::quests::decode(bytes)},permitted);
  return AccountResult{result.status,result.status==FW_OK?std::to_string(result.revision):result.message};
 }
 if(op==gwserver::Operation::characterProgression||op==gwserver::Operation::saveProgression){
  const bool get=op==gwserver::Operation::characterProgression;
  if(f.size()!=(get?2:4)||!social_id(f[0])||!social_id(f[1]))return AccountResult{FW_INVALID,"progression fields"};
  const gwserver::OwnedCharacter owner{gwserver::Identity::from_hex(f[0]),gwserver::Identity::from_hex(f[1])};
  if(get){const auto result=read_character_progression(db,owner,permitted);if(result.status!=FW_OK)return AccountResult{result.status,result.message};
   return AccountResult{FW_OK,result.record?std::to_string(result.record->revision)+"\t"+hex_bytes(gw::progression::encode(result.record->profile)):"0"};}
  const auto result=write_character_progression(db,{owner,number(f[2],INT64_MAX-1),gw::progression::decode_character(gwserver::profile_bytes(f[3],65536))},permitted);
  return AccountResult{result.status,result.status==FW_OK?std::to_string(result.revision):result.message};
 }

 if(op==gwserver::Operation::maintenanceGwAccountTitlesSeed){
  if(f.size()!=3||!social_id(f[0])||!social_id(f[1]))return AccountResult{FW_INVALID,"title seed fields"};
  const auto expected=number(f[2],INT64_MAX-1);Transaction tx(db);
  Query source(db,"SELECT p.revision,p.profile,i.source FROM fw_gw_characters c JOIN fw_gw_progression p ON p.character=c.id JOIN fw_gw_progression_imports i ON i.character=c.id WHERE c.account=? AND c.id=?");
  source.text(1,f[0]);source.text(2,f[1]);if(!source.row())return AccountResult{FW_NOT_FOUND,"owned imported progression missing"};
  if(source.number(0)!=expected)return AccountResult{FW_CONFLICT,"title seed character revision changed"};
  Query existing(db,"SELECT 1 FROM fw_gw_account_titles WHERE account=?");existing.text(1,f[0]);
  if(existing.row())return AccountResult{FW_CONFLICT,"account titles already initialized"};
  const auto profile=gw::progression::account_titles(gw::progression::decode_character(source.blob(1)));
  allowed(permitted);Query write(db,"INSERT INTO fw_gw_account_titles VALUES(?,1,?,?)");
  write.text(1,f[0]);write.blob(2,gw::progression::encode(profile));write.text(3,source.text(2));write.row();
  allowed(permitted);tx.commit();return AccountResult{FW_OK,"1"};
 }
 if(op==gwserver::Operation::maintenanceGwCharacterProgressionImport){
  if(f.size()!=7||!social_id(f[0])||!social_id(f[1])||f[6].size()!=64||f[6].find_first_not_of("0123456789abcdef")!=f[6].npos)return AccountResult{FW_INVALID,"progression import fields"};
  const auto expected_faction=number(f[2],INT64_MAX-1),expected_character=number(f[3],INT64_MAX-1);
  const auto decode=[](const std::string& hex){if(hex.empty()||hex.size()%2||hex.size()>131072)throw std::invalid_argument("progression import bound");std::vector<std::uint8_t> bytes(hex.size()/2);if(sodium_hex2bin(bytes.data(),bytes.size(),hex.data(),hex.size(),nullptr,nullptr,nullptr))throw std::invalid_argument("progression import encoding");return bytes;};
  const auto faction=decode(f[4]),character=decode(f[5]);
  (void)gw::progression::decode_factions(faction);const auto shared_titles=gw::progression::account_titles(gw::progression::decode_character(character));
  Transaction tx(db);Query owner(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");owner.text(1,f[0]);owner.text(2,f[1]);if(!owner.row())return AccountResult{FW_NOT_FOUND,"owned progression missing"};
  Query current_faction(db,"SELECT revision FROM fw_gw_factions WHERE account=?");current_faction.text(1,f[0]);const auto faction_revision=current_faction.row()?current_faction.number(0):0;
  Query current_character(db,"SELECT revision FROM fw_gw_progression WHERE character=?");current_character.text(1,f[1]);const auto character_revision=current_character.row()?current_character.number(0):0;
  if(expected_faction!=faction_revision||expected_character!=character_revision)return AccountResult{FW_CONFLICT,"progression import revisions changed"};
  allowed(permitted);
  Query write_faction(db,"INSERT INTO fw_gw_factions VALUES(?,?,?) ON CONFLICT(account) DO UPDATE SET revision=excluded.revision,profile=excluded.profile");write_faction.text(1,f[0]);write_faction.number(2,faction_revision+1);write_faction.blob(3,faction);write_faction.row();
  Query write_character(db,"INSERT INTO fw_gw_progression VALUES(?,?,?) ON CONFLICT(character) DO UPDATE SET revision=excluded.revision,profile=excluded.profile");write_character.text(1,f[1]);write_character.number(2,character_revision+1);write_character.blob(3,character);write_character.row();
  // This import is the only account-title writer. The faction revision CAS
  // serializes account imports even when they target different characters.
  Query write_titles(db,"INSERT INTO fw_gw_account_titles VALUES(?,1,?,?) ON CONFLICT(account) DO UPDATE SET revision=revision+1,profile=excluded.profile,source=excluded.source");write_titles.text(1,f[0]);write_titles.blob(2,gw::progression::encode(shared_titles));write_titles.text(3,f[6]);write_titles.row();
  Query source(db,"INSERT INTO fw_gw_progression_imports VALUES(?,?) ON CONFLICT(character) DO UPDATE SET source=excluded.source");source.text(1,f[1]);source.text(2,f[6]);source.row();
  allowed(permitted);tx.commit();return AccountResult{FW_OK,std::to_string(faction_revision+1)+"\t"+std::to_string(character_revision+1)};
 }
 if(op==gwserver::Operation::awardFaction||op==gwserver::Operation::exchangeImperialFaction){
  if(f.size()!=4||!social_id(f[0]))return AccountResult{FW_INVALID,"faction mutation fields"};
  const auto account=gwserver::Identity::from_hex(f[0]);const auto revision=number(f[1],INT64_MAX-1);
  const auto target=static_cast<gw::progression::Faction>(number(f[2],op==gwserver::Operation::awardFaction?3:2));
  const auto amount=static_cast<std::uint32_t>(number(f[3],UINT32_MAX));
  const auto result=op==gwserver::Operation::awardFaction?award_faction(db,{account,revision,target,amount},permitted):exchange_faction(db,{account,revision,target,amount},permitted);
  return AccountResult{result.status,result.status==FW_OK?std::to_string(result.revision):result.message};
 }
 if(op!=gwserver::Operation::accountFactions&&op!=gwserver::Operation::saveFactions)return {};
 const bool get=op==gwserver::Operation::accountFactions;
 if(f.size()!=(get?1:3)||!social_id(f[0]))return AccountResult{FW_INVALID,"faction fields"};
 const auto account=gwserver::Identity::from_hex(f[0]);
 if(get){const auto result=read_factions(db,account,permitted);if(result.status!=FW_OK)return AccountResult{result.status,result.message};
  return AccountResult{FW_OK,result.record?std::to_string(result.record->revision)+"\t"+hex_bytes(gw::progression::encode(result.record->profile)):"0"};}
 const auto result=write_factions(db,{account,number(f[1],INT64_MAX-1),gw::progression::decode_factions(gwserver::profile_bytes(f[2],52))},permitted);
 return AccountResult{result.status,result.status==FW_OK?std::to_string(result.revision):result.message};
}

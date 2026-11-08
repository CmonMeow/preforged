gwserver::MapTransitionResult transition_character_map(sqlite3* db,const gwserver::SelectMap& request,const std::function<bool()>& permitted,bool allow_zero=false){
 allowed(permitted);
 if(request.expected_map==request.destination||request.expected_map>65535||request.destination>65535||
    (!allow_zero&&(!request.expected_map||!request.destination)))return {FW_INVALID,"invalid character map transition"};
 Transaction tx(db);
 const auto account=gwserver::storage_hex(request.account.bytes),character=gwserver::storage_hex(request.character.bytes);
 Query read(db,"SELECT summary FROM fw_gw_characters WHERE account=? AND id=? AND map_id=?");
 read.text(1,account);read.text(2,character);read.number(3,request.expected_map);
 if(!read.row())return {FW_CONFLICT,"character map changed or not owned"};
 auto blob=read.blob(0);
 if(!blob.empty()){
  auto profile=gw::CharacterProfile::decode(blob);
  if(profile.last_map_id!=request.expected_map)throw SqlError("stored character profile map mismatch");
  profile.last_map_id=static_cast<std::uint16_t>(request.destination);blob=profile.encode();
 }
 Query update(db,"UPDATE fw_gw_characters SET map_id=?,position_x=0,position_y=0,summary=? WHERE account=? AND id=? AND map_id=?");
 update.number(1,request.destination);update.blob(2,blob);update.text(3,account);update.text(4,character);update.number(5,request.expected_map);update.row();
 if(sqlite3_changes(db)!=1)return {FW_CONFLICT,"character map changed or not owned"};
 allowed(permitted);tx.commit();return {FW_OK,{}};
}
gwserver::DeletionResult delete_character(sqlite3* db,const gwserver::DeleteCharacter& request,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);
 Query remove(db,"DELETE FROM fw_gw_characters WHERE account=? AND id=?");
 remove.text(1,gwserver::storage_hex(request.account.bytes));remove.text(2,gwserver::storage_hex(request.character.bytes));remove.row();
 if(sqlite3_changes(db)!=1)return {FW_NOT_FOUND,"character not owned by account"};
 allowed(permitted);tx.commit();return {FW_OK,{}};
}
gwserver::CharacterNumberResult read_character_number(sqlite3* db,const gwserver::OwnedCharacter& owner,bool map,const std::function<bool()>& permitted,bool allow_zero_map=false){
 allowed(permitted);Transaction tx(db);
 Query read(db,map?"SELECT map_id FROM fw_gw_characters WHERE account=? AND id=?":"SELECT experience FROM fw_gw_characters WHERE account=? AND id=?");
 read.text(1,gwserver::storage_hex(owner.account.bytes));read.text(2,gwserver::storage_hex(owner.character.bytes));
 if(!read.row())return {FW_NOT_FOUND,"character not owned by account",{}};
 if(!map&&sqlite3_column_type(read.statement,0)==SQLITE_NULL){allowed(permitted);return {FW_OK,{},{}};}
 const auto value=read.number(0);if(value<0||value>UINT32_MAX||(map&&((!value&&!allow_zero_map)||value>65535)))throw std::invalid_argument("invalid stored character number");
 allowed(permitted);return {FW_OK,{},static_cast<std::uint32_t>(value)};
}
gwserver::CharacterNameResult read_character_name(sqlite3* db,const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);
 Query name(db,"SELECT name FROM fw_gw_characters WHERE account=? AND id=?");
 name.text(1,gwserver::storage_hex(owner.account.bytes));name.text(2,gwserver::storage_hex(owner.character.bytes));
 if(!name.row())return {FW_NOT_FOUND,"owned character unavailable",{}};
 auto value=name.text(0);(void)gw::character_name_to_utf16(value);allowed(permitted);
 return {FW_OK,{},std::move(value)};
}
gwserver::DevelopmentUnlockResult unlock_character(sqlite3* db,const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted){
    allowed(permitted);const auto account=gwserver::storage_hex(owner.account.bytes),character=gwserver::storage_hex(owner.character.bytes);
        Transaction tx(db);Query owned(db,"SELECT summary FROM fw_gw_characters WHERE account=? AND id=?");owned.text(1,account);owned.text(2,character);
        if(!owned.row())return gwserver::DevelopmentUnlockResult{FW_NOT_FOUND,"owned development character unavailable"};
        const auto roster=gw::CharacterProfile::decode(owned.blob(0));
        if(roster.extension_count<32||((roster.extended[12]>>4)&31)!=20)return gwserver::DevelopmentUnlockResult{FW_INVALID,"development grant requires level20 summary"};
        const auto raw=roster.encode();std::uint32_t appearance=0;for(unsigned i=0;i<4;++i)appearance|=std::uint32_t(raw.at(14+i))<<(8*i);
        const unsigned primary=(appearance>>20)&15,secondary=(roster.extended[12]>>10)&15;
        if(!primary||primary>10||secondary>10)return gwserver::DevelopmentUnlockResult{FW_INVALID,"development summary professions unavailable"};
        Query oldskills(db,"SELECT revision,profile FROM fw_gw_character_skills WHERE character=?");oldskills.text(1,character);const bool has_skills=oldskills.row();
        const auto skill_revision=has_skills?oldskills.number(0):0;gw::skills::Profile profile;
        if(has_skills)profile=gw::skills::decode_profile(oldskills.blob(1));else{profile.primary=static_cast<std::uint8_t>(primary);profile.secondary=static_cast<std::uint8_t>(secondary);}
        if(profile.primary!=primary||profile.secondary!=secondary)return gwserver::DevelopmentUnlockResult{FW_INVALID,"development profile summary mismatch"};
        Query oldattrs(db,"SELECT revision,profile FROM fw_gw_character_attributes WHERE character=?");oldattrs.text(1,character);const bool has_attrs=oldattrs.row();
        const auto attribute_revision=has_attrs?oldattrs.number(0):0;auto allocation=has_attrs?gw::attributes::Profile::decode(oldattrs.blob(1)):gw::attributes::Profile{};
        std::uint32_t campaigns=0;Query entitlements(db,"SELECT campaign FROM fw_gw_campaign_entitlements WHERE account=?");entitlements.text(1,account);while(entitlements.row()){const auto campaign=entitlements.number(0);if(campaign<1||campaign>4)throw SqlError("invalid stored campaign entitlement");campaigns|=1u<<campaign;}
        std::uint32_t professions=0;for(unsigned profession=1;profession<=10;++profession){const unsigned campaign=profession<=6?0:profession<=8?2:3;if(!campaign||(campaigns&(1u<<campaign)))professions|=1u<<profession;}
        profile.profession_mask|=professions&~(1u<<primary);
        for(auto id:gw::skills::obtainable_player_skills){const auto* row=gw::skills::find_skill(id);if(row&&(!row->campaign||(campaigns&(1u<<row->campaign))))profile.learned.try_emplace(id,1);}
        allocation.source=gw::attributes::PointsSource::development;allocation.quest_bonus=30;
        if(gw::attributes::validate(allocation.ranks,{profile.primary,profile.secondary,allocation.total_points(20),true,true})!=gw::attributes::Reason::accepted)return gwserver::DevelopmentUnlockResult{FW_INVALID,"development existing allocation invalid"};
        const auto skill_bytes=gw::skills::encode_profile(profile);const auto encoded_attributes=allocation.encode();const std::vector<std::uint8_t> attribute_bytes(encoded_attributes.begin(),encoded_attributes.end());
        if((has_skills&&(skill_revision<1||skill_revision==INT64_MAX))||(has_attrs&&(attribute_revision<1||attribute_revision==INT64_MAX)))throw SqlError("invalid development revision");
        const bool change_skills=!has_skills||skill_bytes!=oldskills.blob(1),change_attrs=!has_attrs||attribute_bytes!=oldattrs.blob(1);
        allowed(permitted);
        if(change_skills){Query write(db,"INSERT INTO fw_gw_character_skills(character,revision,profile,source) VALUES(?,?,?,?) ON CONFLICT(character) DO UPDATE SET revision=excluded.revision,profile=excluded.profile,source=excluded.source");write.text(1,character);write.number(2,skill_revision+1);write.blob(3,skill_bytes);write.text(4,"private-development");write.row();}
        if(change_attrs){Query write(db,"INSERT INTO fw_gw_character_attributes(character,revision,profile,source) VALUES(?,?,?,?) ON CONFLICT(character) DO UPDATE SET revision=excluded.revision,profile=excluded.profile,source=excluded.source");write.text(1,character);write.number(2,attribute_revision+1);write.blob(3,attribute_bytes);write.text(4,"private-development");write.row();}
        allowed(permitted);tx.commit();return {FW_OK,{},static_cast<std::uint64_t>(skill_revision+change_skills),static_cast<std::uint64_t>(attribute_revision+change_attrs),static_cast<std::uint32_t>(profile.learned.size())};
}
gwserver::CharacterNameResult rename_character(sqlite3* db,const gwserver::RenameCharacter& request,const std::function<bool()>& permitted){
    allowed(permitted);const auto account=gwserver::storage_hex(request.owner.account.bytes),character=gwserver::storage_hex(request.owner.character.bytes);
    (void)fw::gw::character_name_to_utf16(request.expected_name);(void)fw::gw::character_name_to_utf16(request.replacement);
    Transaction tx(db);Query current(db,"SELECT name FROM fw_gw_characters WHERE account=? AND id=?");current.text(1,account);current.text(2,character);
    if(!current.row())return gwserver::CharacterNameResult{FW_NOT_FOUND,"owned rename character unavailable"};
    if(current.text(0)!=request.expected_name)return gwserver::CharacterNameResult{FW_CONFLICT,"character name changed in another session"};
    Query duplicate(db,"SELECT 1 FROM fw_gw_characters WHERE name=? COLLATE NOCASE AND id<>?");duplicate.text(1,request.replacement);duplicate.text(2,character);if(duplicate.row())return gwserver::CharacterNameResult{FW_CONFLICT,"character name already used"};
    allowed(permitted);Query write(db,"UPDATE fw_gw_characters SET name=? WHERE account=? AND id=? AND name=? COLLATE BINARY");write.text(1,request.replacement);write.text(2,account);write.text(3,character);write.text(4,request.expected_name);write.row();
    if(sqlite3_changes(db)!=1)throw SqlError("rename compare and swap");allowed(permitted);tx.commit();return {FW_OK,{},request.replacement};
}
std::optional<AccountResult> development_execute(sqlite3* db,gwserver::Operation operation,const std::vector<std::string>& f,const std::function<bool()>& permitted){
    if(operation==gwserver::Operation::unlockDevelopmentCharacter){
        if(f.size()!=2||!social_id(f[0])||!social_id(f[1]))return AccountResult{FW_INVALID,"invalid development character identity"};
        const auto result=unlock_character(db,{gwserver::Identity::from_hex(f[0]),gwserver::Identity::from_hex(f[1])},permitted);
        return AccountResult{result.status,result.status==FW_OK?std::to_string(result.skill_revision)+"\t"+std::to_string(result.attribute_revision)+"\t"+std::to_string(result.learned):result.message};
    }
    if(operation==gwserver::Operation::characterName){
        if(f.size()!=2||!social_id(f[0])||!social_id(f[1]))return AccountResult{FW_INVALID,"invalid character identity"};
        const auto result=read_character_name(db,{gwserver::Identity::from_hex(f[0]),gwserver::Identity::from_hex(f[1])},permitted);
        return AccountResult{result.status,result.status==FW_OK?*result.value:result.message};
    }
    if(operation!=gwserver::Operation::renameCharacter)return {};
    if(f.size()!=4||!social_id(f[0])||!social_id(f[1]))return AccountResult{FW_INVALID,"rename owner/character fields"};
    const auto result=rename_character(db,{{gwserver::Identity::from_hex(f[0]),gwserver::Identity::from_hex(f[1])},f[2],f[3]},permitted);
    return AccountResult{result.status,result.status==FW_OK?*result.value:result.message};
}

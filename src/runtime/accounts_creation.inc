// One transaction for typed production creation and offline compatibility.
gwserver::CreationResult create_character(sqlite3* db,const gwserver::CreateCharacter& request,const std::function<bool()>& permitted){
    allowed(permitted);
    const auto account=gwserver::storage_hex(request.account.bytes);
    try{gwserver::validate_creation_name(request.name);}
    catch(const std::invalid_argument&){return {FW_INVALID,"invalid character creation request",{}};}
    // Creation can commit the character and its retail summary together.
    // Never leave a new roster entry without a profile if this variant
    // fails validation or the transaction is cancelled.
    std::vector<std::uint8_t> creation_profile;
    std::optional<gw::inventory::Profile> creation_inventory;
    std::optional<gw::skills::Profile> creation_skills;
    std::uint16_t creation_map=148;
    if(request.profile){
        creation_profile=request.profile->encode();
        try{
            const auto profile=gw::CharacterProfile::decode(creation_profile);
            if(profile.unknown1!=6)throw std::invalid_argument("creation requires 2022 summary");
            const auto equipment=profile.equipment_2022();
            const auto appearance=std::uint32_t(profile.extended[2])|(std::uint32_t(profile.extended[3])<<16);
            const auto primary=(appearance>>20)&15;
            // This is creation, not opaque roster import. A selected
            // profession must exist before committing the character;
            // otherwise first world entry falls back to a different
            // class and no durable skill profile is available.
            if(primary<1||primary>10)throw std::invalid_argument("invalid creation profession");
            if(primary>=1&&primary<=10){
                gw::skills::Profile skills;
                skills.primary=static_cast<std::uint8_t>(primary);
                // Secondary professions become available through progression.
                skills.profession_mask=0;
                creation_skills=std::move(skills);
            }
            if(primary>=1&&primary<=10&&!equipment.empty()){
                // Persist gear with creation instead of depending on first
                // world entry. Level-one PvE uses reviewed starter ratings;
                // appearance alone never establishes gameplay item stats.
                const auto level=(profile.extended[12]>>4)&31;
                const bool pvp=(profile.extended[12]&(1u<<9))!=0;
                const unsigned campaign=profile.last_map_id==212?2:profile.last_map_id==490?3:1;
                auto gear=level==1&&!pvp?
                    gw::energy::creation_starter_armor(primary,campaign,equipment,gw::character_name_to_utf16(request.name,31)):
                    gw::energy::private_starter_armor(primary,equipment,gw::character_name_to_utf16(request.name,31));
                if(level==1&&!pvp&&campaign==1&&primary>=1&&primary<=6){
                    const auto armed=gw::inventory::presearing::creation_weapon(gear,primary,gw::character_name_to_utf16(request.name,19));
                    if(!armed)throw std::invalid_argument("captured creation weapon could not be installed");
                    gear=*armed;
                }
                if(!gear.items.empty())creation_inventory=std::move(gear);
            }
            creation_map=profile.last_map_id;
        }catch(const std::invalid_argument&){return {FW_INVALID,"invalid creation profile layout"};}
    }
    Transaction tx(db);
    Query owner(db,"SELECT 1 FROM fw_accounts WHERE id=?"); owner.text(1,account);
    if(!owner.row()) return {FW_NOT_FOUND,"account missing"};
    const unsigned creation_campaign=creation_map==212?2:creation_map==490?3:1;
    Query entitlement(db,"SELECT 1 FROM fw_gw_campaign_entitlements WHERE account=? AND campaign=?"); entitlement.text(1,account);entitlement.number(2,creation_campaign);
    if(!entitlement.row()) return {FW_NOT_FOUND,"Creation campaign entitlement required"};
    // Character names are public identities for whispers, friends and
    // trade. Creation must use the same global namespace as rename.
    Query duplicate(db,"SELECT 1 FROM fw_gw_characters WHERE name=? COLLATE NOCASE");
    duplicate.text(1,request.name);
    if(duplicate.row()) return {FW_CONFLICT,"character name unavailable"};
    Query count(db,"SELECT count(*) FROM fw_gw_characters WHERE account=?"); count.text(1,account); count.row();
    if(count.number(0)>=8) return {FW_OVERLOADED,"character capacity"};
    auto id=random_hex(16);
    Query order(db,"SELECT COALESCE(MAX(roster_order)+1,0) FROM fw_gw_characters WHERE account=?");
    order.text(1,account);order.row();
    Query insert(db,"INSERT INTO fw_gw_characters(id,account,name,map_id,roster_order,summary,experience) VALUES(?,?,?,?,?,?,0)");
    insert.text(1,id); insert.text(2,account); insert.text(3,request.name);
    insert.number(4,creation_map);insert.number(5,order.number(0));insert.blob(6,creation_profile);insert.row();
    if(creation_skills){
        allowed(permitted);
        std::array<unsigned char,crypto_hash_sha256_BYTES> digest{};
        crypto_hash_sha256(digest.data(),creation_profile.data(),creation_profile.size());
        Query skills(db,"INSERT INTO fw_gw_character_skills VALUES(?,1,?,?)");
        skills.text(1,id);skills.blob(2,gw::skills::encode_profile(*creation_skills));
        skills.text(3,hex_bytes(digest));skills.row();
    }
    if(creation_inventory){
        allowed(permitted);
        Query inventory(db,"INSERT INTO fw_gw_inventory(character,revision,profile) VALUES(?,1,?)");
        inventory.text(1,id);inventory.blob(2,gw::inventory::encode(*creation_inventory));inventory.row();
    }
    allowed(permitted); tx.commit();
    return {FW_OK,{},gwserver::CreatedCharacter{gwserver::Identity::from_hex(id),request.name,creation_map}};
}

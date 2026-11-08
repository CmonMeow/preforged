gwserver::ProfessionSaveResult write_profession(sqlite3* db,const gwserver::SaveSecondaryProfession& request,const std::function<bool()>& permitted){
 allowed(permitted);gwserver::check_revision(request.skill_revision);gwserver::check_revision(request.attribute_revision);
 gw::skills::validate_profile(request.skills);(void)request.attributes.encode();
 const auto account=gwserver::storage_hex(request.owner.account.bytes),character=gwserver::storage_hex(request.owner.character.bytes);
            Transaction tx(db);Query owner(db,"SELECT summary FROM fw_gw_characters WHERE account=? AND id=?");
            owner.text(1,account);owner.text(2,character);if(!owner.row())return {FW_NOT_FOUND,"owned character not found"};
            Query old_skills(db,"SELECT revision,profile FROM fw_gw_character_skills WHERE character=?");old_skills.text(1,character);
            if(!old_skills.row())return {FW_NOT_FOUND,"secondary unlocks unavailable for uncaptured character"};
            Query old_attributes(db,"SELECT revision,profile FROM fw_gw_character_attributes WHERE character=?");old_attributes.text(1,character);
            const bool has_attributes=old_attributes.row();const auto skill_revision=old_skills.number(0),attribute_revision=has_attributes?old_attributes.number(0):0;
            if(request.skill_revision!=std::uint64_t(skill_revision)||request.attribute_revision!=std::uint64_t(attribute_revision))
                return {FW_CONFLICT,"secondary profession revision conflict"};
            const auto old_profile=gw::skills::decode_profile(old_skills.blob(1));const auto& profile=request.skills;
            const auto old_allocation=has_attributes?gw::attributes::Profile::decode(old_attributes.blob(1)):gw::attributes::Profile{};
            const auto& allocation=request.attributes;
            if(profile.primary!=old_profile.primary||profile.profession_mask!=old_profile.profession_mask||profile.learned!=old_profile.learned||
                profile.trial_skills!=(profile.secondary!=old_profile.secondary?std::set<std::uint32_t>{}:old_profile.trial_skills)||
                profile.secondary==profile.primary||(!profile.secondary&&old_profile.secondary)||profile.secondary&&!(old_profile.profession_mask&(1u<<profile.secondary))||
                allocation.source!=old_allocation.source||allocation.quest_bonus!=old_allocation.quest_bonus)
                return {FW_INVALID,"secondary change cannot alter character learning or entitlements"};
            const auto summary=owner.blob(0);unsigned level=1;
            if(!summary.empty()){const auto roster=gw::CharacterProfile::decode(summary);if(roster.extension_count>=32)level=(roster.extended[12]>>4)&31;}
            if(gw::attributes::validate(allocation.ranks,{profile.primary,profile.secondary,allocation.total_points(level),true,true})!=gw::attributes::Reason::accepted)
                return {FW_INVALID,"secondary allocation invalid for professions or points"};
            // A profession switch may clear incompatible slots/ranks only. It
            // cannot equip a new skill or secretly spend/rearrange attributes.
            for(unsigned n=0;n<8;++n)if(profile.bar[n]!=old_profile.bar[n]&&profile.bar[n].skill)return {FW_INVALID,"secondary change cannot equip new skills"};
            for(unsigned id=0;id<51;++id)if(allocation.ranks[id]!=old_allocation.ranks[id]&&allocation.ranks[id])return {FW_INVALID,"secondary change cannot allocate points"};
            allowed(permitted);const auto skill_bytes=gw::skills::encode_profile(profile);const auto attribute_bytes=allocation.encode();
            Query skills(db,"UPDATE fw_gw_character_skills SET revision=?,profile=? WHERE character=? AND revision=?");
            skills.number(1,skill_revision+1);skills.blob(2,skill_bytes);skills.text(3,character);skills.number(4,skill_revision);skills.row();
            if(sqlite3_changes(db)!=1)return {FW_CONFLICT,"secondary skill revision conflict"};
            if(has_attributes){Query attrs(db,"UPDATE fw_gw_character_attributes SET revision=?,profile=? WHERE character=? AND revision=?");
                attrs.number(1,attribute_revision+1);attrs.blob(2,attribute_bytes);attrs.text(3,character);attrs.number(4,attribute_revision);attrs.row();
                if(sqlite3_changes(db)!=1)return {FW_CONFLICT,"secondary attribute revision conflict"};
            }else{Query attrs(db,"INSERT INTO fw_gw_character_attributes VALUES(?,1,?,?)");attrs.text(1,character);attrs.blob(2,attribute_bytes);attrs.text(3,std::string(64,'0'));attrs.row();}
            if(!summary.empty()){
                auto roster=gw::CharacterProfile::decode(summary);
                if(roster.extension_count<32)return {FW_INVALID,"secondary roster metadata unavailable"};
                roster.extended[12]=static_cast<std::uint16_t>((roster.extended[12]&~(15u<<10))|(unsigned(profile.secondary)<<10));
                const auto updated_summary=roster.encode();
                Query update_roster(db,"UPDATE fw_gw_characters SET summary=? WHERE account=? AND id=?");
                update_roster.blob(1,updated_summary);update_roster.text(2,account);update_roster.text(3,character);update_roster.row();
                if(sqlite3_changes(db)!=1)return {FW_NOT_FOUND,"secondary character disappeared"};
            }
            allowed(permitted);tx.commit();return {FW_OK,{},static_cast<std::uint64_t>(skill_revision+1),static_cast<std::uint64_t>(attribute_revision+1)};
}

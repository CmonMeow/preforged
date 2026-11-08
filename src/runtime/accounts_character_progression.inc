gwserver::ProgressionResult read_character_progression(sqlite3* db,const gwserver::OwnedCharacter& owned,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);
 const auto account=gwserver::storage_hex(owned.account.bytes),character=gwserver::storage_hex(owned.character.bytes);
 Query owner(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");owner.text(1,account);owner.text(2,character);
 if(!owner.row())return {FW_NOT_FOUND,"owned progression character missing",{}};
 Query current(db,"SELECT revision,profile FROM fw_gw_progression WHERE character=?");current.text(1,character);
 const bool present=current.row();const auto revision=present?current.number(0):0;
 if(revision<0||(present&&revision==0))throw SqlError("invalid progression revision");
 auto profile=present?gw::progression::decode_character(current.blob(1)):gw::progression::Character{};
 Query shared(db,"SELECT profile FROM fw_gw_account_titles WHERE account=?");shared.text(1,account);const bool has_shared=shared.row();
 if(has_shared)profile=gw::progression::with_account_titles(profile,gw::progression::decode_character(shared.blob(0)));
 allowed(permitted);if(!present&&!has_shared)return {FW_OK,{},{}};
 return {FW_OK,{},gwserver::VersionedProfile<gw::progression::Character>{static_cast<std::uint64_t>(revision),std::move(profile)}};
}
gwserver::RevisionResult write_character_progression(sqlite3* db,const gwserver::SaveProgression& request,const std::function<bool()>& permitted){
 allowed(permitted);gwserver::check_revision(request.expected_revision);
 const auto bytes=gw::progression::encode(request.profile);
 const auto account=gwserver::storage_hex(request.owner.account.bytes),character=gwserver::storage_hex(request.owner.character.bytes);
 Transaction tx(db);Query owner(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");owner.text(1,account);owner.text(2,character);
 if(!owner.row())return {FW_NOT_FOUND,"owned progression character missing"};
 Query current(db,"SELECT revision FROM fw_gw_progression WHERE character=?");current.text(1,character);
 const auto revision=current.row()?current.number(0):0;
 if(revision<0||static_cast<std::uint64_t>(revision)!=request.expected_revision)return {FW_CONFLICT,"progression revision changed"};
 allowed(permitted);Query write(db,"INSERT INTO fw_gw_progression VALUES(?,?,?) ON CONFLICT(character) DO UPDATE SET revision=excluded.revision,profile=excluded.profile");
 write.text(1,character);write.number(2,revision+1);write.blob(3,bytes);write.row();allowed(permitted);tx.commit();
 return {FW_OK,{},static_cast<std::uint64_t>(revision+1)};
}

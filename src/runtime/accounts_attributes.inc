gwserver::AttributesResult read_attributes(sqlite3* db,const gwserver::OwnedCharacter& owned,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);
 Query owner(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");
 owner.text(1,gwserver::storage_hex(owned.account.bytes));owner.text(2,gwserver::storage_hex(owned.character.bytes));
 if(!owner.row())return {FW_NOT_FOUND,"owned character not found",{}};
 Query current(db,"SELECT revision,profile FROM fw_gw_character_attributes WHERE character=?");current.text(1,gwserver::storage_hex(owned.character.bytes));
 if(!current.row()){allowed(permitted);return {FW_OK,{},{}};}
 const auto revision=current.number(0);if(revision<=0)throw SqlError("invalid attribute revision");
 auto profile=gw::attributes::Profile::decode(current.blob(1));allowed(permitted);
 return {FW_OK,{},gwserver::VersionedProfile<gw::attributes::Profile>{static_cast<std::uint64_t>(revision),std::move(profile)}};
}

gwserver::RevisionResult write_attributes(sqlite3* db,const gwserver::SaveAttributes& request,const std::function<bool()>& permitted){
 allowed(permitted);gwserver::check_revision(request.expected_revision);Transaction tx(db);
 const auto character=gwserver::storage_hex(request.owner.character.bytes);
 Query owner(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");owner.text(1,gwserver::storage_hex(request.owner.account.bytes));owner.text(2,character);
 if(!owner.row())return {FW_NOT_FOUND,"owned character not found"};
 Query current(db,"SELECT revision,profile FROM fw_gw_character_attributes WHERE character=?");current.text(1,character);
 const bool present=current.row();const auto revision=present?current.number(0):0;
 if(revision<0||static_cast<std::uint64_t>(revision)!=request.expected_revision)return {FW_CONFLICT,"attribute revision conflict"};
 const auto packed=request.profile.encode();
 if(!present){if(packed[1]!=0||packed[2]!=0)return {FW_INVALID,"uncaptured attributes require baseline entitlement"};}
 else {const auto prior=gw::attributes::Profile::decode(current.blob(1));if(prior.source!=request.profile.source||prior.quest_bonus!=request.profile.quest_bonus)return {FW_INVALID,"attribute edit cannot change entitlement"};}
 allowed(permitted);
 if(!present){Query insert(db,"INSERT INTO fw_gw_character_attributes VALUES(?,?,?,?)");insert.text(1,character);insert.number(2,1);insert.blob(3,packed);insert.text(4,std::string(64,'0'));insert.row();allowed(permitted);tx.commit();return {FW_OK,{},1};}
 Query update(db,"UPDATE fw_gw_character_attributes SET revision=?,profile=? WHERE character=? AND revision=?");
 update.number(1,revision+1);update.blob(2,packed);update.text(3,character);update.number(4,revision);update.row();
 if(sqlite3_changes(db)!=1)return {FW_CONFLICT,"attribute revision conflict"};
 allowed(permitted);tx.commit();return {FW_OK,{},static_cast<std::uint64_t>(revision+1)};
}

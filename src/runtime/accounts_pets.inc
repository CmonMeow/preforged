gwserver::PetsResult read_pets(sqlite3* db,const gwserver::OwnedCharacter& owned,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);
 const auto character=gwserver::storage_hex(owned.character.bytes);
 Query owner(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");owner.text(1,gwserver::storage_hex(owned.account.bytes));owner.text(2,character);
 if(!owner.row())return {FW_NOT_FOUND,"owned pet character missing",{}};
 Query current(db,"SELECT revision,profile FROM fw_gw_pets WHERE character=?");current.text(1,character);
 if(!current.row()){allowed(permitted);return {FW_OK,{},gwserver::VersionedProfile<gw::pets::Profile>{0,{}}};}
 const auto revision=current.number(0);if(revision<=0)throw SqlError("invalid pet revision");
 auto profile=gw::pets::decode(current.blob(1));allowed(permitted);
 return {FW_OK,{},gwserver::VersionedProfile<gw::pets::Profile>{static_cast<std::uint64_t>(revision),std::move(profile)}};
}
gwserver::RevisionResult adopt_pet(sqlite3* db,const gwserver::AdoptPet& request,const std::function<bool()>& permitted){
 allowed(permitted);gwserver::check_revision(request.expected_revision);
 gw::pets::validate(request.expected);gw::pets::validate(request.profile);
 // The reviewed captured adoption is a new, unevolved level5 Stalker. This
 // endpoint cannot grant levels, rename, release or replace an existing pet.
 if(request.expected.pet||!request.profile.pet||request.profile.pet->level!=5||request.profile.pet->experience||
    request.profile.pet->evolution!=gw::pets::Evolution::unevolved||!request.profile.pet->name.empty())
  return {FW_INVALID,"invalid initial pet adoption"};
 Transaction tx(db);const auto character=gwserver::storage_hex(request.owner.character.bytes);
 Query owner(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");owner.text(1,gwserver::storage_hex(request.owner.account.bytes));owner.text(2,character);
 if(!owner.row())return {FW_NOT_FOUND,"owned pet character missing"};
 Query current(db,"SELECT revision,profile FROM fw_gw_pets WHERE character=?");current.text(1,character);
 const bool present=current.row();const auto revision=present?current.number(0):0;
 if(revision<0||static_cast<std::uint64_t>(revision)!=request.expected_revision)return {FW_CONFLICT,"pet revision conflict"};
 const auto before=present?gw::pets::decode(current.blob(1)):gw::pets::Profile{};
 if(before!=request.expected||before.pet)return {FW_CONFLICT,"pet adoption state changed"};
 const auto packed=gw::pets::encode(request.profile);allowed(permitted);
 if(present){Query update(db,"UPDATE fw_gw_pets SET revision=?,profile=? WHERE character=? AND revision=?");
  update.number(1,revision+1);update.blob(2,packed);update.text(3,character);update.number(4,revision);update.row();}
 else{Query insert(db,"INSERT INTO fw_gw_pets(character,revision,profile) VALUES(?,?,?)");insert.text(1,character);insert.number(2,1);insert.blob(3,packed);insert.row();}
 if(sqlite3_changes(db)!=1)return {FW_CONFLICT,"pet adoption state changed"};
 allowed(permitted);tx.commit();return {FW_OK,{},static_cast<std::uint64_t>(revision+1)};
}

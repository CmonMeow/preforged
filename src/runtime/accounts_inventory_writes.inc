gwserver::StorageTransferResult write_inventory(sqlite3* db,const gwserver::OwnedCharacter& owned,std::uint64_t expected,const gw::inventory::Profile& profile,const gw::inventory::Profile* bank,std::uint64_t bank_expected,const std::function<bool()>& permitted){
 allowed(permitted);gwserver::check_revision(expected);gw::inventory::validate_character(profile);
 if(bank){gwserver::check_revision(bank_expected);gw::storage::validate(*bank);}
 const auto account=gwserver::storage_hex(owned.account.bytes),character=gwserver::storage_hex(owned.character.bytes);
 Transaction tx(db);Query owner(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");owner.text(1,account);owner.text(2,character);
 if(!owner.row())return {FW_NOT_FOUND,"owned inventory not found"};
 Query current(db,"SELECT revision FROM fw_gw_inventory WHERE character=?");current.text(1,character);const auto revision=current.row()?current.number(0):0;
 if(revision<0||expected!=static_cast<std::uint64_t>(revision))return {FW_CONFLICT,"inventory changed in another session"};
 std::int64_t bank_revision=0;
 if(bank){Query stored(db,"SELECT revision FROM fw_gw_storage WHERE account=?");stored.text(1,account);if(stored.row())bank_revision=stored.number(0);
  if(bank_revision<0||bank_expected!=static_cast<std::uint64_t>(bank_revision))return {FW_CONFLICT,"account storage changed in another session"};}
 const auto packed=gw::inventory::encode(profile);allowed(permitted);
 Query write(db,"INSERT INTO fw_gw_inventory VALUES(?,?,?) ON CONFLICT(character) DO UPDATE SET revision=excluded.revision,profile=excluded.profile");write.text(1,character);write.number(2,revision+1);write.blob(3,packed);write.row();
 if(bank){const auto packed_bank=gw::inventory::encode(*bank);Query write_bank(db,"INSERT INTO fw_gw_storage VALUES(?,?,?) ON CONFLICT(account) DO UPDATE SET revision=excluded.revision,profile=excluded.profile");write_bank.text(1,account);write_bank.number(2,bank_revision+1);write_bank.blob(3,packed_bank);write_bank.row();}
 allowed(permitted);tx.commit();return {FW_OK,{},static_cast<std::uint64_t>(revision+1),bank?static_cast<std::uint64_t>(bank_revision+1):0};
}

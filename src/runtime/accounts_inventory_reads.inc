gwserver::InventoryResult read_inventory(sqlite3* db,const gwserver::OwnedCharacter& owned,bool bank,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);
 const auto account=gwserver::storage_hex(owned.account.bytes),character=gwserver::storage_hex(owned.character.bytes);
 Query owner(db,bank?"SELECT 1 FROM fw_accounts WHERE id=?":"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");
 owner.text(1,account);if(!bank)owner.text(2,character);if(!owner.row())return {FW_NOT_FOUND,"owned inventory not found",{}};
 Query current(db,bank?"SELECT revision,profile FROM fw_gw_storage WHERE account=?":"SELECT revision,profile FROM fw_gw_inventory WHERE character=?");current.text(1,bank?account:character);
 if(!current.row()){allowed(permitted);return {FW_OK,{},{}};}
 const auto revision=current.number(0);if(revision<=0)throw std::invalid_argument("invalid inventory revision");
 auto profile=gw::inventory::decode(current.blob(1));
 if(bank)gw::storage::validate(profile);else gw::inventory::validate_character(profile);
 allowed(permitted);return {FW_OK,{},gwserver::VersionedProfile<gw::inventory::Profile>{static_cast<std::uint64_t>(revision),std::move(profile)}};
}

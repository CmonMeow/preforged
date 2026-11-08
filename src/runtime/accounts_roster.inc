gwserver::RosterResult read_roster(sqlite3* db,const gwserver::CharacterRoster& request,const std::function<bool()>& permitted){
 allowed(permitted);
 if(request.offset>8||request.offset%3)return {FW_INVALID,"invalid roster page offset"};
 Transaction snapshot(db);
 const auto account=gwserver::storage_hex(request.account.bytes);
 Query owner(db,"SELECT 1 FROM fw_accounts WHERE id=?");owner.text(1,account);
 if(!owner.row())return {FW_NOT_FOUND,"account missing"};
 Query total(db,"SELECT count(*) FROM fw_gw_characters WHERE account=?");total.text(1,account);total.row();
 const auto count=total.number(0);
 if(count<0||count>8)throw SqlError("invalid persistent character roster");
 gwserver::RosterResult result{FW_OK,{},static_cast<unsigned>(count),{}};
 Query page(db,"SELECT id,name,map_id,summary FROM fw_gw_characters WHERE account=? ORDER BY roster_order,id LIMIT 3 OFFSET ?");
 page.text(1,account);page.number(2,request.offset);
 while(page.row()){
  allowed(permitted);
  const auto map=page.number(2);
  if(map<1||map>65535)throw SqlError("invalid stored character map");
  gwserver::RosterEntry entry{gwserver::Identity::from_hex(page.text(0)),page.text(1),static_cast<std::uint16_t>(map),{}};
  (void)gw::character_name_to_utf16(entry.name,19);
  const auto blob=page.blob(3);
  if(!blob.empty()){
   auto profile=gw::CharacterProfile::decode(blob);
   if(profile.last_map_id!=map)throw SqlError("stored character profile map mismatch");
   Query inventory(db,"SELECT profile FROM fw_gw_inventory WHERE character=?");inventory.text(1,page.text(0));
   if(inventory.row())profile=profile.with_equipment_2022(gw::equipment::armor_visuals(gw::inventory::decode(inventory.blob(0))));
   entry.profile=std::move(profile);
  }
  result.entries.push_back(std::move(entry));
 }
 const auto expected=request.offset<result.total?std::min(3u,result.total-request.offset):0u;
 if(result.entries.size()!=expected)throw SqlError("inconsistent roster snapshot");
 allowed(permitted);return result;
}

gwserver::FactionsResult read_factions(sqlite3* db,const gwserver::Identity& owned,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);
 const auto account=gwserver::storage_hex(owned.bytes);
 Query owner(db,"SELECT 1 FROM fw_accounts WHERE id=?");owner.text(1,account);
 if(!owner.row())return {FW_NOT_FOUND,"owned faction account missing",{}};
 Query current(db,"SELECT revision,profile FROM fw_gw_factions WHERE account=?");current.text(1,account);
 if(!current.row()){allowed(permitted);return {FW_OK,{},{}};}
 const auto revision=current.number(0);if(revision<=0)throw SqlError("invalid faction revision");
 auto profile=gw::progression::decode_factions(current.blob(1));allowed(permitted);
 return {FW_OK,{},gwserver::VersionedProfile<gw::progression::Factions>{static_cast<std::uint64_t>(revision),std::move(profile)}};
}
gwserver::RevisionResult write_factions(sqlite3* db,const gwserver::SaveFactions& request,const std::function<bool()>& permitted){
 allowed(permitted);gwserver::check_revision(request.expected_revision);
 const auto bytes=gw::progression::encode(request.profile);
 const auto account=gwserver::storage_hex(request.account.bytes);
 Transaction tx(db);Query owner(db,"SELECT 1 FROM fw_accounts WHERE id=?");owner.text(1,account);
 if(!owner.row())return {FW_NOT_FOUND,"owned faction account missing"};
 Query current(db,"SELECT revision FROM fw_gw_factions WHERE account=?");current.text(1,account);
 const auto revision=current.row()?current.number(0):0;
 if(revision<0||static_cast<std::uint64_t>(revision)!=request.expected_revision)return {FW_CONFLICT,"faction revision changed"};
 allowed(permitted);Query write(db,"INSERT INTO fw_gw_factions VALUES(?,?,?) ON CONFLICT(account) DO UPDATE SET revision=excluded.revision,profile=excluded.profile");
 write.text(1,account);write.number(2,revision+1);write.blob(3,bytes);write.row();allowed(permitted);tx.commit();
 return {FW_OK,{},static_cast<std::uint64_t>(revision+1)};
}

gwserver::RevisionResult award_faction(sqlite3* db,const gwserver::AwardFaction& request,const std::function<bool()>& permitted){
 allowed(permitted);gwserver::check_revision(request.expected_revision);
 const auto expected=request.expected_revision;const auto target=static_cast<unsigned>(request.faction);const auto amount=request.amount;
 if(target>3||!amount)return {FW_INVALID,"invalid faction mutation"};
 const auto account=gwserver::storage_hex(request.account.bytes);
  Transaction tx(db);Query owner(db,"SELECT 1 FROM fw_accounts WHERE id=?");owner.text(1,account);
  if(!owner.row())return gwserver::RevisionResult{FW_NOT_FOUND,"faction account missing"};
  Query current(db,"SELECT revision,profile FROM fw_gw_factions WHERE account=?");current.text(1,account);
  const bool present=current.row();const auto revision=present?current.number(0):0;
  if(expected!=revision)return gwserver::RevisionResult{FW_CONFLICT,"faction revision changed"};
  auto profile=present?gw::progression::decode_factions(current.blob(1)):gw::progression::Factions{};
  profile=gw::progression::earn(profile,static_cast<gw::progression::Faction>(target),static_cast<std::uint32_t>(amount));
  allowed(permitted);Query write(db,"INSERT INTO fw_gw_factions VALUES(?,?,?) ON CONFLICT(account) DO UPDATE SET revision=excluded.revision,profile=excluded.profile");
  write.text(1,account);write.number(2,revision+1);write.blob(3,gw::progression::encode(profile));write.row();
  allowed(permitted);tx.commit();return gwserver::RevisionResult{FW_OK,{},static_cast<std::uint64_t>(revision+1)};
}
gwserver::RevisionResult exchange_faction(sqlite3* db,const gwserver::ExchangeImperialFaction& request,const std::function<bool()>& permitted){
 allowed(permitted);gwserver::check_revision(request.expected_revision);
 const auto expected=request.expected_revision;const auto target=static_cast<unsigned>(request.destination);const auto amount=request.amount;
 if(target>2||!amount)return {FW_INVALID,"invalid faction mutation"};
 const auto account=gwserver::storage_hex(request.account.bytes);
  Transaction tx(db);Query owner(db,"SELECT 1 FROM fw_accounts WHERE id=?");owner.text(1,account);
  if(!owner.row())return gwserver::RevisionResult{FW_NOT_FOUND,"faction account missing"};
  Query current(db,"SELECT revision,profile FROM fw_gw_factions WHERE account=?");current.text(1,account);
  const bool present=current.row();const auto revision=present?current.number(0):0;
  if(expected!=revision)return gwserver::RevisionResult{FW_CONFLICT,"faction revision changed"};
  auto profile=present?gw::progression::decode_factions(current.blob(1)):gw::progression::Factions{};
  if(amount>profile.counters[3].current)return gwserver::RevisionResult{FW_CONFLICT,"insufficient Imperial faction"};
  profile=gw::progression::exchange_imperial(profile,static_cast<gw::progression::Faction>(target),static_cast<std::uint32_t>(amount));
  allowed(permitted);Query write(db,"INSERT INTO fw_gw_factions VALUES(?,?,?) ON CONFLICT(account) DO UPDATE SET revision=excluded.revision,profile=excluded.profile");
  write.text(1,account);write.number(2,revision+1);write.blob(3,gw::progression::encode(profile));write.row();
  allowed(permitted);tx.commit();return gwserver::RevisionResult{FW_OK,{},static_cast<std::uint64_t>(revision+1)};
}

// Shared transaction helpers for production typed reads and offline adapters.
gwserver::CapeResult read_guild_cape(sqlite3* db,const gwserver::Identity& identity,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);const auto owner=gwserver::storage_hex(identity.bytes);
 if(!friend_owner(db,owner))return {FW_NOT_FOUND,"social account not found"};
 Query cape(db,"SELECT c.design FROM fw_gw_guild_members m JOIN fw_gw_guild_capes c ON c.guild=m.guild WHERE m.account=?");cape.text(1,owner);
 std::optional<fw::gw::guild::cape::Design> design;
 if(cape.row())try{design=fw::gw::guild::cape::decode(cape.blob(0));}catch(const std::invalid_argument&){throw std::runtime_error("invalid stored cape");}
 allowed(permitted);tx.commit();return {FW_OK,{},design};
}
gwserver::TournamentPointsResult read_tournament_points(sqlite3* db,const gwserver::Identity& identity,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);const auto owner=gwserver::storage_hex(identity.bytes);
 if(!friend_owner(db,owner))return {FW_NOT_FOUND,"social account not found"};
 Query points(db,"SELECT balance FROM fw_gw_tournament_points WHERE account=?");points.text(1,owner);const auto balance=points.row()?points.number(0):0;
 if(balance<0||balance>UINT32_MAX)throw std::runtime_error("invalid stored tournament balance");
 allowed(permitted);tx.commit();return {FW_OK,{},static_cast<std::uint32_t>(balance)};
}
gwserver::GuildFactionResult read_guild_faction(sqlite3* db,const gwserver::Identity& identity,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);const auto owner=gwserver::storage_hex(identity.bytes);
 if(!friend_owner(db,owner))return {FW_NOT_FOUND,"social account not found"};
 Query current(db,"SELECT f.guild,f.allegiance,f.balance,f.decay_day FROM fw_gw_guild_members m JOIN fw_gw_guild_faction f ON f.guild=m.guild WHERE m.account=?");current.text(1,owner);
 gwserver::GuildFaction result{0,0};
 if(current.row()){
  const auto guild=current.text(0);const auto allegiance=current.number(1),balance=current.number(2),old_day=current.number(3);
  if(allegiance<0||allegiance>1||balance<0||balance>UINT32_MAX||old_day<0)throw std::runtime_error("invalid stored guild faction");
  const auto decayed=fw::gw::guild::decay_faction(static_cast<std::uint32_t>(balance),old_day,fw::gw::guild::faction_day(unix_time()));
  result={static_cast<unsigned>(allegiance),decayed.balance};
  if(decayed.day!=old_day){
   Query update(db,"UPDATE fw_gw_guild_faction SET balance=?,decay_day=? WHERE guild=?");update.number(1,decayed.balance);update.number(2,decayed.day);update.text(3,guild);allowed(permitted);update.row();
  }
 }
 allowed(permitted);tx.commit();return {FW_OK,{},result};
}

gwserver::GuildResult read_guild(sqlite3* db,const gwserver::Identity& identity,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);const auto owner=gwserver::storage_hex(identity.bytes);
 if(!friend_owner(db,owner))return {FW_NOT_FOUND,"social account not found"};
 Query guild(db,"SELECT g.id,g.name,g.tag,g.announcement,g.author,g.hall FROM fw_gw_guild_members m JOIN fw_gw_guilds g ON g.id=m.guild WHERE m.account=?");guild.text(1,owner);
 if(!guild.row()){allowed(permitted);tx.commit();return {FW_OK,{},{}};}
 try {
  const auto hall=guild.number(5);if(hall<0||hall>65535)throw std::invalid_argument("guild hall range");
  gwserver::GuildState state{gwserver::Identity::from_hex(guild.text(0)),guild.text(1),guild.text(2),guild.text(3),guild.text(4),static_cast<std::uint16_t>(hall),{}};
  (void)fw::gw::character_name_to_utf16(state.name,31);(void)fw::gw::character_name_to_utf16(state.tag,4);
  if(!state.announcement.empty())(void)fw::gw::character_name_to_utf16(state.announcement,255);
  if(!state.author.empty())(void)fw::gw::character_name_to_utf16(state.author,19);
  Query members(db,"SELECT account,alias,rank FROM fw_gw_guild_members WHERE guild=? ORDER BY rank,alias COLLATE NOCASE,account");members.text(1,gwserver::storage_hex(state.id.bytes));
  std::set<std::string> seen;bool owned=false;unsigned leaders=0;
  while(members.row()){
   allowed(permitted);const auto account=members.text(0),alias=members.text(1);const auto rank=members.number(2);
   if(rank<0||rank>2||state.members.size()>=100||!seen.insert(account).second)throw std::invalid_argument("guild roster bounds");
   (void)fw::gw::character_name_to_utf16(alias,19);auto id=gwserver::Identity::from_hex(account);owned|=id.bytes==identity.bytes;leaders+=rank==0;
   state.members.push_back({id,alias,static_cast<unsigned>(rank)});
  }
  if(!owned||leaders!=1)throw std::invalid_argument("guild ownership or leader missing");
  allowed(permitted);tx.commit();return {FW_OK,{},std::move(state)};
 }catch(const std::invalid_argument&){throw std::runtime_error("invalid stored guild snapshot");}
}

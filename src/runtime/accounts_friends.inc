// Shared native friend storage. Offline adapters serialize only at their boundary.
bool friend_owner(sqlite3* db,const std::string& owner){
 Query account(db,"SELECT 1 FROM fw_accounts WHERE id=?");account.text(1,owner);return account.row();
}
gwserver::FriendsResult read_friends(sqlite3* db,const gwserver::Identity& identity,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);const auto owner=gwserver::storage_hex(identity.bytes);
 if(!friend_owner(db,owner))return {FW_NOT_FOUND,"social account not found",{}};
 Query rows(db,"SELECT f.target,f.type,f.alias,COALESCE((SELECT c.name FROM fw_gw_characters c WHERE c.account=f.target ORDER BY c.roster_order,c.id LIMIT 1),'') FROM fw_gw_friends f WHERE f.owner=? ORDER BY f.type,f.alias COLLATE NOCASE,f.target");rows.text(1,owner);
 gwserver::FriendsResult result{FW_OK,{},{}};unsigned friends=0,ignored=0;std::set<std::string> seen;
 while(rows.row()){
  allowed(permitted);const auto target=rows.text(0),alias=rows.text(2),character=rows.text(3);const auto type=rows.number(1);
  if(target==owner||!seen.insert(target).second||(type!=1&&type!=2)||(type==1&&++friends>100)||(type==2&&++ignored>10))throw std::runtime_error("invalid stored friend list");
  try {
   (void)fw::gw::character_name_to_utf16(alias,19);if(!character.empty())(void)fw::gw::character_name_to_utf16(character,19);
   result.entries.push_back({gwserver::Identity::from_hex(target),static_cast<unsigned>(type),alias,character});
  }catch(const std::invalid_argument&){throw std::runtime_error("invalid stored friend fields");}
 }
 allowed(permitted);tx.commit();return result;
}
gwserver::FriendAddResult add_friend(sqlite3* db,const gwserver::AddFriend& request,const std::function<bool()>& permitted){
 allowed(permitted);
 if(request.type!=1&&request.type!=2)return {FW_INVALID,"invalid friend list type"};
 (void)fw::gw::character_name_to_utf16(request.name,19);const auto alias=request.alias.empty()?request.name:request.alias;
 (void)fw::gw::character_name_to_utf16(alias,19);
 Transaction tx(db);const auto owner=gwserver::storage_hex(request.owner.bytes);
 if(!friend_owner(db,owner))return {FW_NOT_FOUND,"social account not found"};
 const auto target=social_target(db,request.name);if(!target)return {FW_NOT_FOUND,"character name absent or ambiguous"};
 if(*target==owner)return {FW_INVALID,"cannot add yourself"};
 Query count(db,"SELECT count(*) FROM fw_gw_friends WHERE owner=? AND type=? AND target<>?");count.text(1,owner);count.number(2,request.type);count.text(3,*target);count.row();
 if(count.number(0)>=(request.type==1?100:10))return {FW_CONFLICT,"friend or ignore list full"};
 // Native FriendApi keys presence and removals by alias.
 Query duplicate(db,"SELECT 1 FROM fw_gw_friends WHERE owner=? AND target<>? AND alias=? COLLATE NOCASE");duplicate.text(1,owner);duplicate.text(2,*target);duplicate.text(3,alias);
 if(duplicate.row())return {FW_CONFLICT,"friend alias already in use"};
 const auto identity=gwserver::Identity::from_hex(*target);
 Query write(db,"INSERT INTO fw_gw_friends(owner,target,type,alias) VALUES(?,?,?,?) ON CONFLICT(owner,target) DO UPDATE SET type=excluded.type,alias=excluded.alias");
 write.text(1,owner);write.text(2,*target);write.number(3,request.type);write.text(4,alias);
 allowed(permitted);write.row();allowed(permitted);tx.commit();return {FW_OK,{},gwserver::FriendAddition{identity,alias}};
}
gwserver::FriendRemoveResult remove_friend(sqlite3* db,const gwserver::FriendTarget& request,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);const auto owner=gwserver::storage_hex(request.owner.bytes);
 if(!friend_owner(db,owner))return {FW_NOT_FOUND,"social account not found"};
 Query remove(db,"DELETE FROM fw_gw_friends WHERE owner=? AND target=?");remove.text(1,owner);remove.text(2,gwserver::storage_hex(request.target.bytes));
 allowed(permitted);remove.row();allowed(permitted);tx.commit();return {FW_OK,{}};
}
gwserver::FriendBlockedResult friend_blocked(sqlite3* db,const gwserver::FriendTarget& request,const std::function<bool()>& permitted){
 allowed(permitted);Transaction tx(db);const auto owner=gwserver::storage_hex(request.owner.bytes);
 if(!friend_owner(db,owner))return {FW_NOT_FOUND,"social account not found"};
 Query ignored(db,"SELECT 1 FROM fw_gw_friends WHERE owner=? AND target=? AND type=2");ignored.text(1,owner);ignored.text(2,gwserver::storage_hex(request.target.bytes));
 const bool blocked=ignored.row();allowed(permitted);tx.commit();return {FW_OK,{},blocked};
}

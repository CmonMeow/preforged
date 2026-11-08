// Shared native purchase transactions. Protocol/offline encoding stays at the boundary.
gwserver::CapePurchaseResult buy_guild_cape(sqlite3* db,const gwserver::PurchaseGuildCape& request,const std::function<bool()>& permitted){
 allowed(permitted);gwserver::check_revision(request.inventory_revision);
 if(request.quoted_price!=fw::gw::guild::cape::price)return {FW_INVALID,"cape quote mismatch"};
 const auto packed=fw::gw::guild::cape::encode(request.design);Transaction tx(db);
 const auto owner=gwserver::storage_hex(request.owner.account.bytes),character=gwserver::storage_hex(request.owner.character.bytes);
 if(!friend_owner(db,owner))return {FW_NOT_FOUND,"social account not found"};
 Query guild(db,"SELECT m.guild,m.rank FROM fw_gw_guild_members m JOIN fw_gw_characters c ON c.account=m.account WHERE m.account=? AND c.id=?");guild.text(1,owner);guild.text(2,character);
 if(!guild.row()||guild.number(1)!=0)return {FW_CONFLICT,"Only the guild leader can purchase a cape."};const auto guild_id=guild.text(0);
 Query old(db,"SELECT design FROM fw_gw_guild_capes WHERE guild=?");old.text(1,guild_id);const auto trim=old.row()?fw::gw::guild::cape::decode(old.blob(0)).trim:0;
 if(request.design.trim!=trim)return {FW_CONFLICT,"Cape editor cannot change earned guild trim."};
 Query inventory(db,"SELECT revision,profile FROM fw_gw_inventory WHERE character=?");inventory.text(1,character);
 if(!inventory.row()||inventory.number(0)!=static_cast<std::int64_t>(request.inventory_revision))return {FW_CONFLICT,"Inventory changed; reopen the cape editor."};
 if(inventory.number(0)<=0||inventory.number(0)>=INT64_MAX-1)throw std::runtime_error("invalid stored inventory revision");
 auto profile=fw::gw::inventory::decode(inventory.blob(1));const auto revision=request.inventory_revision+1;
 if(profile.gold<fw::gw::guild::cape::price)return {FW_CONFLICT,"A cape costs 2,000 carried gold."};profile.gold-=fw::gw::guild::cape::price;
 Query write(db,"INSERT INTO fw_gw_guild_capes VALUES(?,?) ON CONFLICT(guild) DO UPDATE SET design=excluded.design");write.text(1,guild_id);write.blob(2,packed);allowed(permitted);write.row();
 Query balance(db,"UPDATE fw_gw_inventory SET revision=?,profile=? WHERE character=?");balance.number(1,revision);balance.blob(2,fw::gw::inventory::encode(profile));balance.text(3,character);allowed(permitted);balance.row();allowed(permitted);tx.commit();
 return {FW_OK,{},revision,profile.gold};
}
gwserver::TournamentPointsResult buy_guild_trim(sqlite3* db,const gwserver::PurchaseGuildTrim& request,const std::function<bool()>& permitted){
 allowed(permitted);const auto* offer=fw::gw::guild::trim::find(request.trim);if(!offer)return {FW_INVALID,"trim not sold in this era"};
 const auto expected=fw::gw::guild::cape::encode(request.expected_cape);Transaction tx(db);
 const auto owner=gwserver::storage_hex(request.owner.account.bytes),character=gwserver::storage_hex(request.owner.character.bytes);
 if(!friend_owner(db,owner))return {FW_NOT_FOUND,"social account not found"};
 Query guild(db,"SELECT m.guild,m.rank FROM fw_gw_guild_members m JOIN fw_gw_characters c ON c.account=m.account WHERE m.account=? AND c.id=?");guild.text(1,owner);guild.text(2,character);
 if(!guild.row()||guild.number(1)!=0)return {FW_CONFLICT,"Only the guild leader can purchase trim."};const auto guild_id=guild.text(0);
 Query old(db,"SELECT design FROM fw_gw_guild_capes WHERE guild=?");old.text(1,guild_id);
 if(!old.row())return {FW_CONFLICT,"Purchase a guild cape before choosing its trim."};const auto packed=old.blob(0);
 if(packed.size()!=expected.size()||!std::equal(packed.begin(),packed.end(),expected.begin()))return {FW_CONFLICT,"The cape changed; speak to Tolkano again."};
 auto design=fw::gw::guild::cape::decode(packed);if(design.trim==offer->id)return {FW_CONFLICT,"Your guild already has this trim."};
 Query points(db,"SELECT balance FROM fw_gw_tournament_points WHERE account=?");points.text(1,owner);
 if(!points.row()||points.number(0)<offer->price)return {FW_CONFLICT,"Insufficient Tournament Reward Points."};
 if(points.number(0)>UINT32_MAX)throw std::runtime_error("invalid stored tournament balance");const auto remaining=static_cast<std::uint32_t>(points.number(0)-offer->price);
 design.trim=offer->id;Query cape(db,"UPDATE fw_gw_guild_capes SET design=? WHERE guild=?");cape.blob(1,fw::gw::guild::cape::encode(design));cape.text(2,guild_id);allowed(permitted);cape.row();
 Query balance(db,"UPDATE fw_gw_tournament_points SET balance=? WHERE account=?");balance.number(1,remaining);balance.text(2,owner);allowed(permitted);balance.row();allowed(permitted);tx.commit();return {FW_OK,{},remaining};
}

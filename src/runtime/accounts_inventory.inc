gwserver::TradeResult write_trade(sqlite3* db,const gwserver::ExchangeTrade& request,const std::function<bool()>& permitted){
    allowed(permitted);
    if(request.left.owner.account.bytes==request.right.owner.account.bytes||request.left.owner.character.bytes==request.right.owner.character.bytes)return {FW_INVALID,"trade requires two distinct owners and characters"};
    gwserver::check_revision(request.left.expected_revision);gwserver::check_revision(request.right.expected_revision);
    const std::array participants{request.left,request.right};
    for(const auto& participant:participants){
        if(participant.offer.gold>100000||participant.offer.items.size()>7)throw std::invalid_argument("trade offer bound");
        std::set<std::uint32_t> seen;
        for(auto item:participant.offer.items)if(!item.item||!item.quantity||!seen.insert(item.item).second)throw std::invalid_argument("duplicate or empty trade offer");
    }
    Transaction tx(db);
    std::array<fw::gw::inventory::Profile,2> profiles;std::array<std::int64_t,2> revisions{};std::array<std::u16string,2> names;
    for(unsigned side=0;side<2;++side){const auto& participant=participants[side];
        const auto account=gwserver::storage_hex(participant.owner.account.bytes),character=gwserver::storage_hex(participant.owner.character.bytes);
        Query owner(db,"SELECT name FROM fw_gw_characters WHERE account=? AND id=?");owner.text(1,account);owner.text(2,character);if(!owner.row())return {FW_NOT_FOUND,"owned trade inventory not found"};
        names[side]=fw::gw::character_name_to_utf16(owner.text(0),19);
        Query current(db,"SELECT revision,profile FROM fw_gw_inventory WHERE character=?");current.text(1,character);
        if(current.row()){revisions[side]=current.number(0);if(revisions[side]<=0||revisions[side]>=INT64_MAX)throw std::invalid_argument("invalid trade inventory revision");profiles[side]=fw::gw::inventory::decode(current.blob(1));}else profiles[side]=fw::gw::inventory::baseline();
        if(participant.expected_revision!=revisions[side])return {FW_CONFLICT,"trade inventory changed in another session"};
    }
    const auto exchanged=fw::gw::trade::exchange::plan(profiles[0],request.left.offer,profiles[1],request.right.offer,names[0],names[1]);
    const std::array packed{fw::gw::inventory::encode(exchanged.left),fw::gw::inventory::encode(exchanged.right)};
    allowed(permitted);
    for(unsigned side=0;side<2;++side){Query write(db,"INSERT INTO fw_gw_inventory VALUES(?,?,?) ON CONFLICT(character) DO UPDATE SET revision=excluded.revision,profile=excluded.profile");
        write.text(1,gwserver::storage_hex(participants[side].owner.character.bytes));write.number(2,revisions[side]+1);write.blob(3,packed[side]);write.row();}
    allowed(permitted);tx.commit();return {FW_OK,{},gwserver::VersionedProfile<fw::gw::inventory::Profile>{static_cast<std::uint64_t>(revisions[0]+1),exchanged.left},gwserver::VersionedProfile<fw::gw::inventory::Profile>{static_cast<std::uint64_t>(revisions[1]+1),exchanged.right}};
}
AccountResult trade_exchange_execute(sqlite3* db,const std::vector<std::string>& f,const std::function<bool()>& permitted){
 if(f.size()!=10||!social_id(f[0])||!social_id(f[1])||!social_id(f[5])||!social_id(f[6]))return {FW_INVALID,"trade request fields"};
 const gwserver::ExchangeTrade request{{{gwserver::Identity::from_hex(f[0]),gwserver::Identity::from_hex(f[1])},number(f[2],INT64_MAX-1),fw::gw::trade::exchange::parse(f[3],f[4])},{{gwserver::Identity::from_hex(f[5]),gwserver::Identity::from_hex(f[6])},number(f[7],INT64_MAX-1),fw::gw::trade::exchange::parse(f[8],f[9])}};
 const auto result=write_trade(db,request,permitted);
 if(result.status!=FW_OK)return {result.status,result.message};
 return {FW_OK,std::to_string(result.left->revision)+"\t"+std::to_string(result.right->revision)+"\t"+hex_bytes(fw::gw::inventory::encode(result.left->profile))+"\t"+hex_bytes(fw::gw::inventory::encode(result.right->profile))};
}
std::optional<AccountResult> inventory_execute(sqlite3* db,gwserver::Operation operation,const std::vector<std::string>& f,const std::function<bool()>& permitted){
    if(operation==gwserver::Operation::exchangeTrade)return trade_exchange_execute(db,f,permitted);
    const bool get=operation==gwserver::Operation::characterInventory,save=operation==gwserver::Operation::saveInventory,bankget=operation==gwserver::Operation::accountStorage,transfer=operation==gwserver::Operation::transferStorage;
    if(!get&&!save&&!bankget&&!transfer)return {};
    if(f.size()!=(get?2u:save?4u:bankget?1u:6u)||!social_id(f[0])||(!bankget&&!social_id(f[1])))return AccountResult{FW_INVALID,"inventory request fields"};
    if(get||bankget){const auto result=read_inventory(db,{gwserver::Identity::from_hex(f[0]),bankget?gwserver::Identity{}:gwserver::Identity::from_hex(f[1])},bankget,permitted);
        if(result.status!=FW_OK)return AccountResult{result.status,result.message};
        return AccountResult{FW_OK,result.record?std::to_string(result.record->revision)+"\t"+hex_bytes(gw::inventory::encode(result.record->profile)):"0"};}
    const gwserver::OwnedCharacter owned{gwserver::Identity::from_hex(f[0]),gwserver::Identity::from_hex(f[1])};
    const auto expected=number(f[2],INT64_MAX-1);
    const auto decode=[](const std::string& value){return gw::inventory::decode(gwserver::profile_bytes(value,gw::inventory::maximum_blob));};
    const auto profile=decode(f[save?3:4]);
    std::optional<gw::inventory::Profile> bank;if(transfer)bank=decode(f[5]);
    const auto result=write_inventory(db,owned,expected,profile,bank?&*bank:nullptr,transfer?number(f[3],INT64_MAX-1):0,permitted);
    return AccountResult{result.status,result.status==FW_OK?std::to_string(result.inventory_revision)+(bank?"\t"+std::to_string(result.storage_revision):""):result.message};
}


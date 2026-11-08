// Typed guild mutation shared by native runtime and offline adapter.
gwserver::GuildCommandResult mutate_guild(sqlite3* db,const gwserver::GuildCommand& request,const std::function<bool()>& permitted){
 allowed(permitted);const auto action_index=static_cast<unsigned>(request.action);
 if(action_index>=std::size(gwserver::guild_actions))return {FW_INVALID,"unknown guild command"};
 if(!request.argument.empty())fw::gw::character_name_to_utf16(request.argument,255);
 Transaction tx(db);const auto owner=gwserver::storage_hex(request.owner.account.bytes),character_id=gwserver::storage_hex(request.owner.character.bytes);
        Query character(db,"SELECT name FROM fw_gw_characters WHERE account=? AND id=?");character.text(1,owner);character.text(2,character_id);if(!character.row())return gwserver::GuildCommandResult{FW_NOT_FOUND,"owned guild character not found"};
        const auto own_name=character.text(0);const auto action=gwserver::guild_actions[action_index];const auto& argument=request.argument;
        Query member(db,"SELECT guild,rank FROM fw_gw_guild_members WHERE account=?");member.text(1,owner);const bool in_guild=member.row();const auto guild=in_guild?member.text(0):std::string{};const auto rank=in_guild?member.number(1):3;
        if(action=="create"){
            if(in_guild)return gwserver::GuildCommandResult{FW_CONFLICT,"already in a guild"};const auto divider=argument.find('|');if(divider==argument.npos)return gwserver::GuildCommandResult{FW_INVALID,"use /guild create Name | TAG"};
            auto trim=[](std::string s){while(!s.empty()&&s.front()==' ')s.erase(s.begin());while(!s.empty()&&s.back()==' ')s.pop_back();return s;};
            const auto name=trim(argument.substr(0,divider)),tag=trim(argument.substr(divider+1));const auto wide=fw::gw::character_name_to_utf16(name,31);
            if(wide.size()<3||tag.empty()||!guild_tag_letters(tag))return gwserver::GuildCommandResult{FW_INVALID,"guild name3..31 characters; tag1..4 letters"};
            Query exists(db,"SELECT 1 FROM fw_gw_guilds WHERE name=? COLLATE NOCASE OR tag=? COLLATE NOCASE");exists.text(1,name);exists.text(2,tag);if(exists.row())return gwserver::GuildCommandResult{FW_CONFLICT,"guild name or tag in use"};
            const auto id=random_hex(16);Query create(db,"INSERT INTO fw_gw_guilds(id,name,tag) VALUES(?,?,?)");create.text(1,id);create.text(2,name);create.text(3,tag);allowed(permitted);create.row();
            Query join(db,"INSERT INTO fw_gw_guild_members VALUES(?,?,?,0)");join.text(1,owner);join.text(2,id);join.text(3,own_name);allowed(permitted);join.row();allowed(permitted);tx.commit();return gwserver::GuildCommandResult{FW_OK,"Guild created on the private server."};
        }
        if(action=="join"){
            if(in_guild)return gwserver::GuildCommandResult{FW_CONFLICT,"already in a guild"};Query invite(db,"SELECT guild FROM fw_gw_guild_invites WHERE target=? AND expires>?");invite.text(1,owner);invite.number(2,unix_time());if(!invite.row())return gwserver::GuildCommandResult{FW_NOT_FOUND,"no active guild invitation"};
            Query count(db,"SELECT count(*) FROM fw_gw_guild_members WHERE guild=?");count.text(1,invite.text(0));count.row();if(count.number(0)>=100)return gwserver::GuildCommandResult{FW_CONFLICT,"guild is full"};
            Query join(db,"INSERT INTO fw_gw_guild_members VALUES(?,?,?,2)");join.text(1,owner);join.text(2,invite.text(0));join.text(3,own_name);allowed(permitted);join.row();Query remove(db,"DELETE FROM fw_gw_guild_invites WHERE target=?");remove.text(1,owner);allowed(permitted);remove.row();allowed(permitted);tx.commit();return gwserver::GuildCommandResult{FW_OK,"Guild invitation accepted."};
        }
        if(!in_guild)return gwserver::GuildCommandResult{FW_NOT_FOUND,"you are not in a guild"};
        if(action=="leave"){
            Query count(db,"SELECT count(*) FROM fw_gw_guild_members WHERE guild=?");count.text(1,guild);count.row();if(rank==0&&count.number(0)>1)return gwserver::GuildCommandResult{FW_CONFLICT,"transfer leadership before leaving"};
            Query remove(db,"DELETE FROM fw_gw_guild_members WHERE account=?");remove.text(1,owner);allowed(permitted);remove.row();if(rank==0){Query erase(db,"DELETE FROM fw_gw_guilds WHERE id=?");erase.text(1,guild);allowed(permitted);erase.row();}allowed(permitted);tx.commit();return gwserver::GuildCommandResult{FW_OK,"You left the guild."};
        }
        if(action=="motd"){
            if(rank>1)return gwserver::GuildCommandResult{FW_CONFLICT,"only leader or officers can edit announcement"};Query write(db,"UPDATE fw_gw_guilds SET announcement=?,author=? WHERE id=?");write.text(1,argument);write.text(2,own_name);write.text(3,guild);allowed(permitted);write.row();allowed(permitted);tx.commit();return gwserver::GuildCommandResult{FW_OK,"Guild announcement saved."};
        }
        if(action=="invite"||action=="kick"||action=="promote"||action=="demote"||action=="leader"){
            if(rank>1||(action!="invite"&&action!="kick"&&rank!=0))return gwserver::GuildCommandResult{FW_CONFLICT,"guild permission denied"};
            const auto target=social_target(db,argument);if(!target||*target==owner)return gwserver::GuildCommandResult{FW_NOT_FOUND,"other character absent or ambiguous"};Query other(db,"SELECT guild,rank FROM fw_gw_guild_members WHERE account=?");other.text(1,*target);const bool joined=other.row();
            if(action=="invite"){
                if(joined)return gwserver::GuildCommandResult{FW_CONFLICT,"player is already in a guild"};Query invite(db,"INSERT INTO fw_gw_guild_invites VALUES(?,?,?,?) ON CONFLICT(target) DO UPDATE SET guild=excluded.guild,inviter=excluded.inviter,expires=excluded.expires");invite.text(1,*target);invite.text(2,guild);invite.text(3,owner);invite.number(4,unix_time()+7*86400);allowed(permitted);invite.row();allowed(permitted);tx.commit();return gwserver::GuildCommandResult{FW_OK,"Guild invitation saved; recipient can /guild join."};
            }
            if(!joined||other.text(0)!=guild||other.number(1)<=rank)return gwserver::GuildCommandResult{FW_CONFLICT,"target is not a lower-ranked member of your guild"};
            if(action=="kick"){Query remove(db,"DELETE FROM fw_gw_guild_members WHERE account=? AND guild=?");remove.text(1,*target);remove.text(2,guild);allowed(permitted);remove.row();}
            else {if(action=="leader"){Query lower(db,"UPDATE fw_gw_guild_members SET rank=1 WHERE account=?");lower.text(1,owner);allowed(permitted);lower.row();}Query write(db,"UPDATE fw_gw_guild_members SET rank=? WHERE account=? AND guild=?");write.number(1,action=="leader"?0:action=="promote"?1:2);write.text(2,*target);write.text(3,guild);allowed(permitted);write.row();}
            allowed(permitted);tx.commit();return gwserver::GuildCommandResult{FW_OK,"Guild membership updated."};
        }
        return gwserver::GuildCommandResult{FW_INVALID,"unknown guild command"};

}


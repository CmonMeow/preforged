#include "gw_social_wire.h"
#include "gw_social_presence.h"
#include <fstream>
#include <iostream>
#include <map>
using namespace fw::gw::social;
void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
Packet unpack(std::string text){Packet out;for(size_t i=0;i<text.size();i+=2)out.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(i,2),nullptr,16)));return out;}
int main(int argc,char** argv)try{
    {
        Directory directory;const std::string account="presence-fixture";
        auto old_auth=std::make_shared<fw::gw::world::Outbox>(),new_auth=std::make_shared<fw::gw::world::Outbox>();
        auto old_world=std::make_shared<fw::gw::world::Outbox>(),new_world=std::make_shared<fw::gw::world::Outbox>();
        const auto old_lease=directory.login(account,u"Old Character",old_auth);
        check(directory.character(account,old_lease,u"Old Character",1,old_world),"current world presence lease rejected");
        const auto new_lease=directory.login(account,u"New Character",new_auth);
        check(new_lease!=old_lease&&!directory.character(account,old_lease,u"Old Character",2,old_world),
            "old world socket overwrote replacement login presence");
        check(!directory.character(account,0,u"Unowned Character",2,old_world),"unowned world update accepted");
        auto current=directory.get(account);
        check(current&&current->character==u"New Character"&&current->map==0&&!current->world_outbox&&current->auth_outbox==new_auth,
            "stale world update changed replacement character or transport");
        check(directory.character(account,new_lease,u"New Character",3,new_world),"replacement world update rejected");
        directory.status(account,old_lease,0);directory.logout(account,old_lease);directory.leave_world(account,old_world);
        const auto destination=directory.find(u"new character");
        check(destination&&destination->second.world_outbox==new_world&&destination->second.map==3&&destination->second.status==1,
            "stale session changed replacement whisper routing or status");
        check(directory.character(account,new_lease,u"New Character",4,new_world),"map transfer lost auth presence lease");
        directory.leave_world(account,new_world);check(!directory.get(account)->world_outbox,"current world disconnect did not detach transport");
        directory.logout(account,new_lease);check(!directory.get(account),"current auth logout did not clear presence");
    }
    check(argc==2,"fixture path");std::ifstream file(argv[1]);std::map<std::string,Packet> fixture;std::string key,hex;
    while(file>>key>>hex)fixture[key]=unpack(hex);
    Friend member;for(unsigned i=0;i<16;++i)member.account[i]=static_cast<std::uint8_t>(i+1);member.alias=u"Friend Alias";
    auto equal=[&](Packet packet,const char* label){packet[1]|=0x80;check(packet==fixture.at(label),label);};
    for(unsigned status=0;status<4;++status){
        member.status=status;member.character=u"Other Character";
        const char* labels[]{"presence_offline","presence_online","presence_dnd","presence_away"};
        equal(friend_presence(member),labels[status]);
        check(friend_record(member)==friend_presence(member)&&friend_record(member,true)==friend_presence(member),"legacy caller emitted a cached record as presence");
    }
    member.status=0;equal(friend_added(42,member),"added");equal(friend_removed(43,member),"removed");
    equal(whisper(u"Other Character",u"Private message"),"whisper");
    check(whisper(std::u16string(19,u'A'),std::u16string(255,u'B')).size()==6+2+19*2+2+255*2,"native whisper maximum was truncated");
    for(const auto& operation:{0u,1u,2u,3u}){
        bool rejected=false;try{
            if(operation==0)whisper(std::u16string(20,u'A'),u"message");
            else if(operation==1)whisper(u"Name",std::u16string(256,u'B'));
            else if(operation==2)whisper(u"",u"message");else whisper(u"Name",u"");
        }catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"invalid native whisper bound accepted");
    }
    for(auto label:{"add_request","remove_request"}){
        const auto& packet=fixture.at(label);Request value;
        for(size_t n=0;n<packet.size();++n)check(parse(std::span(packet).first(n),value)==Request::Result::incomplete,"fragmented social request");
        check(parse(packet,value)==Request::Result::complete&&value.consumed==packet.size(),"request decode");
        check(value.transaction==unsigned(label[0]=='a'?42:43),"request transaction");
        if(value.opcode==26)check(value.name==u"Other Character"&&value.alias==member.alias&&value.type==1,"add fields");
        else check(value.account==member.account&&value.name==member.alias,"remove fields");
    }
    auto invalid=fixture.at("add_request");invalid[6]=3;Request request;check(parse(invalid,request)==Request::Result::invalid,"reject list type");
    invalid=fixture.at("remove_request");invalid[22]=20;check(parse(invalid,request)==Request::Result::invalid,"reject oversized name");
    const auto offline=friend_presence(member);member.character=u"Private Character";
    check(friend_presence(member)==offline,"offline presence disclosed playing character");
    member.status=4;try{friend_presence(member);throw std::runtime_error("invalid status accepted");}catch(const std::invalid_argument&){}
    member.status=1;member.type=3;try{friend_presence(member);throw std::runtime_error("unsupported persisted friend category accepted");}catch(const std::invalid_argument&){}
    member.type=1;member.alias.clear();try{friend_presence(member);throw std::runtime_error("missing existing-friend lookup key accepted");}catch(const std::invalid_argument&){}
    std::cout<<"Social wire: pinned native presence/added bytes, fragments, identities, offline privacy and bounds PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

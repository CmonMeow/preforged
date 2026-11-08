#include "gw_guild_cape.h"
#include "gw_guild_wire.h"
#include "gw_guild_announcement_delivery.h"
#include <fstream>
#include <iostream>
#include <map>
int main(int argc,char** argv)try{
 using namespace fw::gw;auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
 {const auto packets=guild::cape::open(10031);check(packets==guild::Batch{{195,0,47,39,0,0},{194,0,9,208,7,0,0}},"native cape vendor opening layout");
 guild::Packet request{54,128,208,7,0,0,240,177,101,3,10,35,0,0};
 check(world::outgoing_frame_2022(request).size==14,"retail cape purchase width");
 const auto parsed=guild::cape::parse(request);check(parsed.quoted_price==2000&&parsed.design.emblem==35&&parsed.design.shape==3,"native cape purchase field order");
 for(unsigned n=0;n<request.size();++n)check(world::outgoing_frame_2022(std::span(request).first(n)).state==world::FrameState::incomplete,"fragmented cape frame treated as complete");
 bool invalid=false;request[9]=9;try{guild::cape::parse(request);}catch(const std::invalid_argument&){invalid=true;}check(invalid,"invalid cape shape accepted");}


 check(guild::affiliation(50,1)==guild::Packet{158,0,30,0,0,0,50,0,0,0,1,0,0,0},"native player guild affiliation");
 check(guild::affiliation(50,0)==guild::Packet{158,0,30,0,0,0,50,0,0,0,0,0,0,0},"leave guild removes actor affiliation");
 {guild::General g;g.local_id=1;g.name=u"Appearance Test";
 check(guild::appearance_update(g,50,false)==guild::Batch{guild::general(g),guild::affiliation(50,1)},"initial affiliation follows its guild record");
 g.trim=3;
 check(guild::appearance_update(g,50,true)==guild::Batch{guild::general(g),guild::affiliation(50,0),guild::affiliation(50,1)},"changed appearance transitions affiliation to trigger the native model rebuild");}
 check(guild::native_rank(0)==5&&guild::native_rank(1)==4&&guild::native_rank(2)==3,"private ranks map to native privilege ordering");
 if(argc!=2)throw std::runtime_error("guild fixture path required");std::ifstream input(argv[1]);std::map<std::string,std::string> fixtures;std::string label,hex;while(input>>label>>hex)fixtures.emplace(label,hex);check(fixtures.size()==7,"all original retail fixtures loaded");
 auto wire=[](guild::Packet p){p[1]|=0x80;std::string s;const char* h="0123456789abcdef";for(auto b:p){s+=h[b>>4];s+=h[b&15];}return s;};
 guild::General g;g.local_id=1;g.key={1,2,3,4};g.name=u"Private Guild";g.tag=u"TEST";g.background=1;g.detail_color=2;g.emblem_color=3;g.shape=4;g.detail=5;g.emblem=6;g.trim=7;
 check(wire(guild::general(g))==fixtures.at("general"),"native general equals retail serializer");check(wire(guild::role(1,g.key,1))==fixtures.at("role"),"native role equals retail serializer");
 check(wire(guild::clear_membership())==fixtures.at("clear"),"native membership clear equals retail serializer");
 check(wire(guild::complete())==fixtures.at("complete"),"guild UI completion equals retail serializer");
 guild::Member m;m.invited_name=u"Fixture Member";m.current_name=u"Fixture Current";m.inviter=u"Fixture Inviter";m.status=1;m.rank=1;
 check(wire(guild::member(m))==fixtures.at("member"),"native member equals retail serializer");check(wire(guild::remove(m.invited_name))==fixtures.at("remove"),"native removal equals retail serializer");check(wire(guild::announcement(u"Private announcement",m.invited_name))==fixtures.at("announcement"),"native announcement equals retail serializer");
 auto packets=guild::restore(g,1,{m});check(packets.size()==5&&packets[0][0]==32&&packets[1][0]==23&&packets.back()==guild::complete(),"guild general precedes local role and completion follows full snapshot");
 bool rejected=false;g.tag=u"TOOLONG";try{guild::general(g);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"native tag maximum four");
 rejected=false;try{guild::remove(u"");}catch(const std::invalid_argument&){rejected=true;}check(rejected,"empty removal cannot target unspecified roster entry");
 rejected=false;try{guild::announcement(std::u16string(256,u'x'),u"Author");}catch(const std::invalid_argument&){rejected=true;}check(rejected,"announcement native bound");
 guild::AnnouncementDelivery delivery;unsigned sends=0;auto emit=[&](const guild::Packet&){++sends;};
 auto announcement=guild::announcement(u"Welcome",u"Author");
 check(delivery.publish("guild1",announcement,emit),"first authenticated publication required");
 check(!delivery.publish("guild1",announcement,emit)&&sends==1,"map transfer/reconnect repeats suppressed");
 check(delivery.publish("guild1",guild::announcement(u"Changed",u"Author"),emit),"changed announcement published");
 check(delivery.publish("guild2",announcement,emit),"new guild announcement published");
 delivery.clear();check(delivery.publish("guild2",announcement,emit),"membership clear permits new membership announcement");
 guild::AnnouncementDelivery fresh_login;check(fresh_login.publish("guild2",announcement,emit),"new login displays existing announcement");
 guild::AnnouncementDelivery failed;try{failed.publish("guild1",announcement,[](const guild::Packet&){throw std::runtime_error("transport");});}catch(const std::runtime_error&){}
 check(failed.publish("guild1",announcement,emit),"failed send must remain retryable");
 std::cout<<"Native guild wire fixtures and bounds passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

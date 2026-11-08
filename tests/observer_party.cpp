#include "gw_observer_party.h"
#include <array>
#include <iostream>
using namespace fw::gw::observer::party_ui;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main()try{
 check(activate_hud()==fw::gw::party::Packet{212,1},"native observer HUD activation descriptor");
 fw::gw::guild::General guild;guild.name=u"Guild";guild.tag=u"TAG";guild.background=1;guild.detail_color=2;guild.emblem_color=3;guild.shape=4;guild.detail=5;guild.emblem=0x123;guild.trim=6;guild.rank=7;guild.rating=1000;
 check(mission_guild(2,guild)==fw::gw::party::Packet{173,1,2,0,1,2,3,4,5,35,1,6,7,0,0,0,232,3,0,0,5,0,'G',0,'u',0,'i',0,'l',0,'d',0,3,0,'T',0,'A',0,'G',0},"native mission guild fixture");
 const std::array<Team,2> teams{{{1,11,0,1,0,u"Blue",{16,11}},{2,22,0,2,0,u"Red",{3,7}}}};
 auto p=publish(teams);check(p.size()==10,"complete two-team stream");check(p[0]==fw::gw::party::Packet{172,1,1,0,11,0,0,0,0,1,0,4,0,'B',0,'l',0,'u',0,'e',0},"mission team fixture");check(p[2]==fw::gw::party::Packet{202,1,1,0,16,0,1},"captured player number not actor");check(p[4]==fw::gw::party::Packet{210,1,1,0},"roster completion");for(const auto& packet:p)check(!(packet[0]==177&&packet[1]==1),"spectator selected into replay party");
 bool denied=false;auto duplicate=teams;duplicate[1].player_numbers.push_back(16);try{publish(duplicate);}catch(const std::invalid_argument&){denied=true;}check(denied,"cross-team duplicated player accepted");
 denied=false;auto malformed=teams[0];malformed.encoded_name=std::u16string(21,u'a');try{mission_team(malformed);}catch(const std::invalid_argument&){denied=true;}check(denied,"native team name bound");
 std::cout<<"observer party wire tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

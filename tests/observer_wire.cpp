#include "gw_observer_wire.h"
#include "gw_observer_presentation.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::observer::wire;
static void check(bool b,const char* text){if(!b)throw std::runtime_error(text);}
static unsigned get(const Packet& p,unsigned at){return p.at(at)|(unsigned(p.at(at+1))<<8)|(unsigned(p.at(at+2))<<16)|(unsigned(p.at(at+3))<<24);}
int main()try{
 Adapter objects;
 auto bridge=objects.apply({"MAP_OBJECT",{"55597","3","3"}});
  check(bridge.status==Status::applied&&bridge.packets==std::vector<Packet>{{13,1,45,217,3,3,0,0,0}},"retail gate animation fixture");
  auto growth=objects.apply({"MAP_OBJECT",{"51238","16","2"}});
  check(growth.status==Status::applied&&growth.packets==std::vector<Packet>{{13,1,38,200,16,2,0,0,0}},"captured blue vine growth rejected");
  auto red_growth=objects.apply({"MAP_OBJECT",{"39278","16","2"}});
  check(red_growth.status==Status::applied&&red_growth.packets==std::vector<Packet>{{13,1,110,153,16,2,0,0,0}},"captured red vine growth rejected");
  check(objects.apply({"MAP_OBJECT",{"51238","15","2"}}).status==Status::unsupported,"unverified neighboring animation accepted");
 auto state=objects.apply({"MAP_OBJECT_STATE",{"55597","1","1"}});
 check(state.status==Status::applied&&state.packets==std::vector<Packet>{{16,1,45,217,1,0,0,0,1,0,0,0}},"retail map object state fixture");
 check(objects.apply({"MAP_OBJECT",{"65536","3","3"}}).status==Status::invalid,"oversized object ID accepted");
 check(objects.apply({"MAP_OBJECT",{"1","255","0"}}).status==Status::unsupported,"unverified animation index accepted");
 Adapter a;a.seed(52,{35,.8f,0,0});a.seed(54,{0,1,0,0});
 auto r=a.apply({"GAME_SMSG_AGENT_MOVE_TO_POINT",{"52","-3904.53","-4200.09","0"}});
 check(r.status==Status::applied&&r.packets.size()==2,"recorded movement failed");
 const auto& p=r.packets.back();check(p.size()==18&&p[0]==41&&p[14]==0&&p[16]==35,"move lost current/target planes");
 r=a.apply({"SKILL_ACTIVATED",{"2192","52","54"}});check(r.status==Status::applied&&get(r.packets[0],6)==52&&get(r.packets[0],10)==54,"caster/target reversed");
 r=a.apply({"SKILL_FINISHED",{"52","0","0"}});check(r.status==Status::applied&&get(r.packets[0],2)==58&&get(r.packets[0],10)==2192,"completion identity lost");
 check(a.apply({"SKILL_FINISHED",{"52","0","0"}}).status==Status::unsupported,"unmatched completion guessed");
 a.apply({"SKILL_ACTIVATED",{"2192","52","54"}});r=a.apply({"SKILL_STOPPED",{"52","0","0"}});check(get(r.packets[0],2)==59,"interruption mapped as success");
 r=a.apply({"DAMAGE",{"54","52","-1.2","55"}});check(r.status==Status::applied&&std::bit_cast<float>(get(r.packets[0],14))==-1.2f&&a.state(52)->health_fraction==0,"raw overkill altered");
 r=a.apply({"HEAL",{"54","52","0.037383","ARMORIGNORING"}});check(r.status==Status::applied&&r.packets.size()==2&&std::bit_cast<float>(get(r.packets[0],14))==.037383f&&a.state(52)->health_fraction==.037383f,"heal fraction misinterpreted");
 check(a.apply({"DAMAGE",{"54","52","0.1","FIRE"}}).status==Status::unsupported,"unverified damage type guessed");
 check(a.apply({"DAMAGE",{"54","52","nan","55"}}).status==Status::invalid,"nonfinite combat accepted");
 r=a.apply({"HEAL",{"0","52","0.022222","ARMORIGNORING"}});check(r.status==Status::applied&&get(r.packets[0],10)==0,"anonymous healing rejected or attributed to invented actor");
 r=a.apply({"DAMAGE",{"0","52","-0.01","55"}});check(r.status==Status::applied&&get(r.packets[0],10)==0,"environmental damage rejected");
 check(a.apply({"GAME_SMSG_AGENT_MOVE_TO_POINT",{"900","1","2","0"}}).status==Status::missing_actor,"unspawned agent used");
 check(a.apply({"INSTANT_SKILL_USED",{"349","52","52"}}).packets.size()==1,"instant incorrectly synthesized completion");
 check(a.apply({"INSTANT_SKILL_USED",{"3453","52","52"}}).status==Status::unsupported,"modern skill silently mapped");
 r.packets=a.sync(52,{2,.5f,0x10,0});check(r.packets.size()==2&&r.packets[1][0]==240,"snapshot effects/death omitted");
 check(Adapter::stop(52).size()==6&&Adapter::position(52,1,2,35).size()==16,"movement fixture shapes");
 check(Adapter::facing(52,1,2)[0]==46&&Adapter::speed(52,220)[0]==39,"snapshot movement resources");
 a.seed(52,{35,1,0,2192,false});
 r=a.apply({"INSTANT_SKILL_USED",{"349","52","52"}});check(get(r.packets[0],2)==48&&a.state(52)->active_skill==2192,"instant cancelled ongoing cast");
 r=a.apply({"ATTACK_SKILL_ACTIVATED",{"392","52","54"}});check(get(r.packets[0],2)==50,"attack skill action");
 r=a.apply({"ATTACK_SKILL_FINISHED",{"52","0","54"}});check(get(r.packets[0],2)==46,"attack skill completion");
 r=a.apply({"ATTACK_STARTED",{"52","54"}});check(get(r.packets[0],2)==4,"ordinary attack start");
 r=a.apply({"ATTACK_FINISHED",{"52","0","54"}});check(get(r.packets[0],2)==1,"ordinary melee completion");
 r=a.apply({"INTERRUPTED",{"52","0","0"}});check(get(r.packets[0],2)==35,"interrupt notification");
 r=a.apply({"DAMAGE",{"54","52","-0.1","17"}});check(get(r.packets[0],2)==17&&r.packets.size()==2,"critical needs notification and absolute reconciliation");
 a.apply({"SKILL_ACTIVATED",{"2192","52","54"}});a.sync(52,{35,0,0x10,0});check(!a.state(52)->active_skill,"death retained cast state");
 auto armor=fw::gw::observer::presentation::player_equipment({"PA",{"47","16","0x08320897","3","1","133165,133166","62723,85331,258467","2:7:802:160477:160478,160480:10,0,0,0:16:802|4:16:2109:298216:0,0:0,0,0,0:6:2109","0xFFFFFFFF","0x1"}});check(armor.diagnostics.empty()&&armor.packets.size()==4,"captured armor rejected");check(get(armor.packets[0],10)==1&&get(armor.packets[0],14)==(802u|(16u<<16))&&get(armor.packets[1],14)==10,"armor tint/index/dye mapping");check(get(armor.packets[2],10)==0,"head index wrong");
 armor=fw::gw::observer::presentation::player_equipment({"PA",{"47","16","0x08320897","3","1","","","2:7:3803:1:0,0:10,0,0,0:16:3803","0","1"}});check(armor.packets.empty()&&!armor.diagnostics.empty(),"modern armor silently mapped");
 armor=fw::gw::observer::presentation::player_equipment({"PA",{"47","16","0x08320897","3","1","","","2:7:802:1:0,0:14,0,0,0:16:802","0","1"}});check(armor.packets.empty()&&!armor.diagnostics.empty(),"invalid dye silently packed");
 auto timing=fw::gw::observer::presentation::attack_timing(85,1.33f,.67f);check(timing.size()==14&&timing[0]==53&&std::bit_cast<float>(get(timing,6))==1.33f&&std::bit_cast<float>(get(timing,10))==.67f,"native attack timing packet");bool timing_denied=false;try{fw::gw::observer::presentation::attack_timing(85,0,1);}catch(const std::invalid_argument&){timing_denied=true;}check(timing_denied,"missing captured interval accepted");
 std::vector<std::string> statuses(33,"0");statuses[31]="7";check(fw::gw::observer::presentation::snapshot_effects(statuses)==0,"visual effect count mistaken for status flags");statuses[20]="1";check(fw::gw::observer::presentation::snapshot_effects(statuses)==0x80,"captured enchantment not reconstructed");statuses[20]="0";statuses[12]="1";statuses[16]="1";check(fw::gw::observer::presentation::snapshot_effects(statuses)==2,"blind guessed unsupported bit");statuses[15]="1";statuses[8]="1";statuses[18]="1";statuses[19]="1";statuses[21]="1";check(fw::gw::observer::presentation::snapshot_effects(statuses)==0x8c1a,"combined captured effects wrong");statuses[12]="2";bool invalid_status=false;try{fw::gw::observer::presentation::snapshot_effects(statuses);}catch(const std::invalid_argument&){invalid_status=true;}check(invalid_status,"nonboolean captured status accepted");
 auto blue=fw::gw::observer::presentation::team_name_color(50,1);check(blue==Packet{158,0,65,0,0,0,50,0,0,0,1,0,0,0},"native blue name color fixture");
 check(fw::gw::observer::presentation::player_guild(50,3)==Packet{158,0,30,0,0,0,50,0,0,0,3,0,0,0},"native player guild fixture");
 auto red=fw::gw::observer::presentation::team_name_color(85,2);check(red.size()==14&&get(red,10)==2,"native red name color fixture");bool color_denied=false;try{fw::gw::observer::presentation::team_name_color(50,8);}catch(const std::invalid_argument&){color_denied=true;}check(color_denied,"invalid native palette accepted");
 std::cout<<"observer wire adapter tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}








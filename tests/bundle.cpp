#include "gw_bundle.h"
#include <fstream>
#include <iostream>
#include <map>
#include <source_location>
using namespace fw::gw;
using namespace bundles;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Bundle lifecycle line "+std::to_string(at.line()));
}
int main(int argc,char** argv)try{
 check(argc==2);std::ifstream input(argv[1]);check(bool(input));std::map<std::string,items::Packet> fixtures;
 std::string label,hex;while(input>>label>>hex){
  check(hex.size()%2==0);items::Packet packet;
  for(std::size_t at=0;at<hex.size();at+=2)packet.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(at,2),nullptr,16)));
  check(fixtures.emplace(label,std::move(packet)).second);
 }
 check(fixtures.size()==4);
 check(items::held_bundle_2022(1,42)==fixtures.at("hold"));
 check(items::held_bundle_2022(1,0)==fixtures.at("clear"));
 check(items::held_bundle_2022(1,42)==fixtures.at("hold_again"));
 check(items::held_bundle_2022(1,99)==fixtures.at("unresolved_item"));
 for(bool bad_inventory:{false,true}){bool rejected=false;
  try{(void)items::held_bundle_2022(bad_inventory?0:1,bad_inventory?42:65536);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 }
 const Scope owner{1,2,3};items::Definition item;item.id=42;item.file=1234;item.type=6;item.quantity=1;item.encoded_name=u"Authored bundle";
 for(const auto disposition:{Disposition::ground,Disposition::destroy})for(const auto cause:{Cause::manual,Cause::shadow_step,Cause::weapon_switch,Cause::death,Cause::zone_exit,Cause::replacement}){
  State empty(owner);const auto held=empty.acquire(owner,item,disposition,100);check(held&&!empty.held()&&held->held()->generation==1);
  check(!held->acquire(owner,item,disposition,100)); // Caller must explicitly retire the previous bundle first.
  const auto proposal=held->drop(owner,1,1,200,cause,123,456,7);
  check(proposal&&!proposal->state.held()&&held->held()); // Preparation is not consumption.
  check(proposal->drop.held.item.encoded_name==item.encoded_name&&proposal->drop.held.disposition==disposition);
  check(proposal->drop.x==123&&proposal->drop.y==456&&proposal->drop.plane==7&&proposal->drop.cause==cause);
  check(!proposal->state.drop(owner,1,2,200,cause,123,456,7));
  const auto next=proposal->state.acquire(owner,item,disposition,201);check(next&&next->held()->generation==2);
  check(!next->drop(owner,1,2,202,cause,0,0,0)&&!next->drop(owner,2,1,202,cause,0,0,0));
  check(bool(next->drop(owner,2,2,202,cause,0,0,0)));
 }
 State empty(owner);auto stale=owner;++stale.generation;
 check(!empty.acquire(stale,item,Disposition::ground,100));
 for(unsigned mode=0;mode<5;++mode){auto invalid=item;
  if(mode==0)invalid.type=27;if(mode==1)invalid.quantity=2;if(mode==2)invalid.id=0;
  if(mode==3)invalid.id=65536;if(mode==4)invalid.encoded_name=std::u16string(64,u'x');
  check(!empty.acquire(owner,invalid,Disposition::ground,100));
 }
 const auto held=*empty.acquire(owner,item,Disposition::ground,100);
 check(!held.drop(stale,1,1,100,Cause::shadow_step,0,0,0));
 check(!held.drop(owner,1,1,99,Cause::shadow_step,0,0,0));
 check(!held.drop(owner,1,0,100,Cause::shadow_step,0,0,0));
 check(!held.drop(owner,1,1,100,Cause::shadow_step,INFINITY,0,0));
 check(!held.drop(owner,1,1,100,static_cast<Cause>(99),0,0,0));
 std::cout<<"PASS native bundle wire fixtures and scoped copy-only acquire/drop,explicit disposition,origin and replay guards\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}


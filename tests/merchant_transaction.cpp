#include "gw_merchant_transaction.h"
#include <fstream>
#include <iostream>
#include <map>
int main(int argc,char** argv)try{
 namespace m=fw::gw::merchant::native;namespace inv=fw::gw::inventory;
 const auto check=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
 check(argc==2,"original merchant price fixtures required");
 std::ifstream input(argv[1]);std::map<std::string,fw::gw::items::Packet> fixtures;std::string name,hex;
 while(input>>name>>hex){auto& packet=fixtures[name];
  for(std::size_t i=0;i<hex.size();i+=2)packet.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));}
 auto owned=inv::baseline();owned.gold=500;owned.next_item=43;
 inv::Item item;item.definition.id=42;item.definition.file=1;item.definition.price=50;item.definition.quantity=5;item.bag=1;item.slot=0;
 owned.items.emplace(42,item);
 m::Binding original;original.actor=100;original.map=146;original.world=99;original.npc_generation=7;
 original.revision=10;original.deadline=1000;original.stock={{item.definition,5}};original.sellable={42};
 original.inventory_at_open=inv::encode(owned);
 const m::Observation observation{100,146,0,99,7,10,500,true};
 const auto admit=[&](const char* label){auto menu=original;auto result=m::authorize_wire(menu,fixtures.at(label),owned,observation);
  check(!menu.actor,"attempt did not consume authority");return result;};
 auto buy=admit("focus");check(buy&&buy->quote.type==1&&buy->quote.gold==100,"original native buy rejected");
 auto partial=admit("sell_partial"),whole=admit("sell_whole_stack");
 check(partial&&partial->quote.quantity==3&&partial->quote.gold==150,"partial selected sell mismatch");
 check(whole&&whole->quote.quantity==5&&whole->quote.gold==250,"whole72 did not select full owned stack");
 check(m::still_current(*buy,owned,10),"unchanged admitted inventory rejected");
 auto changed=owned;changed.gold--;
 check(!m::still_current(*buy,changed,10)&&!m::still_current(*buy,owned,11),"stale inventory admitted to commit");
 const auto deny=[&](m::Request request,m::Binding binding,m::Observation observed,inv::Profile profile){
  binding.inventory_at_open=inv::encode(profile);
  check(!m::authorize(binding,request,profile,observed)&&!binding.actor,"forged/stale merchant attempt admitted");};
 auto request=fw::gw::collectors::native::decode(fixtures.at("focus"));
 auto forged=request;forged.give_gold--;deny(forged,original,observation,owned);
 forged=request;forged.receive[0].item=43;deny(forged,original,observation,owned);
 forged=request;forged.opcode=72;forged.receive[0].quantity=0;deny(forged,original,observation,owned);
 changed=owned;changed.gold=99;deny(request,original,observation,changed);
 auto limited=original;limited.stock[0].maximum_quantity=0;deny(request,limited,observation,owned);
 limited=original;limited.stock.push_back(limited.stock[0]);deny(request,limited,observation,owned);
 for(unsigned field=0;field<7;++field){auto stale=observation;switch(field){
  case 0:++stale.actor;break;case 1:++stale.map;break;case 2:++stale.world;break;
  case 3:++stale.npc_generation;break;case 4:++stale.revision;break;
  case 5:stale.now=1000;break;case 6:stale.nearby=false;break;}
  deny(request,original,stale,owned);}
 auto sell=fw::gw::collectors::native::decode(fixtures.at("sell_partial"));
 limited=original;limited.sellable.clear();deny(sell,limited,observation,owned);
 changed=owned;changed.gold=99900;deny(sell,original,observation,changed);
 changed=owned;changed.items.at(42).definition.quantity=2;deny(sell,original,observation,changed);
 forged=sell;forged.receive_gold++;deny(forged,original,observation,owned);
 forged=sell;forged.give.push_back(forged.give[0]);deny(forged,original,observation,owned);
 changed=owned;changed.items.at(42).definition.price=51;auto stale=original;
 forged=sell;forged.receive_gold=153;
 check(!m::authorize(stale,forged,changed,observation)&&!stale.actor,"changed signature retained opening authority");
 auto free=original;free.stock[0].definition.price=0;
 check(m::authorize_wire(free,fixtures.at("starter_shield"),owned,observation).has_value(),"trusted free shield rejected");
 auto broken=fixtures.at("focus");broken.pop_back();auto menu=original;
 check(!m::authorize_wire(menu,broken,owned,observation)&&!menu.actor,"malformed frame retained authority");
 std::cout<<"PASS native merchant buy/sell authority, exact quote, whole/partial selection, free stock and stale guards\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

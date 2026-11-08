#include "gw_merchant_quote.h"
#include "gw_collector_transaction.h"
#include <cfenv>
#include <fstream>
#include <iostream>
#include <map>

int main(int argc,char** argv)try{
 namespace m=fw::gw::merchant;namespace n=fw::gw::collectors::native;
 const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 check(argc==2,"original merchant price fixtures required");
 std::ifstream input(argv[1]);std::map<std::string,n::Request> fixtures;std::string name,hex;
 while(input>>name>>hex){fw::gw::items::Packet packet;
  for(std::size_t i=0;i<hex.size();i+=2)packet.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
  fixtures.emplace(name,n::decode(packet));}
 struct Case {const char* name;unsigned value,quantity;float scale;};
 const Case buys[]={{"focus",50,1,1},{"starter_shield",0,1,1},{"gray_dye",25,1,1},
  {"salvage_10",20,1,1},{"identification",50,1,1},{"cape",100,1,1},{"flute",2,1,1},
  {"buy_value20_quantity3",20,3,1},{"buy_value51_scale125_quantity3",51,3,1.25f},
  {"buy_value25_scale025",25,1,.25f},{"buy_maxvalue_smallscale",0xffffffffu,1,.0001f}};
 fw::gw::items::Definition current;current.id=42;
 for(const auto& row:buys){current.price=row.value;
  const auto quote=m::buy_quote(current,{row.scale},row.quantity);const auto& original=fixtures.at(row.name);
  check(quote&&quote->item==42&&quote->type==original.type&&quote->quantity==row.quantity&&
        quote->gold==original.give_gold&&original.receive[0].quantity==row.quantity,
        "buy quote differs from original constructor/UI/sender/serializer");}
 for(const auto [label,quantity]:{std::pair{"sell_partial",3u},std::pair{"sell_whole_stack",5u}}){current.price=50;
  const auto quote=m::sell_quote(current,quantity);const auto& original=fixtures.at(label);
  check(quote&&quote->type==original.type&&quote->gold==original.receive_gold,"sell quote differs from original native request");}
 current.price=0;check(m::buy_quote(current,{1},1)->gold==0&&m::sell_quote(current,1)->gold==0,"trusted free item rejected");
 check(!m::buy_quote(current,{1},0)&&!m::sell_quote(current,0)&&!m::buy_quote(current,{1},256)&&
       !m::sell_quote(current,256),"quantity outside BYTE transaction range accepted");
 for(float scale:{0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
  check(!m::buy_quote(current,{scale},1),"nonpositive or nonfinite server scale accepted");
 current.price=4194303;const auto maximum=m::sell_quote(current,255);
 check(maximum&&maximum->gold==4194303u*255u,"native upper price/BYTE quantity boundary rejected");
 current.price=4194304;check(!m::sell_quote(current,1),"native rounder upper bound ignored");
 current.price=2097152;check(!m::buy_quote(current,{1},1),"buy multiplier exceeded native rounder range");
 current.price=0xffffffffu;check(!m::buy_quote(current,{1},255)&&!m::sell_quote(current,255),"overflowing quote accepted");
 current.id=0;current.price=50;check(!m::buy_quote(current,{1},1)&&!m::sell_quote(current,1),"quote lost authoritative item identity");
 current.id=42;current.price=51;const auto mode=std::fegetround();std::fesetround(FE_DOWNWARD);
 const auto down=m::buy_quote(current,{1.25f},3);std::fesetround(FE_UPWARD);
 const auto up=m::buy_quote(current,{1.25f},3);std::fesetround(mode);
 check(down&&up&&down->gold==384&&up->gold==384,"host rounding mode changed nearest-even native quote");
 std::cout<<"PASS original merchant price fixtures, trusted free stock, quantity/range guards and deterministic nearest-even rounding\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

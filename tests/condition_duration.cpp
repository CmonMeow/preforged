#include "gw_condition_duration.h"
#include "gw_condition_item_modifiers.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat::conditions;
void check(bool value){if(!value)throw std::runtime_error("condition duration");}
int main()try{
 const auto decode=[](std::initializer_list<std::uint32_t> words){return fw::gw::items::modifiers::condition_bonuses(std::span(words.begin(),words.size()));};
 const auto poison=unsigned(Kind::poison),disease=unsigned(Kind::disease);
 check(decode({0x246801e4})->extension[poison]);
 check(decode({0x247801e4})->inscription[poison]);
 check(decode({0x27780605})->rune[poison]&&decode({0x27780605})->rune[disease]);
 check(decode({0x28580600})->rune[poison]);
 check(decode({0x246801e6})->extension[unsigned(Kind::weakness)]);
 check(!decode({0x24680000})&&!decode({0x246c01e4})&&!decode({0x27780905})&&!decode({0x28580601}));
 check(!decode({0x246801e4,0x246801e4}));
 check(decode({0x27780605,0x27780605})->rune[poison]); // Same rune collapses.
 check(equipment_reduced_duration(8000,{true,true})==4000);
 check(equipment_reduced_duration(7000,{true,true})==5000);
 check(equipment_reduced_duration(8000,{true,false})==6000);
 check(equipment_reduced_duration(7000,{false,true})==6000);
 for(unsigned seconds=1;seconds<=43200;++seconds){
  const auto base=std::uint64_t(seconds)*1000;
  check(equipment_reduced_duration(base,{})==base);
  check(equipment_reduced_duration(base,{true,false})==equipment_reduced_duration(base,{false,true}));
  const auto rounded=(seconds+2)/5;
  check(equipment_reduced_duration(base,{true,true})==std::uint64_t(seconds-2*rounded)*1000);
 }
 for(unsigned seconds=5;seconds<=25;++seconds){
  check(poisonous_duration(seconds*1000)==((seconds*133+50)/100)*1000);
  check(*poisonous_duration(seconds*1000)>=seconds*1000);
 }
 check(!equipment_reduced_duration(0,{})&&!equipment_reduced_duration(1500,{true,true}));
 check(!equipment_reduced_duration(UINT64_MAX,{true,true})&&!poisonous_duration(UINT64_MAX));
 check(!poisonous_duration(43200000)&&!poisonous_duration(999));
 std::cout<<"Condition equipment independent rounding,43,200duration boundaries and21Poison ranks pass\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

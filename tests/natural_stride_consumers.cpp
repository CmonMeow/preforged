#include "gw_movement_effects.h"
#include "gw_block_attribution.h"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
static void check(bool value){if(!value)throw std::runtime_error("Natural Stride consumer regression");}
int main()try{
 const Entry stride{1,2,1727,15,Category::stance,100,8100};
 MovementEffects movement;movement.apply(stride);
 check(movement.at(99)==1.f&&movement.at(100)==1.33f&&movement.at(8099)==1.33f&&movement.at(8100)==1.f);
 movement.remove(1,1100);check(movement.at(1099)==1.33f&&movement.at(1100)==1.f);
 check(std::abs(movement.integral(100,2100)-2330.)<.01);
 MovementEffects stack;stack.apply(stride);stack.apply({2,2,76,0,Category::hex,100,8100});
 check(std::abs(stack.at(100)-.665f)<.0001f);
 const std::array<Entry,1> defender{stride};
 check(reviewed_block_probability(defender,false,100)==.5);
 check(reviewed_block_probability(defender,false,8100)==0.);
 check(reviewed_block_probability(defender,true,100)==0.);
 const auto sources=reviewed_block_sources(defender,100);
 check(sources&&sources->size()==1&&sources->front().skill==1727&&sources->front().instance==1&&sources->front().probability==.5);
 const auto blocked=resolve_attributed_block_draw(*sources,.499);
 check(blocked&&blocked->source&&blocked->source->skill==1727);
 check(resolve_attributed_block_draw(*sources,.5)->outcome==fw::gw::combat::weapon::AttackOutcome::hit);
 const std::array<Entry,2> duplicate{stride,Entry{3,2,1727,15,Category::stance,100,8100}};
 check(reviewed_block_probability(duplicate,false,100)==.5&&reviewed_block_sources(duplicate,100)->size()==1);
 for(unsigned mode=0;mode<6;++mode){auto invalid=stride;
  if(mode==0)invalid.kind=Category::enchantment;
  if(mode==1)invalid.instance=0;
  if(mode==2)invalid.cause=0;
  if(mode==3)invalid.rank=21;
  if(mode==4)invalid.expires=8101;
  if(mode==5)invalid.expires.reset();
  check(!natural_stride_entry(invalid,100));MovementEffects rejected;rejected.apply(invalid);check(rejected.at(100)==1.f);
 }
 std::cout<<"PASS Natural Stride movement history, snare composition, blocking, attribution, expiry and malformed-entry rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

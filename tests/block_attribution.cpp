#include "gw_block_attribution.h"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
using Outcome=fw::gw::combat::weapon::AttackOutcome;
void check(bool ok){if(!ok)throw std::runtime_error("Block attribution regression");}
int main()try{
 const std::array<BlockSource,2> sources{{{258,1,.5},{11,2,.75}}};
 for(unsigned mode=0;mode<3;++mode){unsigned calls=0;
  const auto result=resolve_attributed_block(Outcome::hit,sources,false,{},100,[&](auto source){++calls;return mode==0?0.:source.skill==258?.9:mode==1?0.:.9;});
  check(bool(result));
  check(result->outcome==(mode==2?Outcome::hit:Outcome::block));
  check(calls==(mode==0?1u:2u));
  if(mode!=2)check(result->source&&result->source->skill==(mode==0?258u:11u));
  else check(!result->source);
 }
 unsigned blocks=0;
 for(unsigned a=0;a<100;++a)for(unsigned b=0;b<100;++b){
  const auto result=resolve_attributed_block(Outcome::hit,sources,false,{},100,[&](auto source){return (source.skill==258?a:b)/100.;});
  check(bool(result));blocks+=result->outcome==Outcome::block;
 }
 check(blocks==8750); // Independent .5/.75 effects yield .875 combined.
 for(unsigned i=0;i<10000;++i){
  const double draw=i/10000.;const auto result=resolve_attributed_block_draw(sources,draw);
  check(result&&result->draws==1);
  check(result->outcome==(draw<.875?Outcome::block:Outcome::hit));
  if(draw<.875)check(result->source->skill==(draw<.5?258u:11u));
 }
 const std::array<Entry,3> renewal{{{10,1,258,12,Category::enchantment,100,10000},
  {11,1,258,12,Category::enchantment,200,10000},{12,1,453,12,Category::stance,100,10000}}};
 const auto reviewed=reviewed_block_sources(renewal,300);
 check(reviewed&&reviewed->size()==2&&(*reviewed)[0].instance==11&&(*reviewed)[1].instance==12);
 check(reviewed_block_sources(renewal,10000)->empty());
 const auto no_draw=[](const BlockSource&)->double{throw std::runtime_error("unexpected draw");};
 for(auto earlier:{Outcome::miss,Outcome::dodge,Outcome::obstruction,Outcome::cancelled,Outcome::target_retired,Outcome::block}){
  const auto result=resolve_attributed_block(earlier,sources,false,{},100,no_draw);check(result&&result->outcome==earlier&&!result->source&&!result->draws);
 }
 check(resolve_attributed_block(Outcome::hit,sources,true,{},100,no_draw)->outcome==Outcome::hit);
 const Entry rigor{3,4,137,12,Category::hex,100,18100};
 check(resolve_attributed_block(Outcome::hit,sources,false,std::span<const Entry>(&rigor,1),100,no_draw)->draws==0);
 const std::array<BlockSource,2> invalid{{{258,1,.5},{11,1,.75}}};
 check(!resolve_attributed_block(Outcome::hit,invalid,true,{},100,no_draw));
 check(!resolve_attributed_block(Outcome::hit,sources,false,{},100,[](auto){return 1.;}));
 std::cout<<"PASS per-source block attribution, independent combined probability, bypass/failure no-draw and invalid-state rejection; retail source order unverified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

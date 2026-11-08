#include "gw_inspired_hex.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Inspired Hex policy regression");}
int main()try{
 for(unsigned skill:{22u,1049u}){
  check(inspired_hex(skill,0)->energy==4&&inspired_hex(skill,12)->energy==9&&inspired_hex(skill,20)->energy==12&&!inspired_hex(skill,21));
  const Scope caster{1,2,1},ally{1,3,1};Registry registry(ally);
  const auto none=prepare_inspired_hex(registry,caster,ally,*inspired_hex(skill,12),100);check(none&&!none->removed&&none->energy==0);
  check(registry.apply(ally,{7,4,28,12,Category::hex,100,6100},Replace::same_skill).accepted);
  check(registry.apply(ally,{8,4,30,12,Category::hex,200,6200},Replace::same_skill).accepted);
  const auto proposal=prepare_inspired_hex(registry,caster,ally,*inspired_hex(skill,12),300);
  check(proposal&&proposal->removed->instance==8&&proposal->energy==9&&proposal->copy.kind==HexCopyKind::player&&proposal->copy.actual==30);
  check(proposal->registry.lookup(ally,7)&&!proposal->registry.lookup(ally,8)&&registry.lookup(ally,8));
  check(!prepare_inspired_hex(registry,{2,2,1},ally,*inspired_hex(skill,12),300));
  check(!prepare_inspired_hex(registry,caster,{1,3,2},*inspired_hex(skill,12),300));
  check(!prepare_inspired_hex(registry,caster,ally,*inspired_hex(skill,12),UINT64_MAX-100));
 }
 check(classify_hex_copy(3152).kind==HexCopyKind::player_pvp_alias&&classify_hex_copy(3152).base==54);
 unsigned monster=0;for(const auto& row:fw::gw::skills::skill_catalog_2022)if(row.type==4&&classify_hex_copy(row.id).kind==HexCopyKind::monster_or_unobtainable){monster=row.id;break;}
 check(monster!=0);const Scope owner{1,2,1};Registry registry(owner);check(registry.apply(owner,{1,3,monster,0,Category::hex,100,5000},Replace::same_skill).accepted);
 const auto result=prepare_inspired_hex(registry,owner,owner,*inspired_hex(22,12),200);
 check(result&&result->energy==9&&result->removed&&result->copy.kind==HexCopyKind::monster_or_unobtainable&&result->copy.base==0);
 std::cout<<"PASS Inspired/Revealed Hex removal proposal, Energy scale, cover order, immutable original, lifetime/overflow and monster/PvP-alias classification; World/temp-bar pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

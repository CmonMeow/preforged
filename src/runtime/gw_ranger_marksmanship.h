#pragma once
#include "gw_projectile_flight.h"
#include <limits>
namespace fw::gw::skills::ranger {
// Local wiki: Point_Blank_Shot.html (da6b25e116c83286d13f1b019bec6cf4ceccf30d505fc85518ceafc00cfd66a9).
// Expertise affects the bonus; range is half the equipped bow's normal range.
struct PointBlankShot {unsigned bonus;double range;};
inline std::optional<PointBlankShot> point_blank_shot(unsigned expertise,double bow_range){
 if(expertise>20||!std::isfinite(bow_range)||bow_range<=0)return {};
 return PointBlankShot{10+2*expertise,bow_range*.5};
}
// Read_the_Wind.html (a8c33906ad36acfdf3667747e1b216614273102705c7223fee00a2de01db3e0b).
// This policy has no packet mapping and does not enable the skill in casting.
class ReadTheWind {
 std::uint64_t expiry_=0;
 unsigned bonus_=0;
 bool prepared_=false;
public:
 bool prepare(unsigned marksmanship,std::uint64_t now){
  if(marksmanship>20||now>UINT64_MAX-24000)return false;
  bonus_=static_cast<unsigned>(std::lround(3.+7.*marksmanship/15.));
  expiry_=now+24000;prepared_=true;return true;
 }
 bool active(std::uint64_t now)const{return prepared_&&now<expiry_;}
 unsigned bonus(std::uint64_t now)const{return active(now)?bonus_:0;}
 void remove(){prepared_=false;}
 std::optional<projectiles::Policy> flight(projectiles::Policy bow,std::uint64_t now)const{
  if(!bow.valid())return {};
  if(active(now))bow.speed*=2;
  if(!bow.valid())return {};
  return bow;
 }
};
}

#include "gw_burning_speed.h"
#include "gw_movement_effects.h"
#include <cmath>
#include <stdexcept>
using namespace fw::gw::skills::effects;
static void check(bool value){if(!value)throw std::runtime_error("Burning Speed policy");}
int main(){
 const auto low=burning_speed(0),mid=burning_speed(15),high=burning_speed(20);
 check(low&&mid&&high&&!burning_speed(21));
 check(low->energy==10&&low->activation_ms==250&&low->aftercast_ms==750&&
  low->recharge_ms==0&&low->duration_ms==7000&&low->self_burning);
 check(low->movement_bonus_percent==30&&mid->movement_bonus_percent==45&&
  high->movement_bonus_percent==50);
 check(low->ending_burning_ms==3000&&mid->ending_burning_ms==9000&&
  high->ending_burning_ms==11000);
 MovementEffects movement;
 movement.apply({1,1,823,15,Category::enchantment,100,7100});
 check(std::abs(movement.at(99)-1.f)<.0001f);
 check(std::abs(movement.at(100)-1.45f)<.0001f);
 movement.remove(1,1100);
 check(std::abs(movement.at(1099)-1.45f)<.0001f&&std::abs(movement.at(1100)-1.f)<.0001f);
 MovementEffects wrong_kind;
 wrong_kind.apply({1,1,823,15,Category::stance,100,7100});
 check(std::abs(wrong_kind.at(100)-1.f)<.0001f);
 const Scope caster{2,7,3},near{2,8,1},far{2,9,1};
 const Entry ended{17,7,823,15,Category::enchantment,100,7100};
 std::vector<BurningSpeedFoe> foes{{near,166,0,true,{}},{far,167,0,true,{}}};
 auto burn=burning_speed_end(ended,caster,7000,0,0,foes,1100);
 check(burn&&burn->size()==1&&burn->front().scope==near);
 const auto applied=burn->front().conditions.lookup(fw::gw::combat::conditions::Kind::burning);
 check(applied&&applied->expires==10100&&applied->cause==caster.actor);
 check(!foes.front().conditions.has(fw::gw::combat::conditions::Kind::burning,1100));
 foes[1].scope=near;check(!burning_speed_end(ended,caster,7000,0,0,foes,1100));
 foes[1].scope=far;foes[0].eligible=false;
 check(burning_speed_end(ended,caster,7000,0,0,foes,1100)->empty());
 check(!burning_speed_end(ended,{2,7,4},7000,0,0,foes,99));
 auto invalid_end=ended;invalid_end.expires.reset();
 check(!burning_speed_end(invalid_end,caster,7000,0,0,foes,1100));
 invalid_end.expires=7101;
 check(!burning_speed_end(invalid_end,caster,7000,0,0,foes,1100));
 invalid_end.expires=8500;
 check(bool(burning_speed_end(invalid_end,caster,8400,0,0,foes,1100)));
 check(!burning_speed_end(invalid_end,caster,7000,0,0,foes,1100));
 foes[0].eligible=true;foes[0].scope.instance=3;
 check(!burning_speed_end(ended,caster,7000,0,0,foes,1100));
}

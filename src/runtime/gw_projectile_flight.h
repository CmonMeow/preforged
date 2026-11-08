#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <optional>
namespace fw::gw::projectiles {
// Explicit private simulation policy. Retail flight speed/hit tolerance remain
// uncalibrated; changing these values never changes reviewed skill damage.
struct Policy {double speed=1800,hit_radius=32;bool valid()const{return std::isfinite(speed)&&speed>=100&&speed<=10000&&std::isfinite(hit_radius)&&hit_radius>=0&&hit_radius<=166;}};
struct Aim {float x,y;std::uint64_t flight_ms;};
inline std::optional<Aim> intercept(Policy p,double sx,double sy,double tx,double ty,double vx,double vy){
 if(!p.valid()||!std::isfinite(sx)||!std::isfinite(sy)||!std::isfinite(tx)||!std::isfinite(ty)||!std::isfinite(vx)||!std::isfinite(vy))return {};
 const double dx=tx-sx,dy=ty-sy,a=vx*vx+vy*vy-p.speed*p.speed,b=2*(dx*vx+dy*vy),c=dx*dx+dy*dy;
 double seconds=0;if(c>0){if(std::abs(a)<1e-9){if(b>=0)return {};seconds=-c/b;}else{const auto discriminant=b*b-4*a*c;if(discriminant<0)return {};const auto one=(-b-std::sqrt(discriminant))/(2*a),two=(-b+std::sqrt(discriminant))/(2*a);seconds=one>0?one:two;if(one>0&&two>0)seconds=std::min(one,two);}}
 if(!std::isfinite(seconds)||seconds<0||seconds>30)return {};
 return Aim{static_cast<float>(tx+vx*seconds),static_cast<float>(ty+vy*seconds),std::max<std::uint64_t>(1,static_cast<std::uint64_t>(std::ceil(seconds*1000)))};
}
}


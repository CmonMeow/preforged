#include "gw_armor_earth.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Armor Earth line "+std::to_string(at.line()));}
int main()try{
 const unsigned armor[]={24,26,29,31,34,36,38,41,43,46,48,50,53,55,58,60,62,65,67,70,72};
 const unsigned slow[]={50,48,45,43,40,38,36,33,31,28,26,24,21,19,16,14,12,9,7,4,2};
 for(unsigned rank=0;rank<=20;++rank){auto p=armor_earth(rank);check(p&&p->armor==armor[rank]&&p->slow_percent==slow[rank]&&p->duration==30000);check(std::abs(p->movement_multiplier()-(100.-slow[rank])/100.)<1e-9);}
 check(!armor_earth(21));const Scope owner{7,42,9};Registry r(owner);const auto p=*armor_earth(12);
 auto a=prepare_armor_earth(r,owner,1,p,100);check(a&&r.size()==0&&a->registry.size()==1&&a->change.removed.empty());
 check(!active_armor_earth(a->registry,owner,99)&&active_armor_earth(a->registry,owner,100)==p&&active_armor_earth(a->registry,owner,30099)==p&&!active_armor_earth(a->registry,owner,30100));
 check(!active_armor_earth(a->registry,{7,42,10},100));
 check(armor_earth_rating(60,p)==113&&armor_earth_rating(80,p)==133&&!armor_earth_rating({},p)&&armor_earth_rating(500,p)==553&&!armor_earth_rating(501,p));
 combat::Actor actor{42,9,combat::Health{500,500},60,0,true,80,70,{}};
 auto buff=armor_earth_actor(actor,a->registry,owner,100);check(buff&&buff->earth_armor==113&&buff->lightning_armor==133&&buff->fire_armor==123&&!buff->cold_armor);
 check(actor.earth_armor==60&&armor_earth_actor(actor,a->registry,owner,30100)->earth_armor==60);
 check(!armor_earth_actor(actor,a->registry,{7,42,10},100));
 // The common actor resolver proves all reviewed elemental paths respect the
 // bonus; armor-ignoring damage still bypasses it. Health/resources unchanged.
 for(auto kind:{combat::Kind::earth,combat::Kind::lightning,combat::Kind::fire,combat::Kind::armor_ignoring}){
  auto before=actor,after=*buff;auto x=combat::apply(before,{kind,100,20,172},true),y=combat::apply(after,{kind,100,20,172},true);
  check(x.reason==combat::Reason::accepted&&y.reason==combat::Reason::accepted);
  check(std::abs(y.damage/x.damage-(kind==combat::Kind::armor_ignoring?1.:std::exp2(-53./40.)))<1e-9);
 }
 auto renewal=prepare_armor_earth(a->registry,owner,2,*armor_earth(0),200);check(renewal&&renewal->change.removed.size()==1&&renewal->change.removed[0].instance==1&&active_armor_earth(renewal->registry,owner,200)->armor==24);
 check(renewal->registry.remove(owner,2)&&!active_armor_earth(renewal->registry,owner,201));
 auto forged=p;++forged.armor;check(!prepare_armor_earth(r,owner,1,forged,100));
 check(!prepare_armor_earth(r,owner,0,p,100)&&!prepare_armor_earth(r,{7,42,10},1,p,100)&&!prepare_armor_earth(r,owner,1,p,UINT64_MAX-29999)&&!prepare_armor_earth(a->registry,owner,1,p,101));
 std::cout<<"PASS Armor Earth rank0..20, immutable owned renewal/removal/expiry, all elemental armor and armor-ignoring controls, unknown base preservation and forged/lifetime bounds\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

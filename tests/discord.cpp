#include "gw_discord.h"
#include <iostream>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Discord policy");}
int main()try{
 for(bool pvp:{false,true})for(unsigned rank=0;rank<=20;++rank){const auto p=discord(rank,pvp);check(bool(p));
  const auto d=discord_damage(*p,20);check(d&&d->skill==(pvp?2863u:817u)&&d->kind==combat::Kind::armor_ignoring&&std::abs(d->amount-(30.+80.*rank/15.))<1e-9);
  for(bool c:{false,true})for(bool h:{false,true})for(bool e:{false,true}){const auto ready=discord_ready(c,h,e);check(ready&&*ready==(c&&(h||e)));}
  auto bad=*p;++bad.damage;check(!discord_damage(bad,20));
 }
 check(!discord(21));check(discord_ready(false,{},{} )==false);check(discord_ready({},false,false)==false);
 check(!discord_ready(true,false,{}));check(discord_ready(true,true,{})==true);check(!discord_ready({},true,false));
 std::cout<<"PASS Discord both modes/all21ranks,8predicate combinations,unknown facts and altered plans\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

#include "gw_blessed_signet.h"
#include "gw_blessed_signet_wire.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw::skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Blessed Signet line "+std::to_string(at.line()));}
int main()try{
 const auto wire=blessed_signet_wire(42,900000,50,300000);check(wire&&wire->popup);
 check(wire->delta==fw::gw::combat::Packet({162,0,52,0,0,0,42,0,0,0,42,0,0,0,143,194,117,61}));
 check(*wire->popup==fw::gw::combat::Packet({159,0,54,0,0,0,42,0,0,0,42,0,0,0,3,0,0,0}));
 const auto fractional=blessed_signet_wire(42,450000,50,300000);check(fractional&&!fractional->popup);
 check(!blessed_signet_wire(0,900000,50,300000));check(!blessed_signet_wire(65536,900000,50,300000));
 check(!blessed_signet_wire(42,0,50,300000));check(!blessed_signet_wire(42,900000,0,300000));check(!blessed_signet_wire(42,900000,50,0));
 const Scope caster{1,1,1},other{1,2,1};
 for(unsigned rank=0;rank<=20;++rank)for(unsigned count=0;count<=12;++count){
  MaintainedEnchantments ledger;const auto plan=*blessed_signet(rank);
  for(unsigned n=0;n<count;++n){Scope target{1,100+n,1};Registry registry(target);check(registry.apply(target,{n+1,1,290,12,Category::enchantment,100,{}},Replace::independent).accepted);check(ledger.add_mending(caster,target,n+1,registry,100));}
  check(ledger.count(caster,100)==count);check(blessed_signet_gain(plan,caster,ledger,100)==std::min(3*count,interpolate(3,24,rank)));
  check(blessed_signet_gain(plan,other,ledger,100)==0);check(ledger.size()==count);
  auto forged=plan;++forged.per_enchantment;check(!blessed_signet_gain(forged,caster,ledger,100));
  if(count)check(!ledger.count(caster,99));else check(ledger.count(caster,99)==0);
 }
 for(unsigned mode=0;mode<10;++mode){MaintainedEnchantments ledger;const Scope target{1,100,1};Registry registry(target);Entry entry{7,1,290,12,Category::enchantment,100,{}};
  if(mode==1)entry.skill=288;if(mode==2)entry.cause=2;if(mode==3)entry.expires=500;
  check(registry.apply(target,entry,Replace::independent).accepted);auto source=caster,recipient=target;if(mode==4)source.instance=2;if(mode==5)recipient.generation=2;if(mode==6)source.generation=0;
  const bool accepted=ledger.add_mending(source,recipient,mode==7?8:7,registry,mode==8?101:mode==9?99:100);check(accepted==(mode==0));
  if(!accepted){check(ledger.size()==0);continue;}
  check(!ledger.add_mending(source,recipient,7,registry,100));check(registry.apply(target,{8,1,290,12,Category::enchantment,100,{}},Replace::independent).accepted);check(!ledger.add_mending(source,recipient,8,registry,100));check(!ledger.ended(Scope{1,100,2},7,100));check(ledger.count(caster,100)==1);
  check(ledger.retire(Scope{1,1,2},100)==0);check(ledger.ended(target,7,100));check(!ledger.ended(target,7,100));check(ledger.count(caster,100)==0);
 }
 for(bool retire_caster:{false,true}){MaintainedEnchantments ledger;Scope target{1,100,1};Registry registry(target);check(registry.apply(target,{7,1,290,12,Category::enchantment,100,{}},Replace::independent).accepted);check(ledger.add_mending(caster,target,7,registry,100));check(ledger.retire(retire_caster?caster:target,200)==1);check(!ledger.count(caster,100));check(ledger.count(caster,200)==0);check(!ledger.retire(caster,199));}
 check(!blessed_signet(21));MaintainedEnchantments empty;check(!blessed_signet_gain(*blessed_signet(12),{},empty,100));
 std::cout<<"PASS273 rank/maintained-count proposals,10 producer guards,exact removal and source/recipient retirement;policy-only; World/Runtime callers checked separately\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


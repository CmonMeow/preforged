#include "gw_health_regeneration.h"
#include <iostream>
#include <stdexcept>
int main()try{
 using namespace fw::gw::skills;using namespace regeneration;
 auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
 effects::Scope scope{1,42,7};auto breeze=reviewed(288,12),troll=reviewed(446,12);
 check(breeze&&breeze->pips==8&&breeze->duration==15000&&!breeze->self_only,"reviewed Breeze");
 check(troll&&troll->pips==9&&troll->duration==13000&&troll->self_only,"reviewed Troll");
 for(unsigned rank:{0u,12u,15u}){
  const auto refuge=reviewed(814,rank);
  check(refuge&&refuge->self_only&&refuge->duration==6000&&
        refuge->pips==(rank==0?5:rank==12?9:10)&&
        end_healing(814,rank)==40+4*rank,"Shadow Refuge ranked regeneration/end heal");
 }
 check(!reviewed(814,21)&&end_healing(814,21)==0,"Shadow Refuge rank bound");
 for(const auto [rank,duration]:{std::pair{0u,8000u},std::pair{12u,16000u},std::pair{15u,18000u}}){
  const auto restful=reviewed(886,rank);
  check(restful&&restful->pips==10&&restful->duration==duration&&!restful->self_only,"reviewed Restful Breeze");
 }
 check(!reviewed(886,21),"Restful Breeze rejects impossible rank");
 State restful_dense(scope,0),restful_sparse(scope,0);
 check(restful_dense.apply(scope,{3,42,886,10,0,16000})&&restful_sparse.apply(scope,{3,42,886,10,0,16000}),"Restful Breeze owned source");
 double rh_dense=100,rh_sparse=100;
 for(unsigned at=100;at<=2000;at+=100)check(restful_dense.advance(scope,at,rh_dense,480),"dense Restful advance");
 check(restful_dense.end(scope,3,2000)&&restful_sparse.end(scope,3,2000),"Restful early end at action time");
 for(unsigned at=2100;at<=4000;at+=100)check(restful_dense.advance(scope,at,rh_dense,480),"dense Restful post-end");
 check(restful_sparse.advance(scope,4000,rh_sparse,480)&&rh_dense==140&&rh_sparse==rh_dense,"Restful early end retains earned regeneration only");
 for(unsigned rank:{0u,12u,15u,20u}){
  const auto renewal=reviewed(115,rank);
  check(renewal&&renewal->duration==7000&&renewal->self_only,"Blood Renewal duration and self restriction");
  check(renewal->pips==static_cast<int>(std::lround(3.+rank/5.)),"Blood Renewal regeneration scale");
  check(end_healing(115,rank)==40+10*rank,"Blood Renewal end healing scale");
 }
 check(!reviewed(115,21)&&end_healing(115,21)==0&&end_healing(288,12)==0,"end healing rejects unsupported effect/rank");
 State whole(scope,0),split(scope,0);Source source{1,42,288,8,0,15000};check(whole.apply(scope,source)&&split.apply(scope,source),"timed source");
 double a=100,b=100;check(whole.advance(scope,16000,a,480),"whole delayed tick");
 for(unsigned now=25;now<=16000;now+=25)check(split.advance(scope,now,b,480),"fragmented tick");
 check(std::abs(a-340)<1e-8&&std::abs(a-b)<1e-8,"expiry overlap and fractional health invariant");
 State capped(scope,0);check(capped.apply(scope,{1,42,288,8,0,15000})&&capped.apply(scope,{2,42,446,9,0,13000}),"distinct sources stack");
 double c=100;capped.advance(scope,1000,c,480,-9);check(c==116,"cap after source and degeneration sum");
 double dead=0;check(capped.advance(scope,2000,dead,480)&&dead==0,"dead actors never revived by regeneration");
 check(!capped.advance({1,42,8},3000,c,480)&&c==116,"generation isolation");
 State degen_whole(scope,1000),degen_split(scope,1000);double dw=1000,ds=1000;
 check(degen_whole.advance(scope,4000,dw,1000,-4),"sparse degeneration");
 for(unsigned at=1100;at<=4000;at+=100)check(degen_split.advance(scope,at,ds,1000,-4),"dense degeneration");
 check(dw==976&&ds==dw&&std::ceil(ds)==976,"same integer publication after fractional degeneration");
 State fraction(scope,1000);double fh=1000;check(fraction.advance(scope,1100,fh,1000,-4)&&std::abs(fh-999.2)<1e-9,"retain real fractional Health");
 std::cout<<"Timed health regeneration semantic tests passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

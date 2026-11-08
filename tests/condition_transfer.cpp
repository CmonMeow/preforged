#include "gw_condition_transfer.h"
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("condition transfer regression");}
int main(){using namespace fw::gw::skills::effects;using namespace fw::gw::combat::conditions;
 check(condition_transfer(154,0)->count==1&&condition_transfer(154,12)->count==3&&!condition_transfer(149)&&!condition_transfer(154,21));
 State source,target;source.apply(Kind::poison,1,100,10000);source.apply(Kind::blind,2,200,5000);source.apply(Kind::weakness,3,300,20000);
 target.apply(Kind::weakness,4,300,30000);
 auto one=transfer_conditions(*condition_transfer(154,0),source,target,42,1000);check(one&&one->kinds.size()==1&&one->kinds[0]==Kind::weakness&&!one->source.has(Kind::weakness,1000)&&one->target.lookup(Kind::weakness)->expires==30300);
 auto all=transfer_conditions(*condition_transfer(154,12),source,{},42,1000);check(all&&all->kinds.size()==3&&all->kinds[0]==Kind::weakness&&all->kinds[1]==Kind::blind&&all->kinds[2]==Kind::poison&&all->source.count(1000)==0);
 check(all->target.lookup(Kind::poison)->expires==10100&&all->target.lookup(Kind::blind)->expires==5200&&all->target.lookup(Kind::blind)->cause==42);
 auto expired=transfer_conditions(*condition_transfer(154,12),source,{},42,21000);check(expired&&expired->kinds.empty());
 auto forged=*condition_transfer(154);forged.count=10;check(!transfer_conditions(forged,source,target,42,1000));
 source.apply(Kind::poison,1,400,100);auto covered=transfer_conditions(*condition_transfer(154,0),source,{},42,1000);check(covered&&covered->kinds[0]==Kind::poison&&covered->target.lookup(Kind::poison)->expires==10100);
}

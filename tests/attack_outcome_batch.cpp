#include "gw_attack_outcome_batch.h"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat::weapon;
void check(bool ok){if(!ok)throw std::runtime_error("Attack outcome batch regression");}
int main(){
 const AttackScope a{1,1,10},b{1,2,20},c{1,3,30};AttackOutcomes current(a);check(current.release(a,b,1,100));
 const std::array valid{AttackRelease{b,2,AttackDelivery::melee},AttackRelease{c,3,AttackDelivery::ranged}};
 auto proposed=stage_attack_outcome_batch(current,a,valid,200);check(proposed&&proposed->size()==3&&current.size()==1);
 auto invalid=valid;invalid[1].target.generation=0;
 check(!stage_attack_outcome_batch(current,a,invalid,200)&&current.size()==1);
 invalid=valid;invalid[1].event=2;check(!stage_attack_outcome_batch(current,a,invalid,200));
 invalid=valid;invalid[1].delivery=static_cast<AttackDelivery>(99);check(!stage_attack_outcome_batch(current,a,invalid,200)&&current.size()==1);
 check(!stage_attack_outcome_batch(current,{1,1,11},valid,200));
 check(!stage_attack_outcome_batch(current,a,valid,99)&&!stage_attack_outcome_batch(current,a,{},200));
 // Failure cannot consume or retime a pre-existing projectile.
 check(current.finish(a,b,1,AttackOutcome::hit,150).has_value());
 current=std::move(*proposed);
 const auto ranged=current.finish(a,c,3,AttackOutcome::miss,250);check(ranged&&ranged->delivery==AttackDelivery::ranged);
 check(current.finish(a,b,1,AttackOutcome::hit,300).has_value());
 const auto melee=current.finish(a,b,2,AttackOutcome::miss,300);check(melee&&melee->delivery==AttackDelivery::melee&&current.size()==0);
 check(!current.finish(a,b,2,AttackOutcome::miss,300));
 AttackOutcomes capacity(a);for(unsigned id=1;id<=4095;++id)check(capacity.release(a,b,id,id));
 const std::array overflow{AttackRelease{b,4096},AttackRelease{c,4097}};
 check(!stage_attack_outcome_batch(capacity,a,overflow,5000)&&capacity.size()==4095);
 proposed=stage_attack_outcome_batch(capacity,a,std::span(overflow).first(1),5000);
 check(proposed&&proposed->size()==4096&&capacity.size()==4095);
 std::cout<<"PASS atomic multi-recipient release staging,late rejection rollback,existing flights and capacity;world/cardinality wiring pending\n";
}

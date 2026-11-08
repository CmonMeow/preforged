#include "gw_reversal_fortune.h"
#include "gw_combat.h"
#include "gw_skill_particle_wire.h"
#include <iostream>
#include <string>
#include <fstream>
#include <limits>
#include <source_location>
using namespace fw::gw;
using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Reversal Fortune foundation line "+std::to_string(at.line()));
}
int main(int argc,char** argv)try{
    check(argc<=2);
    const Scope owner{9,42,3};
    for(unsigned rank=0;rank<=20;++rank){const auto p=reversal_fortune(rank);check(p&&p->maximum_healing==interpolate(15,80,rank)&&p->duration==8000);
        for(auto input:{ReversalInput::damage,ReversalInput::life_steal})for(double amount:{.5,10.,67.,200.,16000000.}){
            Registry r(owner);check(r.apply(owner,{1,43,307,rank,Category::enchantment,100,8100},Replace::same_skill).accepted);
            const auto q=prepare_reversal_fortune(r,owner,input,amount,100);check(q&&q->split.healing==std::min(amount,double(p->maximum_healing))&&q->split.prevented==q->split.healing&&q->split.remaining+q->split.prevented==amount);
            check(r.size()==1&&consume_reversal_fortune(r,*q,100)&&r.size()==0&&!consume_reversal_fortune(r,*q,100));
        }
    }
    check(!reversal_fortune(21));
    Registry r(owner);const Entry entry{5,43,307,12,Category::enchantment,100,8100};
    check(r.apply(owner,entry,Replace::same_skill).accepted);
    check(!r.entry_sequence({9,42,4},5)&&!r.entry_sequence(owner,999));
    check(!prepare_reversal_fortune(r,owner,ReversalInput::damage,10,99)&&prepare_reversal_fortune(r,owner,ReversalInput::damage,10,8099)&&!prepare_reversal_fortune(r,owner,ReversalInput::damage,10,8100));
    check(!prepare_reversal_fortune(r,{9,42,4},ReversalInput::damage,10,100));
    const auto q=*prepare_reversal_fortune(r,owner,ReversalInput::damage,200,100);
    check(q.split==DamageHealingSplit{67,67,133});
    auto forged=q;forged.split.healing=200;check(!consume_reversal_fortune(r,forged,100)&&r.size()==1);
    check(!consume_reversal_fortune(r,q,101)&&r.size()==1);
    check(r.apply(owner,entry,Replace::same_skill).accepted&&!consume_reversal_fortune(r,q,100)); // Same ID/time/rank/cause,new application.
    const auto replacement=*prepare_reversal_fortune(r,owner,ReversalInput::damage,200,100);
    check(replacement.application!=q.application&&r.remove(owner,5)&&!consume_reversal_fortune(r,replacement,100));
    check(r.apply(owner,entry,Replace::same_skill).accepted);const auto pending=*prepare_reversal_fortune(r,owner,ReversalInput::life_steal,20,100);
    check(r.expire(owner,8100).size()==1&&!consume_reversal_fortune(r,pending,100));
    check(r.apply(owner,entry,Replace::same_skill).accepted);
    for(double amount:{-1.,0.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),16000001.})
        check(!prepare_reversal_fortune(r,owner,ReversalInput::damage,amount,100)&&r.size()==1);
    check(!prepare_reversal_fortune(r,owner,static_cast<ReversalInput>(9),10,100));
    const auto preview=[&](double hp,unsigned maximum,double amount,double healing_factor){
        const auto split=*damage_to_healing(amount,67);const double healed=std::min(double(maximum)-hp,split.healing*healing_factor);
        return std::max(0.,hp+healed-split.remaining);};
    check(preview(10,500,100,1)==44); // Heal67 first,then33 excess;not lethal.
    check(preview(500,500,100,1)==467); // Full Health loses excess,not healed above max.
    check(preview(100,500,200,1)==34); // Prevent67 AND heal67,not a simple damage cap.
    check(preview(100,500,100,.5)==100.5); // The real consumer must apply healing modifiers.
    combat::Actor protected_actor{42,3,combat::Health{100,500},{},0,true};protected_actor.incoming_damage_cap=50;
    const auto protected_hit=combat::apply(protected_actor,{combat::Kind::armor_ignoring,200,20,42},true);
    check(protected_hit.damage==50&&damage_to_healing(protected_hit.damage,67)==DamageHealingSplit{50,50,0});
    // Do not use already-clamped applied loss: this target has only10 Health,
    // but the incoming100 must still quote67 healing/33 remaining damage.
    combat::Actor low{42,3,combat::Health{10,500},{},0,true};const auto lethal=combat::apply(low,{combat::Kind::armor_ignoring,100,20,42},true);
    check(lethal.applied==10&&lethal.damage==100&&damage_to_healing(lethal.damage,67)==DamageHealingSplit{67,67,33});
    if(argc==2){std::ofstream output(argv[1]);check(output.good());
        const auto body=skills::particles::target_body_2022(42,307);check(body.has_value());
        output<<"{\"skill\":307,\"constructed_not_paid_cast\":true,\"body\":\"";
        for(auto byte:*body){constexpr char hex[]="0123456789abcdef";output<<hex[byte>>4]<<hex[byte&15];}output<<"\"}\n";}
    std::cout<<"Reversal Fortune:210 rank/input splits,scoped one-use receipts,replacement/expiry and healing-first ordering models PASS;World casting remains unbound\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

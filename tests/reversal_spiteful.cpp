#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Spiteful Reversal line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& p,unsigned offset){unsigned value=0;for(unsigned i=0;i<4;++i)value|=unsigned(p[offset+i])<<(8*i);return value;}
struct Fixture {
    world::Instance w;bool npc;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    effects::Scope source,owner,side,bonded;
    Fixture(bool is_npc,unsigned hp=500,unsigned side_hp=500,bool known=true):npc(is_npc){
        if(npc){std::vector<world::Instance::Npc> population;
            for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?0:60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
                combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
                d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(id==100?hp:side_hp),1000};
                d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic";
                if(known||id==100){d.packed_allegiance=2;d.evidence.allegiance="synthetic";}
                n.combat_definition=d;population.push_back(n);
            }w.configure_npcs(population);
        }
        w.join(1,"Hexer",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        for(unsigned id:{2u,3u}){w.join(id,"Recipient",npc?700:id==2?0:60,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(id,{npc?1000u:id==2?hp:side_hp,1000,100,100},100);}
        source=*w.effect_scope(1);owner=*w.effect_scope(npc?100:2);side=*w.effect_scope(npc?101:3);bonded=*w.effect_scope(2);
        if(npc){check(w.elemental_armor(bonded,60,60));check(w.stage_npc_bonds_of_torment(owner,bonded,*effects::bonds_of_torment_npc(),100));}
    }
    double hp(effects::Scope scope)const{return scope.actor>=100?w.npc_combat_snapshot(scope)->health->current:w.resources(scope.actor)->health;}
    void reversal(unsigned rank){for(auto target:{owner,side})check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,900,rank,100,8100,true));}
    void hex(unsigned rank){check(w.spiteful_spirit(source,owner,*effects::spiteful_spirit(rank),20,100));}
    void source_state(unsigned state){if(state==1)check(w.kill(source,100));if(state>=2)w.leave(1);if(state==3){w.join(1,"Replacement",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);}}
    effects::SpitefulAction action(bool completion,unsigned at=200)const{return {owner,completion?effects::SpitefulPhase::skill_completed:effects::SpitefulPhase::attack_release,completion?1239u:0u,1,at,true};}
};
struct Result {double hp,applied;};
Result expected(unsigned hp,unsigned amount,int rank){const double heal=rank<0?0:std::min(double(amount),double(effects::reversal_fortune(rank)->maximum_healing));const auto healed=std::min(1000.,hp+heal);const auto applied=std::min(healed,amount-heal);return {healed-applied,applied};}
int main()try {
    unsigned cases=0;
    for(bool npc:{false,true})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u})
    for(unsigned hp:{10u,500u,1000u})for(unsigned state:{0u,1u,2u,3u})for(bool completion:{false,true}){
        ++cases;Fixture f(npc,hp);if(rr>=0)f.reversal(rr);f.hex(rank);f.source_state(state);f.box->drain();
        std::string why;check(f.w.spiteful_action(f.action(completion),&why)&&why=="applied");
        const auto primary=expected(hp,effects::spiteful_spirit(rank)->damage,rr),side=expected(500,effects::spiteful_spirit(rank)->damage,rr);
        check(std::abs(f.hp(f.owner)-primary.hp)<1.01&&std::abs(f.hp(f.side)-side.hp)<1.01);
        if(npc)check(std::abs(double(f.w.resources(2)->health)-(1000-primary.applied))<1.01); // Last lethal hit still triggers Bonds.
        const auto packets=f.box->drain();unsigned losses=0;
        for(unsigned i=0;i<packets.size();++i){const auto& p=packets[i];if(p.size()!=18||p[0]!=162||word(p,2)!=55||std::bit_cast<float>(word(p,14))>=0)continue;
            ++losses;check(state<2&&word(p,10)==1);const auto actor=word(p,6);check(actor==f.owner.actor||actor==f.side.actor);
            check(i&&packets[i-1]==combat::detail::integer(10,actor,121));
        }
        const auto heal=rr<0?0:std::min(effects::spiteful_spirit(rank)->damage,effects::reversal_fortune(rr)->maximum_healing);
        check(losses==unsigned(state<2&&effects::spiteful_spirit(rank)->damage>heal)*2);
        if(npc&&state<2)check(f.w.take_npc_deaths(f.source).size()==unsigned(primary.hp==0));
        check(!f.w.spiteful_action(f.action(completion,201),&why));check(f.hp(f.owner)==primary.hp&&f.hp(f.side)==side.hp);
        if(state==3)check(f.w.resources(1)->health==1000);
    }
    check(cases==576);
    // The complete area pulse survives either recipient's conversion-healing
    // death. Both phases must still affect the other member exactly once.
    for(bool npc:{false,true})for(bool wound_side:{false,true})for(unsigned state:{0u,1u,2u,3u})for(bool completion:{false,true}){
        ++cases;Fixture f(npc,wound_side?500:10,wound_side?10:500);f.reversal(12);f.hex(12);
        const auto target=wound_side?f.side:f.owner,other=wound_side?f.owner:f.side;
        if(npc){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,target,*effects::condition_transfer(132,12),100));}
        else check(f.w.apply_condition(f.source,target,combat::conditions::Kind::deep_wound,100,10000));
        f.source_state(state);check(f.w.spiteful_action(f.action(completion)));check(f.hp(target)==0&&f.hp(other)==529);
        if(npc&&state<2)check(f.w.take_npc_deaths(f.source).size()==1);
        f.w.tick(201);check(f.hp(target)==0&&f.hp(other)==529);if(npc&&state<2)check(f.w.take_npc_deaths(f.source).empty());
    }
    // Unknown allegiance rejects the area before consuming either conversion.
    {++cases;Fixture f(true,500,500,false);f.reversal(12);f.hex(12);f.box->drain();std::string why;
        check(!f.w.spiteful_action(f.action(false),&why)&&why=="unknown_relationship");
        check(f.hp(f.owner)==500&&f.hp(f.side)==500&&f.box->drain().empty());
        for(auto target:{f.owner,f.side}){const auto entries=*f.w.status_snapshot(target,200);check(std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;}));}
    }
    check(cases==609);
    std::cout<<"PASS "<<cases<<" Spiteful Reversal cases; area conversion, recipient121 markers, original-caster lifetime, lethal Bonds, healing death, action deduplication and unknown relationship admission;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

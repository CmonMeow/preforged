#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <fstream>
#include <limits>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
namespace casting=skills::casting;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("direct heal completion line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;effects::Scope source,target;unsigned skill,version,kind;
    std::shared_ptr<world::Outbox> caster_outbox=std::make_shared<world::Outbox>();
    unsigned attempts=0;bool stale=false;std::uint64_t event=0;std::vector<world::Packet> feedback;
    Fixture(unsigned spell,unsigned variant,unsigned recipient,unsigned health=100,bool npc_healing=true,float npc_x=60,unsigned npc_plane=0)
        :skill(spell),version(variant),kind(recipient){
        if(kind==2){world::Instance::Npc n;n.id=n.definition.id=100;n.x=npc_x;n.plane=npc_plane;
            n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
            d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={100,1000};
            if(spell==280||spell==1119)d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=true;
            d.healing_allowed=npc_healing;d.healing_evidence="authored direct heal completion";
            if(spell==1396){d.conditions_allowed=true;d.fleshy=true;d.fleshy_evidence="fixture";
                d.creature_type="human";d.creature_type_evidence="fixture";
                d.disease_immune=false;d.disease_immunity_evidence="fixture";}
            d.spawn_energy=combat::npcs::Energy{20,30};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});}
        w.join(1,"Caster",0,0,caster_outbox,0,{}, {},100,1);
        w.set_vitals(1,{health,1000,50,100},100);w.public_professions(1,6,3);
        if(kind==1){w.join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            w.set_vitals(2,{100,1000,30,30},100);}
        source=*w.effect_scope(1);target=*w.effect_scope(kind==0?1:kind==1?2:100);
        if(version){const auto renewal=*effects::ether_renewal(version,15);
            check(w.status_mutation(source,version,80,15,100,100+renewal.duration,true));}
    }
    casting::Hooks hooks(){
        casting::Hooks h;h.events=[this](const auto& packets){feedback.insert(feedback.end(),packets.begin(),packets.end());};h.resources=[this](auto id){return w.resources(id);};
        h.consume_energy=[this](auto n){return w.consume_energy(1,n);};
        h.target_scope=[this](auto id){auto value=w.effect_scope(id);if(value&&stale&&id==target.actor)++value->generation;return value;};
        h.direct_heal_target_valid=[this](auto scope,auto id,auto now){return w.valid_direct_heal_target(source,scope,id,now);};
        h.before_spell_effect=[this](auto id,auto token,auto now){event=token;return w.before_spell_effect(source,id,token,now);};
        h.direct_heal_completed=[this](auto scope,auto id,auto rank,auto divine,auto token,auto now){
            ++attempts;check(token==event);return w.complete_direct_heal(source,scope,id,rank,divine,token,now);};
        h.heal=[](auto,auto,float,bool)->bool{throw std::runtime_error("legacy unscoped heal selected");};
        h.heal_scoped=[](auto,auto,float,bool,auto)->bool{throw std::runtime_error("legacy scoped heal selected");};
        h.zealous_benediction=[](auto,auto,auto,auto)->bool{throw std::runtime_error("legacy Zealous selected");};
        h.has_enchantment=[](auto,auto)->bool{throw std::runtime_error("legacy Light observation selected");};
        h.restore_energy=[](auto)->bool{throw std::runtime_error("legacy Light refund selected");};return h;
    }
    casting::Reason start(casting::Runtime& runtime,casting::Hooks& h){
        skills::Policy policy;policy.primary=6;policy.secondary=3;policy.campaigns=255;policy.learned[skill]=1;
        skills::Slots slots{};slots[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};
        return runtime.start(100,{skill,0,target.actor},slots,policy,ranks,true,h);
    }
    void reject_unknown_scope(casting::Hooks hooks){
        const auto before=w.resources(1)->energy;
        hooks.target_scope=[](auto)->std::optional<effects::Scope>{return {};};
        casting::Runtime rejected(1);check(start(rejected,hooks)==casting::Reason::target);
        check(w.resources(1)->energy==before);
    }
    std::uint64_t finish()const{return 100+std::uint64_t(skills::find_skill(skill)->activation*1000);}
    double target_health()const{return kind==2?w.npc_combat_snapshot(target)->health->current:w.resources(target.actor)->health;}
    unsigned refund()const{return version?effects::ether_renewal(version,15)->energy_per_enchantment:0;}
};
int main(int argc,char** argv)try{
    check(argc>=1&&argc<=3);std::ofstream exported,hex_exported;
    if(argc>=2){exported.open(argv[1]);check(exported.good());}
    if(argc==3){hex_exported.open(argv[2]);check(hex_exported.good());}
    unsigned cases=0;
    for(unsigned skill:{281u,282u,1120u,286u,313u,958u,959u,867u,1686u,1687u})
    for(unsigned version:{0u,181u,2860u})for(unsigned kind=0;kind<3;++kind)for(unsigned mode=0;mode<5;++mode){
        const auto profile=*effects::direct_heal(skill);if(kind==0&&!profile.self)continue;
        ++cases;Fixture f(skill,version,kind);auto h=f.hooks();casting::Runtime runtime(1);
        check(f.start(runtime,h)==casting::Reason::accepted);const auto paid=f.w.resources(1)->energy;
        if(mode==1)h.direct_heal_completed={};if(mode==2)h.direct_heal_target_valid={};
        if(mode==3)runtime.cancel(h);if(mode==4)f.stale=true;runtime.update(f.finish(),h);
        const auto caster_body=casting::skill_success_body_effect(1,skill);
        const auto target_body=(skill==286||skill==1120||skill==958)?
            skills::particles::targeted_body_2022(f.target.actor,1,skill):skills::particles::target_body_2022(f.target.actor,skill);
        if(caster_body)check(std::count(f.feedback.begin(),f.feedback.end(),*caster_body)==unsigned(mode==0));
        if(target_body)check(std::count(f.feedback.begin(),f.feedback.end(),*target_body)==unsigned(mode==0));
        if(mode){check(f.attempts==0&&f.w.resources(1)->health==100&&f.w.resources(1)->energy==paid&&f.target_health()==100);continue;}
        check(f.attempts==1);const auto* row=skills::find_skill(skill);
        const unsigned after=100+(version?20:0),target_before=kind==0?after:100;
        const unsigned amount=row->scale0+(profile.below_half_bonus&&target_before*2<1000?row->bonus0:0);
        check(f.w.resources(1)->health==(kind==0?after+amount:after));check(f.target_health()==target_before+amount);
        const unsigned own=skill==1687?7u:skill==867&&kind==0&&version?row->bonus0:0u;
        check(f.w.resources(1)->energy==paid+f.refund()+own);
        const auto health=f.target_health();check(!f.w.complete_direct_heal(f.source,f.target,skill,0,0,f.event,f.finish()));
        check(f.target_health()==health);
        if(exported.is_open()&&version==0&&kind==1){
            exported<<"{\"skill\":"<<skill<<",\"packets\":[";bool first=true;
            for(const auto& packet:f.feedback){if(!first)exported<<',';first=false;exported<<'\"';
                for(auto byte:packet){constexpr char hex[]="0123456789abcdef";exported<<hex[byte>>4]<<hex[byte&15];}exported<<'\"';}
            exported<<"]}\n";
        }
    }
    check(cases==405);
    unsigned status_cases=0;
    for(unsigned skill:{283u,1396u})for(unsigned version:{0u,181u,2860u})
    for(unsigned kind:{0u,1u,2u})for(unsigned mode=0;mode<5;++mode){
        if(skill==283&&kind==0)continue;
        ++status_cases;
        Fixture f(skill,version,kind);auto h=f.hooks();casting::Runtime runtime(1);
        h.target_valid=[&](auto target,auto range,auto self){return f.w.valid_heal_target(1,target,range,self);};
        h.status_heal_target_valid=[&](auto target,auto id,auto now){return f.w.valid_status_heal_target(f.source,target,id,now);};
        if(kind==2&&skill==283)check(f.w.apply_npc_ether_renewal(f.target,*effects::ether_renewal(181,0),100));
        h.status_heal=[](auto,auto,auto)->bool{throw std::runtime_error("status fallback");};
        h.status_heal_completed=[&](auto target,const auto& plan,auto event,auto now){
            const bool result=f.w.status_heal(f.source,target.actor,plan,now,event,target);
            if(result)check(!f.w.status_heal(f.source,target.actor,plan,now,event,target));
            return result;};
        f.reject_unknown_scope(h);
        check(f.start(runtime,h)==casting::Reason::accepted);const auto paid=f.w.resources(1)->energy;
        if(mode==1)h.status_heal_completed={};if(mode==2)h.before_spell_effect={};
        if(mode==3)runtime.cancel(h);if(mode==4)f.stale=true;
        runtime.update(f.finish(),h);
        const auto amount=effects::status_heal(skill,0,0)->base+(kind==2&&skill==283?effects::status_heal(skill,0,0)->per_status:0);
        check(f.target_health()==100+(mode==0?(kind==0&&version?20:0)+amount:0));
        check(f.w.resources(1)->health==100+(mode==0?(version?20:0)+(kind==0?amount:0):0));
        check(f.w.resources(1)->energy==paid+(mode==0?f.refund():0));
    }

    check(status_cases==75);
    unsigned cleanse_cases=0;
    for(unsigned skill:{275u,276u,277u,278u,1691u,1401u})for(unsigned version:{0u,181u,2860u})
    for(unsigned kind:{0u,1u})for(unsigned mode=0;mode<5;++mode){
        const auto plan=*effects::condition_removal(skill,0,0);if(kind==0&&!plan.self)continue;
        ++cleanse_cases;Fixture f(skill,version,kind);auto h=f.hooks();casting::Runtime runtime(1);
        if(kind==1){check(f.w.party_action(1,158,2,100));check(f.w.party_action(2,154,1,100));}
        check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::blind,100,10000));
        check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::weakness,100,10000));
        h.target_valid=[&](auto target,auto range,auto self){return f.w.valid_heal_target(1,target,range,self);};
        h.remove_conditions=[](auto,auto,auto)->bool{throw std::runtime_error("cleanse fallback");};
        h.remove_conditions_completed=[&](auto target,const auto& effect,auto event,auto now){
            const bool applied=f.w.remove_conditions(f.source,target.actor,effect,now,event,target);
            if(applied)check(!f.w.remove_conditions(f.source,target.actor,effect,now,event,target));return applied;};
        f.reject_unknown_scope(h);
        check(f.start(runtime,h)==casting::Reason::accepted);const auto paid=f.w.resources(1)->energy;
        if(mode==1)h.remove_conditions_completed={};if(mode==2)h.before_spell_effect={};
        if(mode==3)runtime.cancel(h);if(mode==4)f.stale=true;runtime.update(f.finish(),h);
        const auto removed=std::min(2u,plan.count);
        const auto amount=*effects::condition_heal(plan,removed,2-removed,kind==0&&version);
        check(f.target_health()==100+(mode==0?amount+(kind==0&&version?20:0):0));
        check(f.w.resources(1)->health==100+(mode==0?(version?20:0)+(kind==0?amount:0):0));
        check(f.w.resources(1)->energy==paid+(mode==0?f.refund():0));
        check(f.w.has_condition(f.target,combat::conditions::Kind::weakness,f.finish())==(mode!=0));
        check(f.w.has_condition(f.target,combat::conditions::Kind::blind,f.finish())==(mode!=0||removed<2));
    }
    check(cleanse_cases==150);
    unsigned hex_removal_cases=0;
    for(unsigned skill:{301u,2003u})for(unsigned version:{0u,181u,2860u})
    for(unsigned kind:{0u,1u})for(bool hex:{false,true})for(unsigned mode=0;mode<5;++mode){
        ++hex_removal_cases;Fixture f(skill,version,kind);auto h=f.hooks();casting::Runtime runtime(1);
        if(kind==1)check(f.w.party_action(1,158,2,100)&&f.w.party_action(2,154,1,100));
        if(hex)check(f.w.status_mutation(f.target,31,81,0,100,10000,true));
        h.target_valid=[&](auto target,auto range,auto self){return f.w.valid_heal_target(1,target,range,self);};
        h.events=[&](const auto& packets){f.feedback.insert(f.feedback.end(),packets.begin(),packets.end());f.caster_outbox->enqueue(packets);};
        h.remove_effects=[](auto,auto,auto)->bool{throw std::runtime_error("hex removal fallback");};
        h.remove_effects_completed=[&](auto target,const auto& plan,auto event,auto now){
            std::function<void()> publication;
            const bool applied=f.w.remove_effects(f.source,target.actor,plan,now,event,target,&publication);
            if(applied)check(!f.w.remove_effects(f.source,target.actor,plan,now,event,target));
            const auto body=skills::particles::target_body_2022(target.actor,plan.skill);check(bool(body));
            return casting::EffectCompletion{applied,{*body},{},std::move(publication)};};
        f.reject_unknown_scope(h);
        check(f.start(runtime,h)==casting::Reason::accepted);
        check(f.w.consume_energy(1,0,f.source,f.finish()));const auto paid=f.w.resources(1)->energy;
        if(mode==1)h.remove_effects_completed={};if(mode==2)h.before_spell_effect={};
        if(mode==3)runtime.cancel(h);if(mode==4)f.stale=true;
        f.caster_outbox->drain();runtime.update(f.finish(),h);
        const auto amount=hex?effects::removal(skill,0,0)->conditional_heal:0;
        check(f.target_health()==100+(mode==0?amount+(kind==0&&version?20:0):0));
        check(f.w.resources(1)->health==100+(mode==0?(version?20:0)+(kind==0?amount:0):0));
        check(f.w.resources(1)->energy==paid+(mode==0?f.refund():0));
        const auto statuses=*f.w.status_snapshot(f.target,f.finish());
        check(std::count_if(statuses.begin(),statuses.end(),[](const auto& entry){return entry.kind==effects::Category::hex;})==(hex&&mode!=0?1:0));
        const auto packets=f.caster_outbox->drain();const auto body=*skills::particles::target_body_2022(f.target.actor,skill);
        if(hex_exported.is_open()&&version==0&&kind==1&&hex&&mode==0){
            hex_exported<<"{\"skill\":"<<skill<<",\"packets\":[";bool first=true;
            for(const auto& packet:packets){if(!first)hex_exported<<',';first=false;hex_exported<<'\"';
                for(auto byte:packet){constexpr char digits[]="0123456789abcdef";hex_exported<<digits[byte>>4]<<digits[byte&15];}
                hex_exported<<'\"';}
            hex_exported<<"]}\n";
        }

        check(std::count(packets.begin(),packets.end(),body)==unsigned(mode==0));
        if(mode==0){const auto finished=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
            const auto visual=std::find(packets.begin(),packets.end(),body);
            check(finished!=packets.end()&&finished<visual);
            if(hex){const auto removed=std::find(packets.begin(),packets.end(),casting::effect_remove(f.target.actor,81));
                check(removed!=packets.end()&&visual<removed);}}
    }
    check(hex_removal_cases==120);
    unsigned party_cases=0;
    for(unsigned skill:{287u,3232u,1397u,279u,1117u,280u,1119u})for(unsigned version:{0u,181u,2860u})
    for(unsigned mode=0;mode<5;++mode){
        ++party_cases;Fixture f(skill,version,2);f.target=f.source;
        for(unsigned id:{2u,3u}){f.w.join(id,"Recipient",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?1:2);
            f.w.set_vitals(id,{100,1000,50,100},100);}
        check(f.w.party_action(1,158,2,100)&&f.w.party_action(2,154,1,100));
        auto h=f.hooks();casting::Runtime runtime(1);
        h.target_valid=[&](auto target,auto range,auto self){return f.w.valid_heal_target(1,target,range,self);};
        h.party_heal=[](auto,auto)->bool{throw std::runtime_error("party fallback");};
        h.party_heal_completed=[&](const auto& plan,auto event,auto now){
            const bool result=f.w.party_heal(f.source,plan,now,event);
            if(result)check(!f.w.party_heal(f.source,plan,now,event));return result;};
        f.reject_unknown_scope(h);
        check(f.start(runtime,h)==casting::Reason::accepted);// Settle ordinary regeneration independently of the completion reward.
        check(f.w.consume_energy(1,0,f.source,f.finish()));
        const auto paid=f.w.resources(1)->energy;
        if(mode==1)h.party_heal_completed={};if(mode==2)h.before_spell_effect={};
        if(mode==3)runtime.cancel(h);if(mode==4)f.stale=true;runtime.update(f.finish(),h);
        const auto plan=*effects::party_heal(skill,0,0);const bool applied=mode==0;
        check(f.w.resources(1)->health==100+(applied?plan.base_heal+(version?20:0):0));
        if(f.w.resources(1)->energy!=paid+(applied?f.refund():0))throw std::runtime_error("party Energy skill="+std::to_string(skill)+" version="+std::to_string(version)+" mode="+std::to_string(mode)+" actual="+std::to_string(f.w.resources(1)->energy)+" expected="+std::to_string(paid+(applied?f.refund():0)));
        check(f.w.resources(2)->health==100+(applied?plan.base_heal:0));
        check(f.w.resources(3)->health==100+(applied&&plan.all_creatures?plan.base_heal:0));
        check(f.w.npc_combat_snapshot(*f.w.effect_scope(100))->health->current==100+(applied&&plan.all_creatures?plan.base_heal:0));
    }
    check(party_cases==105);
    for(unsigned skill:{280u,1119u})for(unsigned mode=0;mode<4;++mode){
        Fixture f(skill,181,2,100,mode!=2,mode==1?167.f:166.f,mode==3?1u:0u);
        const auto plan=*effects::party_heal(skill,0,0);
        check(f.w.before_spell_effect(f.source,skill,91,100));
        check(f.w.party_heal(f.source,plan,100,91));
        check(f.w.npc_combat_snapshot(f.target)->health->current==100+(mode==0||mode==3?plan.base_heal:0));
        check(f.w.resources(1)->health==120+plan.base_heal);
        check(f.w.resources(1)->energy==50+f.refund());
        check(!f.w.party_heal(f.source,plan,100,91));
    }

    unsigned owned_party_cases=0;
    for(unsigned skill:{287u,3232u,1397u,279u,1117u})for(unsigned version:{0u,181u,2860u})
    for(unsigned mode=0;mode<12;++mode){
        ++owned_party_cases;const auto plan=*effects::party_heal(skill,0,0);
        Fixture f(skill,version,2,100,mode!=7,mode==9?plan.range+1:60.f,mode==8?1u:0u);
        for(unsigned id:{2u,3u}){f.w.join(id,"Owner",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            f.w.set_vitals(id,{100,1000,50,100},100);}
        check(f.w.party_action(1,158,2,100)&&f.w.party_action(2,154,1,100));
        auto owner=*f.w.effect_scope(mode==0?1:mode==2?3:2);
        if(mode==10){auto stale=owner;++stale.generation;check(!f.w.set_npc_party_owner(f.target,stale));}
        else if(mode==11){auto stale=f.target;++stale.generation;check(!f.w.set_npc_party_owner(stale,owner));}
        else check(f.w.set_npc_party_owner(f.target,owner));
        if(mode==3)check(f.w.set_npc_party_owner(f.target,{}));
        if(mode==4)check(f.w.party_action(2,160,0,100));
        if(mode==5){f.w.leave(2);f.w.join(2,"Replacement owner",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            f.w.set_vitals(2,{100,1000,50,100},100);check(*f.w.effect_scope(2)!=owner);}
        if(mode==6)check(f.w.kill(owner,100));
        auto h=f.hooks();casting::Runtime runtime(1);
        h.target_valid=[&](auto target,auto range,auto self){return f.w.valid_heal_target(1,target,range,self);};
        h.party_heal=[](auto,auto)->bool{throw std::runtime_error("owned party fallback");};
        h.party_heal_completed=[&](const auto& effect,auto event,auto now){return f.w.party_heal(f.source,effect,now,event);};
        check(f.start(runtime,h)==casting::Reason::accepted);
        check(f.w.consume_energy(1,0,f.source,f.finish()));const auto paid=f.w.resources(1)->energy;
        runtime.update(f.finish(),h);
        const bool included=mode==0||mode==1||mode==6||mode==8;
        check(f.w.npc_combat_snapshot(f.target)->health->current==100+(included?plan.base_heal:0));
        check(f.w.resources(1)->health==100+plan.base_heal+(version?20:0));
        check(f.w.resources(1)->energy==paid+f.refund());
    }
    check(owned_party_cases==180);
    // Disease transfers through the normal World tick to a typed friendly NPC.
    // Complete at that boundary so the conditional bonus can be isolated from
    // subsequent degeneration without a test-only condition mutation API.
    for(unsigned version:{0u,181u,2860u}){
        Fixture f(1396,version,2);
        f.w.join(2,"Infected human",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        f.w.set_vitals(2,{1000,1000,50,50},100);
        check(f.w.apply_condition(f.source,*f.w.effect_scope(2),combat::conditions::Kind::disease,100,10000));
        f.w.tick(200);f.w.tick(300);
        check(f.w.has_condition(f.target,combat::conditions::Kind::disease,300));
        const auto before=f.target_health();const auto energy=f.w.resources(1)->energy;
        const auto plan=*effects::status_heal(1396,0,0);
        check(f.w.before_spell_effect(f.source,1396,91,300));
        check(f.w.status_heal(f.source,f.target.actor,plan,300,91,f.target));
        check(f.target_health()==before+plan.base+plan.per_status);
        check(f.w.has_condition(f.target,combat::conditions::Kind::disease,300));
        check(f.w.resources(1)->energy==energy+f.refund());
        check(!f.w.status_heal(f.source,f.target.actor,plan,300,91,f.target));
        check(f.target_health()==before+plan.base+plan.per_status);
    }
    // Self casts crossing half Health through Renewal change both Word's
    // bonus and Zealous's refund before the spell's own heal is applied.
    for(unsigned skill:{282u,1687u})for(unsigned version:{0u,181u,2860u}){
        Fixture f(skill,version,0,490);auto h=f.hooks();casting::Runtime runtime(1);
        check(f.start(runtime,h)==casting::Reason::accepted);const auto paid=f.w.resources(1)->energy;
        runtime.update(f.finish(),h);const auto* row=skills::find_skill(skill);
        const unsigned before=490+(version?20:0);
        const unsigned amount=row->scale0+(skill==282&&before*2<1000?row->bonus0:0);
        check(f.w.resources(1)->health==before+amount);
        check(f.w.resources(1)->energy==paid+f.refund()+(skill==1687&&before*2<1000?7:0));
    }
    for(unsigned kind:{1u,2u})for(unsigned version:{0u,181u,2860u}){
        Fixture f(867,version,kind);
        if(kind==1)check(f.w.status_mutation(f.target,180,81,0,100,60100,true));
        else check(f.w.apply_npc_ether_renewal(f.target,*effects::ether_renewal(181,0),100));
        auto h=f.hooks();casting::Runtime runtime(1);check(f.start(runtime,h)==casting::Reason::accepted);
        const auto paid=f.w.resources(1)->energy;runtime.update(f.finish(),h);
        check(f.w.resources(1)->energy==paid+f.refund()+skills::find_skill(867)->bonus0);
    }
    for(unsigned skill:{281u,282u,867u,1687u}){
        Fixture f(skill,181,2,100,false);auto h=f.hooks();casting::Runtime runtime(1);
        check(f.start(runtime,h)==casting::Reason::accepted);const auto paid=f.w.resources(1)->energy;runtime.update(f.finish(),h);
        check(f.attempts==1&&f.w.resources(1)->health==100&&f.w.resources(1)->energy==paid&&f.target_health()==100);
    }
    {Fixture f(281,181,1);check(f.w.before_spell_effect(f.source,281,1,100));
        check(!f.w.complete_direct_heal(f.source,f.target,281,0,1,1,100)); //Secondary Monk has no Divine Favor.
        check(f.w.resources(1)->health==100&&f.target_health()==100);
        check(f.w.complete_direct_heal(f.source,f.target,281,0,0,1,100));}
    for(unsigned skill:{313u,286u}){Fixture f(skill,0,2);f.w.public_professions(1,3,6);
        check(f.w.before_spell_effect(f.source,skill,1,100));check(f.w.complete_direct_heal(f.source,f.target,skill,12,10,1,100));
        const auto* row=skills::find_skill(skill);const auto profile=*effects::direct_heal(skill);
        check(f.target_health()==100+effects::heal_scale(row->scale0,row->scale15,12)+32*profile.divine_multiplier);}
    for(unsigned skill:{282u,1687u}){Fixture f(skill,0,0,999);
        f.w.join(2,"Setup ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);f.w.set_vitals(2,{100,1000,50,100},100);
        check(f.w.infuse_health(f.source,*f.w.effect_scope(2),*effects::infuse_health(0),100));
        check(f.w.resources(1)->health==500); //Precise499.5;UI is rounded upward.
        check(f.w.before_spell_effect(f.source,skill,1,100));check(f.w.complete_direct_heal(f.source,f.source,skill,0,0,1,100));
        const auto* row=skills::find_skill(skill);
        check(f.w.resources(1)->health==500+row->scale0+(skill==282?row->bonus0:0));
        check(f.w.resources(1)->energy==50+(skill==1687?7:0));}
    {Fixture f(281,181,1);check(f.w.before_spell_effect(f.source,281,1,100));f.w.tick(1000);
        check(f.w.valid_direct_heal_target(f.source,f.target,281,100));
        check(f.w.complete_direct_heal(f.source,f.target,281,0,0,1,100));
        check(f.w.resources(1)->health==120&&f.target_health()==100+skills::find_skill(281)->scale0);
        check(!f.w.complete_direct_heal(f.source,f.target,281,0,0,1,100));}
    {Fixture f(281,0,1);check(f.w.status_mutation(f.source,181,80,0,100,5100,true));
        check(f.w.before_spell_effect(f.source,281,1,100));f.w.tick(5200);
        check(f.w.complete_direct_heal(f.source,f.target,281,0,0,1,100));
        check(f.w.resources(1)->health==100); //Expired Renewal does not reward a delayed effect.
        check(f.target_health()==100+skills::find_skill(281)->scale0);}
    {Fixture f(281,181,1);check(f.w.before_spell_effect(f.source,281,1,200));
        check(!f.w.complete_direct_heal(f.source,f.target,281,0,0,1,150));
        check(f.w.resources(1)->health==100&&f.target_health()==100);
        check(f.w.complete_direct_heal(f.source,f.target,281,0,0,1,200));}
    const auto below=effects::direct_heal_amount_precise(282,0,0,499.5,1000);
    const auto half=effects::direct_heal_amount_precise(282,0,0,500,1000);
    check(below&&half&&*below==*half+skills::find_skill(282)->bonus0);
    check(!effects::direct_heal_amount_precise(282,0,0,std::numeric_limits<double>::quiet_NaN(),1000));
    check(!effects::direct_heal_amount_precise(282,0,0,1000.1,1000));
    std::cout<<"PASS405 direct-heal casts,75 status-heal casts,150 cleansing casts,120 hex-removal casts,105 party casts,180 owned-NPC party casts,3 conditioned-NPC World completions and edge cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

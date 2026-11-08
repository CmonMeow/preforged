#include "gw_mimicry_commit.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
unsigned scenario=0;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Mimicry cast mode "+std::to_string(scenario)+" line "+std::to_string(at.line()));
}
int main()try{
    for(scenario=0;scenario<19;++scenario){
        world::Instance w;
        const auto join=[&](unsigned id){w.join(id,"Actor",id==1?0:scenario==13?2000:600,0,
            std::make_shared<world::Outbox>(),0,{},0,100,id==2&&scenario==7?2:1);
            w.set_vitals(id,{1000,1000,id==1&&scenario==9?14u:100u,100},100);};
        join(1);join(2);const auto owner=*w.effect_scope(1);
        check(w.energy_capacity(1,5,0,100,{80,-2}));
        skills::Slots bar{},donor{};bar[0]={65,0};donor[0]={5,17};
        if(scenario==1)donor[0]={194,0};if(scenario==2){donor[0]={1518,0};donor[1]={5,17};}
        check(w.configure_adrenaline(owner,scenario==15?skills::Slots{}:bar,100));
        check(w.configure_adrenaline(*w.effect_scope(2),donor,100));
        skills::effects::TemporarySkillSession session(owner,bar);skills::effects::TemporarySkillIds ids(owner);
        skills::casting::Runtime runtime(1);skills::casting::Hooks h;
        unsigned paid=0,completed=0,publications=0;std::vector<skills::Packet> packets;std::optional<std::uint64_t> base;
        h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.current_skillbar=[&](){return std::optional{session.bar()};};
        h.target_valid=[](auto,float,bool){return false;};
        h.consume_energy_at=[&](unsigned amount,auto at){check(amount==15);const auto ok=w.consume_energy(1,amount,owner,at);if(ok)++paid;return ok;};
        h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(owner,skill,event,at);};
        h.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(owner,slot,at);};
        h.prepare_skill_use=[&](auto slot,auto event,auto at){return w.prepare_diversion_skill_use(owner,slot,event,at);};
        h.abort_skill_use=[&](auto event){return w.abort_diversion_skill_use(owner,event);};
        h.finalize_skill_use=[&](auto event,auto at,auto milliseconds,auto existing,bool instant){
            check(!instant);base=milliseconds;const auto result=w.finalize_diversion_skill_use(owner,event,at,milliseconds,existing,instant);check(bool(result));return result;};
        h.cooldown_completion_source=[&](auto event){return w.cooldown_completion_stamp(owner,event);};
        h.mimicry_target_valid=[&](auto slot,auto target,auto at){return bool(w.prepare_mimicry_donor(owner,target,slot,at));};
        h.mimicry=[&](auto slot,auto target,auto at)->skills::casting::MimicryCompletion{
            ++completed;const auto offer=w.prepare_mimicry_donor(owner,target,slot,at);if(!offer)return {};
            if(offer->donor().status!=skills::effects::MimicrySelection::copy)return {{true},false};
            const auto copied=skills::effects::commit_mimicry_copy(w,session,ids,*offer);
            skills::casting::MimicryCompletion result{{bool(copied),copied?copied->packets:std::vector<skills::Packet>{}},bool(copied)};
            if(scenario==16&&copied){
                result.effect.visuals_before_publication=true;
                result.effect.publication=[&]{++publications;
                    check(std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1))!=packets.end());
                    check(std::find(packets.begin(),packets.end(),skills::slot_packet_2022(1,0,{5,1}))!=packets.end());};
            }
            if(scenario==17&&copied){
                h.events({skills::casting::terminal(58,1),skills::casting::generic(21,1,117)});
                result.effect.completion_prefix_published=true;
            }
            return result;
        };
        if(scenario==8)h.mimicry={};if(scenario==14)h.mimicry_target_valid={};
        skills::Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[65]=1;attributes::Ranks ranks{};
        using Reason=skills::casting::Reason;
        const auto result=runtime.start(100,{65,0,scenario==6?1u:2u},session.bar(),policy,ranks,true,h);
        const auto expected=scenario==8||scenario==14?Reason::unsupported:scenario==9?Reason::energy:
            scenario==6||scenario==7||scenario==13||scenario==15?Reason::target:Reason::accepted;
        check(result==expected);
        if(expected!=Reason::accepted){check(paid==0&&completed==0&&session.bar()==bar);continue;}
        check(paid==1&&w.resources(1)->energy==85);
        runtime.update(2099,h);check(completed==0);
        if(scenario==3)runtime.cancel(h);if(scenario==4)runtime.interrupt(2099,h);
        if(scenario==5)check(w.kill(*w.effect_scope(2),2099));
        if(scenario==10)h.mimicry={};
        if(scenario==18)h.mimicry_target_valid={};
        if(scenario==11){donor={};donor[0]={194,0};check(w.configure_adrenaline(*w.effect_scope(2),donor,2099));}
        if(scenario==12){w.leave(2);join(2);}
        runtime.update(2100,h);runtime.update(2100,h);check(!runtime.active()&&paid==1);
        const bool finished=scenario==0||scenario==1||scenario==2||scenario==11||scenario==16||scenario==17;
        check(completed==unsigned(finished));
        if(!finished){check(session.bar()==bar);continue;}
        check(std::count(packets.begin(),packets.end(),skills::casting::terminal(58,1))==1);
        check(std::count(packets.begin(),packets.end(),skills::casting::generic(21,1,117))==1);
        const auto target_body=*skills::particles::targeted_body_2022(2,1,65);
        check(std::count(packets.begin(),packets.end(),target_body)==1);
        check(std::find(packets.begin(),packets.end(),skills::casting::generic(21,1,117))<std::find(packets.begin(),packets.end(),target_body));
        if(scenario==0||scenario==16||scenario==17){
            check(session.bar()[0]==skills::Slot{5,1}&&base==0);
            const auto replacement=skills::slot_packet_2022(1,0,{5,1});
            check(std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1))<std::find(packets.begin(),packets.end(),replacement));
            check(std::count(packets.begin(),packets.end(),skills::casting::slot_event(228,1,{65,0},60))==0);
        }else{
            check(session.bar()==bar&&base==60000);
            check(std::count(packets.begin(),packets.end(),skills::casting::slot_event(228,1,{65,0},60))==1);
            check(runtime.start(62099,{65,0,2},session.bar(),policy,ranks,true,h)==Reason::recharge);
        }
        check(publications==unsigned(scenario==16));
    }
    std::cout<<"19 staged Mimicry Runtime/World payment,completion and denial cases passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

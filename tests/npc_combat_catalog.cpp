#include "gw_npc_combat_catalog.h"
#include "gw_npc_catalog.h"
#include "gw_power_block_disables.h"
#include <iostream>
#include <functional>
#include <fstream>
#include <map>
#include <sstream>

using namespace fw::gw::combat;
using namespace fw::gw::combat::npcs;
namespace npc_detail=fw::gw::combat::npcs::detail;
namespace {
const std::string hash(64,'a'),other_hash(64,'b');
const std::string header="gw-npc-combat 1 "+hash+"\n";
// Authored, synthetic fixture content only. These are not retail NPC values,
// captured enemies or a production population proposal.
const std::string sources=
    "source relation authored "+hash+" tests/synthetic-relation\n"
    "source health authored "+hash+" tests/synthetic-health\n"
    "source armor authored "+hash+" tests/synthetic-armor\n"
    "source condition authored "+hash+" \"tests/synthetic condition policy\"\n"
    "source allegiance capture "+hash+" tests/synthetic-packed-word\n";
const std::string foe="npc 160 10001 1 1234 5678 0 8 8 hostile-to-players - 240 240 80 1 1 relation health armor - condition\n";
const std::string unknown="npc 160 10002 2 1000 0 0 8 20 unknown - - - - 0 0 - - - - -\n";
const std::string protected_row="npc 160 10003 3 2000 0 0 8 20 protected - 480 480 - 0 0 relation health - - -\n";
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
void rejected(const std::function<void()>& operation,const char* text){
    bool failed=false;try{operation();}catch(const std::invalid_argument&){failed=true;}require(failed,text);
}
Catalog load(const std::string& text){std::istringstream input(text);return load_catalog(input,hash);}
std::string replace(std::string text,std::string_view from,std::string_view to){
    const auto at=text.find(from);require(at!=text.npos,"test replacement pattern missing");text.replace(at,from.size(),to);return text;
}
}
int main(int argc,char** argv)try{
    {
        namespace ai=fw::gw::combat::ai;using fw::gw::skills::Slot;
        const ai::SpellTarget martial{{1,2,1},100,true,true,true,true,true,true,false};
        auto caster=martial;caster.martial_weapon=false;caster.spellcasting_weapon=true;
        ai::Spellbook priorities({{{28,0},1248,"synthetic reviewed Backfire"},{{194,0},1248,"synthetic reviewed Flare"}});
        require(priorities.choose(caster,15,1000)==Slot{28,0},"Backfire spellcasting weapon policy failed");
        require(priorities.choose(martial,15,1000)==Slot{194,0},"Backfire targeted martial weapon");
        auto wire_target=caster;wire_target.scope.actor=65535;
        require(priorities.choose(wire_target,15,1000)==Slot{28,0},"maximum wire actor rejected");
        for(const unsigned invalid_actor:{0u,65536u,UINT32_MAX}){
            wire_target.scope.actor=invalid_actor;
            require(!ai::spell_target(28,wire_target)&&!ai::spell_target(194,wire_target)&&
                !priorities.choose(wire_target,15,1000)&&!priorities.eligible({194,0},wire_target,15,1000),
                "invalid wire actor admitted by primary or fallback spell");
        }
        const ai::Spellbook::Availability disabled_first=[](Slot slot,std::uint64_t)->std::optional<std::uint64_t>{
            return slot.skill==28?5000:0;
        };
        require(priorities.choose(caster,15,1000,disabled_first)==Slot{194,0},"externally disabled first spell starved ready fallback");
        require(priorities.choose(caster,15,5000,disabled_first)==Slot{28,0},"external disable expiry failed exact boundary");
        require(priorities.eligible({194,0},caster,15,1000,disabled_first)&&
            !priorities.eligible({28,0},caster,15,1000,disabled_first),"exact-slot admission disagrees with external readiness");
        const ai::Spellbook::Availability unknown_first=[](Slot slot,std::uint64_t)->std::optional<std::uint64_t>{
            return slot.skill==28?std::nullopt:std::optional<std::uint64_t>{0};
        };
        require(priorities.choose(caster,15,1000,unknown_first)==Slot{194,0},"unknown external state fabricated first-slot readiness");
        const ai::Spellbook::Availability unknown_all=[](Slot,std::uint64_t)->std::optional<std::uint64_t>{return {};};
        require(!priorities.choose(caster,15,1000,unknown_all),"unknown external timers admitted AI spell");
        require(!priorities.eligible({194,1},caster,15,1000),"unowned skill copy admitted as fallback");
        ai::Spellbook copies({{{194,3},1248,"synthetic copy three"},{{194,4},1248,"synthetic copy four"}});
        const ai::Spellbook::Availability copy_gate=[](Slot slot,std::uint64_t)->std::optional<std::uint64_t>{return slot.copy==3?5000:0;};
        require(copies.choose(caster,15,1000,copy_gate)==Slot{194,4},"external disable collapsed distinct skill copies");
        const fw::gw::skills::effects::Scope npc_owner{1,10001,1},mesmer{1,2,1};
        fw::gw::skills::effects::PowerBlockDisables power(npc_owner);
        const auto block=*fw::gw::skills::effects::power_block(12);
        require(power.apply(npc_owner,mesmer,block,28,1000),"Power Block AI fallback fixture failed");
        const ai::Spellbook::Availability power_ready=[&](Slot slot,std::uint64_t at){return power.disabled_until(npc_owner,slot,at);};
        require(priorities.choose(caster,15,1000,power_ready)==Slot{194,0},"Power Block disabled attribute stalled usable Fire Magic spell");
        require(priorities.choose(caster,15,11000,power_ready)==Slot{28,0},"Power Block expiry did not restore reviewed priority");
        const auto reset=power.propose_reset_slots(npc_owner,{{28,0}},1001);
        require(reset.has_value(),"Power Block exact AI slot reset rejected");power=*reset;
        require(priorities.choose(caster,15,1001,power_ready)==Slot{28,0},"reset NPC slot remained skipped by AI selection");
        require(!priorities.choose(caster,{},1000)&&!priorities.choose(caster,4,1000),"unknown/insufficient Energy admitted NPC spell");
        auto unknown=caster;unknown.martial_weapon.reset();unknown.spellcasting_weapon.reset();
        require(!ai::spell_target(28,unknown),"unknown weapon fabricated as spellcasting weapon");
        auto nonmartial=caster;nonmartial.spellcasting_weapon.reset();
        require(priorities.choose(nonmartial,15,1000)==Slot{194,0}&&
            !priorities.eligible({28,0},nonmartial,15,1000),"non-martial inferred spellcasting weapon");
        nonmartial.spellcasting_weapon=false;
        require(priorities.choose(nonmartial,15,1000)==Slot{194,0},"unarmed/other target admitted Backfire");
        auto conflicting=caster;conflicting.martial_weapon=true;
        require(priorities.choose(conflicting,15,1000)==Slot{194,0},"contradictory weapon observations admitted Backfire");
        auto observed=caster;observed.martial_weapon.reset();
        require(priorities.choose(observed,15,1000)==Slot{28,0},"positive spellcasting weapon observation rejected");
        auto hard=martial;hard.hard_mode=true;hard.martial_primary=false;
        require(!ai::spell_target(220,hard)&&ai::spell_target(220,martial),"Blinding Flash mode/profession policy failed");
        hard.martial_primary=true;require(ai::spell_target(220,hard),"hard-mode martial target rejected");
        hard.hard_mode.reset();require(!ai::spell_target(220,hard),"unknown mode invented for Blind targeting");
        ai::Spellbook earth({{{167,0},1248,"Foe_behavior oldid2656731 Eruption"},{{194,0},1248,"synthetic fallback"}});
        for(const auto mode:{std::optional<bool>{false},std::optional<bool>{true},std::optional<bool>{}}){
            auto foe=martial;foe.hard_mode=mode;foe.martial_primary=false;
            require(earth.choose(foe,25,1000)==Slot{167,0},"Eruption incorrectly required martial primary or known mode");
            foe.martial_weapon=false;
            require(earth.choose(foe,25,1000)==Slot{194,0}&&!earth.eligible({167,0},foe,25,1000),
                "Eruption targeted spellcasting weapon or blocked fallback");
            foe.martial_weapon.reset();
            require(earth.choose(foe,25,1000)==Slot{194,0},"Eruption fabricated unknown weapon");
        }
        rejected([]{ai::Spellbook book({{{226,0},1248,"unsupported overcast payment"}});},"unsupported overcast NPC spell admitted");
        rejected([]{ai::Spellbook book({{{194,0},1248,""}});},"unreviewed NPC spell admitted");
        auto definition=*load(header+sources+foe).find(160,10001);
        LiveActor unmeasured(definition,{1,1,20,160,10001});
        require(unmeasured.configure_spellbook(unmeasured.lifetime(),{{{220,0},1248,"synthetic blind"}})&&
            !unmeasured.choose_spell(unmeasured.lifetime(),martial,1000),"unknown NPC Energy invented for AI");
        definition.spawn_energy=Energy{30,30};definition.energy_pips=0;
        LiveActor wire_npc(definition,{1,1,27,160,10001});const auto wire_life=wire_npc.lifetime();
        require(wire_npc.configure_spellbook(wire_life,{{{194,0},1248,"synthetic wire target regression"}}),
            "wire target spellbook rejected");
        for(const unsigned invalid_actor:{0u,65536u,UINT32_MAX}){
            wire_target.scope.actor=invalid_actor;
            require(!wire_npc.begin_spell(wire_life,{194,0},wire_target,1000)&&
                wire_npc.energy(wire_life)->current==30&&!wire_npc.active_action(wire_life,1000),
                "invalid wire actor spent Energy or started action");
        }
        wire_target.scope.actor=65535;
        require(wire_npc.begin_spell(wire_life,{194,0},wire_target,1000).has_value()&&
            wire_npc.energy(wire_life)->current==25,"valid wire boundary failed after rejected targets");
        LiveActor backfire_npc(definition,{1,1,26,160,10001});const auto backfire_life=backfire_npc.lifetime();
        require(backfire_npc.configure_spellbook(backfire_life,{{{28,0},1248,"Foe_behavior oldid2656731 Backfire"},
            {{194,0},1248,"synthetic fallback"}}),"Backfire spellbook rejected");
        require(!backfire_npc.begin_spell(backfire_life,{28,0},nonmartial,1000)&&
            backfire_npc.energy(backfire_life)->current==30,"non-spellcasting Backfire spent Energy or started action");
        require(backfire_npc.choose_spell(backfire_life,nonmartial,1000)==Slot{194,0},"Backfire weapon rejection stalled fallback");
        require(backfire_npc.begin_spell(backfire_life,{28,0},caster,1000).has_value()&&
            backfire_npc.energy(backfire_life)->current==15,"positive spellcasting observation did not admit/pay Backfire");
        LiveActor earth_npc(definition,{1,1,25,160,10001});const auto earth_life=earth_npc.lifetime();
        require(earth_npc.configure_spellbook(earth_life,{{{167,0},1248,"Foe_behavior oldid2656731 Eruption"},
            {{194,0},1248,"synthetic fallback"}}),"Eruption spellbook rejected");
        require(!earth_npc.begin_spell(earth_life,{167,0},caster,1000)&&earth_npc.energy(earth_life)->current==30,
            "rejected Eruption spent Energy or started action");
        require(earth_npc.choose_spell(earth_life,caster,1000)==Slot{194,0},"rejected Eruption stalled live NPC fallback");
        auto earth_target=martial;earth_target.hard_mode=true;earth_target.martial_primary=false;
        require(earth_npc.begin_spell(earth_life,{167,0},earth_target,1000).has_value()&&earth_npc.energy(earth_life)->current==5,
            "reviewed Eruption did not admit/pay real25 Energy");
        definition.knockdown_allowed=true;definition.knockdown_evidence="synthetic reviewed knockdown trait";
        LiveActor gaining(definition,{1,1,24,160,10001});const auto gaining_life=gaining.lifetime();
        require(gaining.lose_energy(gaining_life,5)&&gaining.gain_energy(gaining_life,3)&&gaining.energy(gaining_life)->current==28,
            "known NPC Energy gain did not change actual pool");
        require(gaining.gain_energy(gaining_life,UINT32_MAX)&&gaining.energy(gaining_life)->current==30,
            "NPC Energy gain exceeded reviewed maximum");
        require(!unmeasured.gain_energy(unmeasured.lifetime(),5),"NPC Energy gain fabricated unknown pool");
        LiveActor npc(definition,{1,1,21,160,10001});const auto life=npc.lifetime();
        require(npc.configure_spellbook(life,{{{220,0},1248,"synthetic blind"}}),"reviewed spellbook configuration failed");
        const auto cast=npc.begin_spell(life,{220,0},martial,1000);
        require(cast&&cast->completes==1750&&npc.energy(life)->current==20,"AI activation or actual Energy payment wrong");
        require(!npc.begin_spell(life,{220,0},martial,1100)&&npc.energy(life)->current==20,"busy AI spell paid twice");
        require(npc.movement_integral(life,1000,1500)==0,"pending NPC spell allowed movement");
        require(!npc.complete_action(life,*cast,1749)&&npc.complete_action(life,*cast,1750),"NPC spell completion boundary wrong");
        require(!npc.ordinary_action_ready(life,2499)&&!npc.begin_action(life,control::Action::attack,2499,2999),"ordinary fallback bypassed spell aftercast");
        require(npc.ordinary_action_ready(life,2500),"ordinary fallback blocked at aftercast boundary");
        require(npc.movement_integral(life,1750,2499)==0&&npc.movement_integral(life,1750,2500)==0,
            "NPC moved during spell aftercast");
        require(npc.movement_integral(life,2000,2600)==100&&npc.movement_integral(life,2600,2700)==100,
            "coarse movement tick spent blocked aftercast time");
        require(!npc.choose_spell(life,martial,9749)&&npc.choose_spell(life,martial,9750)==Slot{220,0},"AI recharge began before completion");
        const auto second=npc.begin_spell(life,{220,0},martial,9750);require(second.has_value(),"recharged NPC spell rejected");
        require(npc.cancel_action(life,10000)&&!npc.complete_action(life,*second,10500),"interrupted NPC spell completed");
        require(!npc.choose_spell(life,martial,17999)&&npc.choose_spell(life,martial,18000)==Slot{220,0},"interrupted AI spell bypassed full recharge");
        LiveActor voluntary(definition,{1,1,22,160,10001});const auto own=voluntary.lifetime();
        require(voluntary.configure_spellbook(own,{{{220,0},1248,"synthetic blind"}})&&voluntary.begin_spell(own,{220,0},martial,1000).has_value(),"voluntary fixture failed");
        require(voluntary.voluntarily_cancel_spell(own,1200)&&voluntary.energy(own)->current==20&&
            voluntary.choose_spell(own,martial,1200)==Slot{220,0},"voluntary cancel refunded Energy or began recharge");
        LiveActor knocked(definition,{1,1,23,160,10001});const auto down=knocked.lifetime();
        require(knocked.configure_spellbook(down,{{{220,0},1248,"synthetic blind"}})&&knocked.begin_spell(down,{220,0},martial,1000).has_value(),"knockdown fixture failed");
        require(knocked.knock_down(down,1100)==control::Knockdown::accepted&&
            !knocked.choose_spell(down,martial,9099)&&knocked.choose_spell(down,martial,9100)==Slot{220,0},"knockdown AI recharge wrong");
    }
    if(argc!=1&&argc!=4)throw std::invalid_argument("optional population/catalog/SHA256 arguments required together");
    {
        const auto catalog=load(header+sources+foe);const auto definition=*catalog.find(160,10001);
        LiveActor npc(definition,{1,1,1,160,10001});const auto life=npc.lifetime();
        const auto plan=*fw::gw::skills::effects::fragility(19,12);
        require(bool(npc.apply_fragility(life,plan,42,20,90,100)),"NPC Fragility application failed");
        require(npc.apply_condition(life,conditions::Kind::blind,42,200,1000),"Fragility new condition failed");
        require(npc.actor(life)->health->current==223,"new condition did not cause17 damage");
        require(npc.apply_condition(life,conditions::Kind::blind,43,300,1000)&&npc.actor(life)->health->current==223,"condition refresh triggered Fragility");
        require(npc.remove_condition(life,conditions::Kind::blind,400)&&npc.actor(life)->health->current==206,"condition removal missed Fragility");
        auto hits=npc.take_fragility_hits(life);require(hits.size()==2&&hits[0].at==200&&hits[1].at==400&&hits[0].cause==42,"Fragility source/timestamps incorrect");
        require(npc.take_fragility_hits(life).empty(),"Fragility events replayed");
        require(npc.apply_condition(life,conditions::Kind::blind,42,500,1000),"Fragility second condition failed");
        npc.advance_conditions(life,2000);
        require(npc.actor(life)->health->current==172,"natural expiry missed Fragility damage");
        hits=npc.take_fragility_hits(life);require(hits.size()==2&&hits[1].at==1500,"delayed tick moved Fragility expiry damage");
        npc.advance_conditions(life,2100);require(npc.take_fragility_hits(life).empty(),"expired condition triggered twice");
        require(npc.apply_condition(life,conditions::Kind::blind,42,2200,1000),"clear test condition failed");
        require(npc.clear_conditions(life,2300)&&npc.actor(life)->health->current==138,"clear did not cause one condition loss");
        auto stale=life;++stale.actor_generation;require(npc.take_fragility_hits(stale).empty(),"stale lifetime drained reactions");
        npc.take_fragility_hits(life);
        conditions::State donor;require(donor.apply(conditions::Kind::blind,55,2400,1000)&&donor.apply(conditions::Kind::dazed,55,2400,1000),"transfer donor invalid");
        const auto transfer=*fw::gw::skills::effects::condition_transfer(132,12);
        require(bool(npc.receive_conditions(life,transfer,donor,55,2400)),"Fragility transfer receiver failed");
        hits=npc.take_fragility_hits(life);require(hits.size()==2&&hits[0].cause==42&&hits[1].cause==42&&npc.actor(life)->health->current==104,"transferred conditions missed separate Fragility triggers");
        auto durable=definition;durable.spawn_health=Health{1000,1000};LiveActor shocked(durable,{1,1,3,160,10001});
        require(bool(shocked.apply_fragility(shocked.lifetime(),plan,42,20,92,100)),"Shockwave Fragility failed");
        const auto wave=*fw::gw::skills::effects::caster_earth_aoe(937,12);
        const auto result=shocked.apply_earth_aoe(shocked.lifetime(),wave,0,20,55,200);
        require(result&&result->applied_conditions.size()==3,"Shockwave did not apply three conditions");
        require(shocked.actor(shocked.lifetime())->health->current==result->after.actor.health->current,"Shockwave bypassed Fragility reactions");
        require(shocked.take_fragility_hits(shocked.lifetime()).size()==3,"Shockwave condition trigger count incorrect");
        auto weak=definition;weak.spawn_health=Health{10,10};LiveActor dying(weak,{1,1,2,160,10001});
        require(bool(dying.apply_fragility(dying.lifetime(),plan,42,20,91,100)),"lethal test Fragility failed");
        require(dying.apply_condition(dying.lifetime(),conditions::Kind::blind,42,200,1000),"lethal trigger condition failed");
        require(dying.actor(dying.lifetime())->health->current==0&&(dying.actor(dying.lifetime())->effects&0x10),"Fragility did not kill NPC");
        require(dying.take_fragility_hits(dying.lifetime()).size()==1,"lethal Fragility event missing");
    }
    {
        const auto catalog=load(header+sources+foe);auto definition=*catalog.find(160,10001);
        const auto plan=*fw::gw::skills::effects::attack_hex(66,12);
        LiveActor unknown_energy(definition,{1,1,12,160,10001});
        require(!unknown_energy.apply_attack_hex(unknown_energy.lifetime(),plan,42,20,96,100),"unknown NPC energy fabricated for Shackles");
        definition.spawn_energy=Energy{14,20};definition.energy_pips=0;LiveActor npc(definition,{1,1,13,160,10001});const auto life=npc.lifetime();
        require(bool(npc.apply_attack_hex(life,plan,42,20,96,100)),"NPC Shackles application failed");
        for(unsigned i=0;i<4;++i){const auto token=npc.begin_action(life,control::Action::attack,200+i*1000,700+i*1000);require(bool(token),"Shackles blocked attack admission");
            require(npc.energy(life)->current==(i==0?9u:i==1?4u:0u),"Shackles repeated NPC energy loss incorrect");require(npc.complete_action(life,*token,700+i*1000),"Shackles cancelled attack");}
        require(npc.actor(life)->health->current==240,"Shackles incorrectly dealt damage");
    }
    for(unsigned skill:{43u,47u}){
        const auto catalog=load(header+sources+foe);const auto definition=*catalog.find(160,10001);
        LiveActor npc(definition,{1,1,10,160,10001});const auto life=npc.lifetime();
        const auto plan=*fw::gw::skills::effects::attack_hex(skill,12);
        require(bool(npc.apply_attack_hex(life,plan,42,20,93,100)),"attack hex application failed");
        require(!npc.begin_action(life,control::Action::attack,50,500),"pre-application clock admitted attack");
        const auto spell=npc.begin_action(life,control::Action::spell,200,300);
        require(spell&&npc.actor(life)->health->current==240&&npc.take_attack_hex_hits(life).empty(),"spell triggered attack hex");
        require(npc.complete_action(life,*spell,300),"spell completion failed");
        const auto attack=npc.begin_action(life,control::Action::attack,400,900);
        require(bool(attack),"valid attack not admitted");
        require(npc.actor(life)->health->current==240-plan.damage,"attack hex damage incorrect");
        require(npc.has_condition(life,conditions::Kind::blind,400)==(skill==47),"Ineptitude Blindness timing incorrect");
        const auto hits=npc.take_attack_hex_hits(life);require(hits.size()==1&&hits[0].effect==93&&hits[0].cause==42&&hits[0].interrupted==(skill==43),"attack hex event identity incorrect");
        require(npc.complete_action(life,*attack,900)==(skill==47),"Clumsiness interrupt/Ineptitude continued attack incorrect");
        if(skill==43)require(bool(npc.begin_action(life,control::Action::attack,1000,1500)),"subsequent unhexed attack rejected");
        require(npc.take_attack_hex_hits(life).empty(),"consumed attack hex triggered twice");
        if(skill==47){
            auto durable=definition;durable.spawn_health=Health{1000,1000};LiveActor combined(durable,{1,1,11,160,10001});
            require(bool(combined.apply_fragility(combined.lifetime(),*fw::gw::skills::effects::fragility(19,12),44,20,94,100)),"combined Fragility failed");
            require(bool(combined.apply_attack_hex(combined.lifetime(),plan,42,20,95,100)),"combined Ineptitude failed");
            require(bool(combined.begin_action(combined.lifetime(),control::Action::attack,200,700)),"combined admitted attack failed");
            require(combined.actor(combined.lifetime())->health->current==869,"Ineptitude Blindness did not trigger Fragility before damage");
            require(combined.take_fragility_hits(combined.lifetime()).size()==1&&combined.take_attack_hex_hits(combined.lifetime()).size()==1,"combined reactions missing or duplicated");
        }

    }
    const auto empty=load(header);require(!empty.size(),"empty reviewed catalog invented actors");
    {
        auto row=foe;row.pop_back();row+=" - - - - - - 1 - 80 armor\n";
        const auto text="gw-npc-combat 5 "+hash+"\n"+sources+row;
        const auto catalog=load(text);const auto* definition=catalog.find(160,10001);
        require(definition&&definition->lightning_armor==80,"revision5 lightning armor missing");
        LiveActor npc(*definition,{1,1,1,160,10001});const auto life=npc.lifetime();
        const auto hit=npc.apply_damage(life,{Kind::lightning,40,20,230,25},100);
        require(hit.reason==Reason::accepted&&hit.damage==40,"reviewed lightning armor/penetration not applied");
        require(npc.apply_condition(life,conditions::Kind::cracked_armor,1,100,1000),"lightning condition fixture");
        const auto cracked=npc.apply_damage(life,{Kind::lightning,40,20,230,25},101);
        require(cracked.reason==Reason::accepted&&cracked.damage>40,"lightning damage ignores current cracked armor");
        require(npc.actor(life)->lightning_armor==80,"temporary condition overwrote base lightning armor");
        rejected([&]{load(replace(text,"80 armor\n","80 -\n"));},"lightning armor without evidence accepted");
        rejected([&]{load(replace(text,"80 armor\n","80 missing\n"));},"unknown lightning evidence accepted");
        rejected([&]{load(replace(text,"80 armor\n","501 armor\n"));},"out of bounds lightning armor accepted");
        const auto absent=load(replace(text,"80 armor\n","- -\n"));
        LiveActor missing(*absent.find(160,10001),{1,1,1,160,10001});
        require(missing.apply_damage(missing.lifetime(),{Kind::lightning,40,20,230,25},100).reason!=Reason::accepted,"missing lightning armor invented a default");
    }
    {
        auto row=foe;row.pop_back();row+=" 80 80 armor armor 80 armor 2 armor\n";
        const auto text="gw-npc-combat 4 "+hash+"\n"+sources+row;
        const auto vulnerability=load(text);
        const auto* d=vulnerability.find(160,10001);
        require(d&&d->fire_multiplier==2&&d->physical_armor==80,"revision4 lost vulnerability/armor");
        for(const auto invalid:{std::string("0 armor"),std::string("5 armor"),std::string("2 -"),
            std::string("2 missing"),std::string("1 armor")})
            rejected([&]{load(replace(text,"2 armor\n",invalid+"\n"));},"invalid vulnerability accepted");
        require(load(replace(text,"2 armor\n","1 -\n")).find(160,10001)->fire_multiplier==1,
            "explicit normal fire damage rejected");
        const Lifetime life{1,1,1,160,10001};LiveActor fire(*d,life),cold(*d,life);
        const auto hit=fire.apply_damage(life,{Kind::fire,10,20,197},0);
        const auto control=cold.apply_damage(life,{Kind::cold,10,20,846},0);
        require(hit.reason==Reason::accepted&&control.reason==Reason::accepted&&
            std::abs(hit.damage-2*control.damage)<0.000001,"vulnerability affected wrong element");
        auto guardian=*d;guardian.relation=Relation::neutral;
        LiveActor friendly(guardian,life);
        require(friendly.apply_damage(life,{Kind::earth,10,20,174},0).reason!=Reason::accepted,
            "player damage bypassed neutral guardian");
        require(friendly.apply_guardian_damage(life,{Kind::earth,10,20,174},0).reason==Reason::accepted,
            "trusted guardian damage rejected");
        guardian.relation=Relation::protected_ally;guardian.damageable=false;guardian.conditions_allowed=false;
        LiveActor protected_guardian(guardian,life);
        require(protected_guardian.apply_guardian_damage(life,{Kind::earth,10,20,174},0).reason!=Reason::accepted,
            "trusted guardian damage bypassed protected NPC");
    }
    auto catalog=load(header+sources+foe+unknown+protected_row);
    require(catalog.size()==3&&catalog.population_sha256()==hash&&catalog.sources().size()==5,"catalog content lost");
    require(catalog.sources().at("condition").reference=="tests/synthetic condition policy","quoted source reference lost");
    const auto* definition=catalog.find(160,10001);require(definition&&definition->spawn_health&&definition->earth_armor==80,"explicit foe metadata missing");
    const auto fractional=load(header+sources+replace(foe,"240 240 80","240 239.25 80"));
    require(fractional.find(160,10001)->spawn_health->current==239.25,"fractional spawn health rounded to capacity");
    require(!definition->packed_allegiance,"normalized hostility fabricated packed allegiance");
    require(!catalog.find(161,10001)&&!catalog.find(160,9),"lookup crossed map or actor identity");
    const Observed foe_observed{definition->binding,false};
    std::vector<Observed> population{foe_observed,{catalog.find(160,10002)->binding,false},{catalog.find(160,10003)->binding,true}};
    catalog.validate_population(population,hash);
    require(catalog.bind(foe_observed,hash)==definition,"bound metadata not selected");
    auto missing=foe_observed;missing.binding.actor=10004;require(!catalog.bind(missing,hash),"missing metadata invented stats");
    empty.validate_population(population,hash);
    rejected([&]{catalog.validate_population(population,other_hash);},"stale population revision accepted");
    for(unsigned field=0;field<9;++field){
        auto bad=population;
        switch(field){
        case 0:++bad[0].binding.map;break;case 1:++bad[0].binding.actor;break;
        case 2:++bad[0].binding.definition;break;case 3:++bad[0].binding.model_file;break;
        case 4:++bad[0].binding.skin_file;break;case 5:++bad[0].binding.appearance;break;
        case 6:++bad[0].binding.definition_flags;break;case 7:++bad[0].binding.level;break;
        case 8:bad[0].protected_service=true;break;
        }
        rejected([&]{catalog.validate_population(bad,hash);},"mismatched or protected service population accepted");
    }
    auto duplicate=population;duplicate.push_back(population.front());
    rejected([&]{catalog.validate_population(duplicate,hash);},"duplicate observed binding accepted");
    const Lifetime life{90,7,11,160,10001};LiveActor live(*definition,life);
    require(live.matches(life)&&live.hostile_to_players(life)&&!live.packed_allegiance(life),"lifetime or relation missing");
    require(live.actor(life)->health->current==240&&live.actor(life)->effects==0,"fresh state not from explicit spawn health");
    auto stale=life;--stale.actor_generation;
    auto wrong_world=life;++wrong_world.world;
    auto wrong_instance=life;++wrong_instance.instance_generation;
    auto wrong_map=life;++wrong_map.map;
    auto wrong_actor=life;++wrong_actor.actor;
    for(const auto& handle:{stale,wrong_world,wrong_instance,wrong_map,wrong_actor}){
        require(!live.actor(handle)&&!live.hostile_to_players(handle)&&!live.final_earth_armor(handle,1000),"stale handle exposed current state");
        require(!live.apply_condition(handle,conditions::Kind::cracked_armor,7,1000,5000),"stale generation applied condition");
        require(live.apply_damage(handle,{Kind::earth,10,20,194},1000).reason==Reason::invalid,"stale generation applied damage");
        require(!live.expire_conditions(handle,1000)&&!live.clear_conditions(handle),"stale generation cleared current conditions");
    }
    require(live.apply_condition(life,conditions::Kind::cracked_armor,7,1000,5000),"reviewed living condition rejected");
    require(live.final_earth_armor(life,1000)==60&&live.actor(life)->earth_armor==80,"cracked armor modified stored base armor");
    const auto hit=live.apply_damage(life,{Kind::earth,100,20,194},1000);
    require(hit.reason==Reason::accepted&&hit.damage==100&&live.actor(life)->health->current==140&&live.actor(life)->earth_armor==80,"damage ignored current conditions or overwrote base armor");
    require(live.apply_condition(life,conditions::Kind::cracked_armor,8,2000,1),"short reapplication rejected");
    require(live.final_earth_armor(life,5999)==60&&live.final_earth_armor(life,6000)==80,"condition shortened or outlived expiry");
    live.expire_conditions(life,6000);require(live.actor(life)->effects==0,"expired condition left presentation flags");
    require(!live.apply_condition(life,conditions::Kind::blind,65536,7000,1000),"invalid cause actor accepted");
    require(live.apply_condition(life,conditions::Kind::blind,7,7000,1000),"condition policy unexpectedly disabled");
    require(live.clear_conditions(life)&&!live.has_condition(life,conditions::Kind::blind,7000),"condition reset failed");
    const auto lethal=live.apply_damage(life,{Kind::armor_ignoring,1000,20,194},7000);
    require(lethal.killed&&!live.apply_condition(life,conditions::Kind::blind,7,7000,1000),"dead actor received condition");
    require(live.apply_damage(life,{Kind::earth,10,20,194},7000).reason==Reason::dead,"dead actor received repeated damage");
    auto next=life;++next.actor_generation;LiveActor respawn(*definition,next);
    require(!respawn.matches(life)&&respawn.actor(next)->health->current==240&&respawn.actor(next)->effects==0&&
        !respawn.has_condition(next,conditions::Kind::blind,7000),"actor ID reuse retained damage/conditions");
    LiveActor no_stats(*catalog.find(160,10002),{90,7,12,160,10002});
    const auto no_stats_life=no_stats.lifetime();
    require(!no_stats.actor(no_stats_life)->health&&!no_stats.actor(no_stats_life)->earth_armor&&
        !no_stats.actor(no_stats_life)->damageable,"unknown captured NPC derived level-based combat stats");
    require(!no_stats.apply_condition(no_stats_life,conditions::Kind::blind,7,0,1000)&&
        no_stats.apply_damage(no_stats_life,{Kind::earth,10,20,194},0).reason==Reason::not_hostile,"unknown actor became a victim");
    LiveActor ally(*catalog.find(160,10003),{90,7,13,160,10003});
    require(ally.apply_damage(ally.lifetime(),{Kind::earth,100,20,194},0).reason==Reason::not_hostile&&
        !ally.apply_condition(ally.lifetime(),conditions::Kind::blind,7,0,1000),"protected actor received hostile effects");
    auto ignore_definition=*definition;ignore_definition.earth_armor.reset();
    LiveActor armor_unknown(ignore_definition,life);
    require(armor_unknown.apply_damage(life,{Kind::earth,10,20,194},0).reason==Reason::unknown_armor,"missing armor filled from level");
    require(armor_unknown.apply_damage(life,{Kind::armor_ignoring,10,20,194},0).reason==Reason::accepted,"armor-ignore unnecessarily requires armor");
    auto packed_row=replace(foe,"hostile-to-players -","hostile-to-players 305419896");
    rejected([&]{load(header+sources+packed_row);},"packed allegiance accepted without independent evidence");
    packed_row=replace(packed_row,"relation health armor - condition","relation health armor allegiance condition");
    const auto packed=load(header+sources+packed_row);
    LiveActor packed_live(*packed.find(160,10001),life);
    require(packed_live.packed_allegiance(life)==0x12345678u&&
        !packed_live.packed_allegiance(stale),"reviewed full packed word lost or stale word exposed");
    // Revisions6/7/8/9/10 added general armor, energy, regeneration, holy traits,
    // and independently evidenced knockdown susceptibility.
    // A header-only catalog is valid for every recognized revision.
    for(unsigned revision=1;revision<=17;++revision)
        require(load("gw-npc-combat "+std::to_string(revision)+" "+hash+"\n").size()==0,"recognized empty revision rejected");
    unsigned bad_catalog_case=0;
    auto bad_catalog=[&](const std::string& text){++bad_catalog_case;try{(void)load(text);}catch(const std::invalid_argument&){return;}throw std::runtime_error("invalid combat catalog accepted case "+std::to_string(bad_catalog_case));};
    bad_catalog("");bad_catalog("gw-npc-combat 0 "+hash+"\n");bad_catalog("gw-npc-combat 1 "+other_hash+"\n");
    // The new revision adds two mandatory columns; merely changing the header
    // on an old row must not silently imply susceptibility.
    bad_catalog("gw-npc-combat 10 "+hash+"\n"+sources+foe);
    bad_catalog("gw-npc-combat 11 "+hash+"\n"+sources+foe);
    bad_catalog(header+sources+foe+foe);bad_catalog(header+sources+"source health authored "+hash+" duplicate\n");
    bad_catalog(header+"\"");bad_catalog(header+"source ref authored "+hash+" \"unterminated\n");
    bad_catalog(header+sources+foe+"extra\n");bad_catalog(header+std::string(4097,' '));
    for(const auto& [from,to]:std::vector<std::pair<std::string,std::string>>{
        {"npc 160","npc 0"},{"160 10001","160 65536"},{"5678 0 8 8","5678 0 8 256"},
        {"hostile-to-players","enemy3"},{"- 240 240 80","- - 240 80"},{"- 240 240 80","- 240 - 80"},
        {"- 240 240 80","- 0 0 80"},{"- 240 240 80","- 240 241 80"},{"- 240 240 80","- 240 nan 80"},
        {"- 240 240 80","- 240 0 80"},{"- 240 240 80","- 1000001 240 80"},
        {"240 240 80","240 240 501"},{"240 240 80","240 240 -101"},
        {"80 1 1","80 2 1"},{"80 1 1","80 1 2"},
        {"relation health armor - condition","missing health armor - condition"},
        {"relation health armor - condition","relation - armor - condition"},
        {"relation health armor - condition","relation health - - condition"},
        {"relation health armor - condition","relation health armor - -"},
        {"hostile-to-players","protected"}})
        bad_catalog(header+sources+replace(foe,from,to));
    bad_catalog(header+sources+replace(unknown,"unknown","protected"));
    bad_catalog(header+sources+replace(unknown,"0 0 - - - - -","1 0 - - - - -"));
    bad_catalog(header+sources+replace(unknown,"0 0 - - - - -","0 1 - - - - -"));
    bad_catalog(header+sources+replace(unknown,"0 0 - - - - -","0 0 relation - - - -"));
    bad_catalog(header+"source x authored "+std::string(64,'A')+" tests/x\n");
    bad_catalog(header+"source x guessed "+hash+" tests/x\n");
    bad_catalog(header+"source x authored "+hash+" \"tests/control\tpath\"\n");
    for(auto bad_life:{Lifetime{0,7,11,160,10001},Lifetime{90,0,11,160,10001},Lifetime{90,7,0,160,10001},
                      Lifetime{90,7,11,161,10001},Lifetime{90,7,11,160,10002}})
        rejected([&]{LiveActor bad(*definition,bad_life);},"invalid actor lifetime accepted");
    // Parse a complete replacement first; failed RHS assignment leaves the
    // installed catalog and current dynamic actor health/conditions untouched.
    const auto before_health=respawn.actor(next)->health->current;
    rejected([&]{catalog=load(header+sources+foe+foe);},"bad replacement was published");
    require(catalog.size()==3&&respawn.actor(next)->health->current==before_health,"failed replacement mutated installed state");
    if(argc==4){
        std::ifstream population_file(argv[1]),metadata_file(argv[2]);
        require(bool(population_file)&&bool(metadata_file),"production NPC fixture missing");
        const auto production=load_catalog(metadata_file,argv[3]);
        const auto appearance=fw::gw::world::load_npc_catalog(population_file);
        const auto installed=fw::gw::world::combat_population(appearance);
        production.validate_population(installed,argv[3]);
        require(installed.size()==1402&&production.size()==1402,"reviewed relation-only population coverage changed");
        // The content hash passed to load/validate binds all 1402 definitions.
        // Pin the previous eight populations independently: added maps must not
        // silently alter reviewed identities, encoded names, placement or appearance.
        std::ifstream identity_file(argv[1]);std::map<unsigned,std::string> identity_rows;
        for(std::string line;std::getline(identity_file,line);){
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.rfind("npc ",0)!=0)continue;
            std::istringstream fields(line);std::string tag;unsigned map=0;fields>>tag>>map;
            identity_rows[map]+=line+"\n";
        }
        require(identity_rows.size()==115,"populated map coverage changed");
        const std::map<unsigned,std::uint64_t> preserved_hashes={
            {55,8079701219649172952ULL},
            {148,14495779798750653560ULL},
            {163,1818198357166068969ULL},
            {164,15026836618277478396ULL},
            {165,562656784672579064ULL},
            {166,13764176837997734877ULL},
            {248,8593423624164572577ULL},
            {360,71830735303082943ULL},
        };
        for(const auto& [map,expected]:preserved_hashes){
            std::uint64_t actual=14695981039346656037ULL;
            for(unsigned char byte:identity_rows.at(map))actual=(actual^byte)*1099511628211ULL;
            require(actual==expected,"previously installed population identity changed");
        }
        unsigned protected_count=0,neutral_count=0,unknown_count=0;
        for(const auto& observed:installed){
            const auto* d=production.bind(observed,argv[3]);require(d,"reviewed actor missing from production combat metadata");
            require(!d->spawn_health&&!d->earth_armor&&!d->packed_allegiance&&!d->damageable&&!d->conditions_allowed,
                "uncaptured combat stats, packed faction or effects fabricated for friendly NPC");
            protected_count+=d->relation==Relation::protected_ally;neutral_count+=d->relation==Relation::neutral;unknown_count+=d->relation==Relation::unknown;
            LiveActor runtime(*d,{1,1,1,d->binding.map,d->binding.actor});
            require(!runtime.hostile_to_players(runtime.lifetime()),"friendly captured actor became a foe");
        }
        require(protected_count==388&&neutral_count==727&&unknown_count==287,"observed relation mapping changed");
        std::cout<<"PASS actual1402 captured actor bindings across115 maps,388 protected/727 neutral/287 unknown, no fabricated combat values\n";
    }
    std::cout<<"PASS reviewed NPC combat metadata, strict population/evidence binding, service protection, unknown stats, condition expiry, atomic replacement and generation isolation\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

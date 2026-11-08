#include "gw_npc_combat_catalog.h"
#include "gw_defensive_ground_ward.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace combat::npcs;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const skills::effects::Scope scope{1,100,2};control::State state(scope);
    const auto attack=state.begin(scope,control::Action::attack,100,600);
    check(attack&&state.attacking(100)&&state.attacking(599)&&!state.attacking(600),"explicit windup boundaries");
    check(!state.active(99)&&state.active(100)==attack&&state.active(599)==attack&&!state.active(600),"read-only current action interval");
    check(!state.complete(*attack,599),"premature strike");
    check(state.knock_down(scope,{},200,2000)==control::Knockdown::unknown&&state.epoch()==0,"unknown is not susceptible");
    check(state.knock_down(scope,false,200,2000)==control::Knockdown::immune&&state.attacking(200),"immune keeps action");
    check(state.knock_down(scope,true,200,2000)==control::Knockdown::accepted,"knockdown");
    check(!state.active(200),"cancelled action not observable");
    check(!state.attacking(200)&&!state.complete(*attack,5000),"late polling revived interrupted attack");
    check(state.knock_down(scope,true,2199,2000)==control::Knockdown::already_down,"re-knockdown while down");
    check(!state.begin(scope,control::Action::spell,300,400),"spell while down");
    const auto spell=state.begin(scope,control::Action::spell,2200,2500);
    check(spell&&state.complete(*spell,2600)&&!state.complete(*spell,2600),"recovery and one-shot completion");
    control::State spirit_state(scope);
    const auto spirit_attack=spirit_state.begin(scope,control::Action::attack,2700,4700,skills::Slot{3153,0});
    check(spirit_attack&&spirit_attack->skill==skills::Slot{3153,0}&&spirit_state.attacking(2700),"typed spirit attack windup");
    check(!spirit_state.complete(*spirit_attack,4699)&&spirit_state.complete(*spirit_attack,4700),"typed attack completion boundary");
    check(!spirit_state.begin(scope,control::Action::attack,4800,6800,skills::Slot{1239,0}),"spell cannot masquerade as typed attack");
    check(state.knock_down(scope,true,2500,2000)==control::Knockdown::invalid,"backdated knockdown");
    auto stale=scope;++stale.generation;
    check(!state.begin(stale,control::Action::attack,3000,3500)&&!state.cancel(stale,3000),"reused actor ID");
    skills::effects::MovementEffects movement;
    movement.apply({1,42,76,0,skills::effects::Category::hex,100,3100});
    check(state.movement_integral(movement,100,4100)==1500,"pause excludes slowed travel and resumes after expiry");
    check(state.movement_boundaries(movement,100,4100)==std::vector<std::uint64_t>{200,2200,3100},"forecast exposes knockdown and speed boundaries");
    state.consume_movement(2200);
    check(state.movement_integral(movement,2200,4100)==1450&&state.movement_integral(movement,100,4100)==0,"consumed history cannot replay");
    state.consume_movement(100);
    check(state.movement_integral(movement,2200,4100)==1450,"backwards consumption");
    check(state.knock_down(scope,true,3000,2000)==control::Knockdown::accepted,"second separated pause");
    check(state.movement_integral(movement,2200,6000)==1400,"multiple pauses preserve movement modifier history");
    check(state.knock_down(scope,true,5000,2000)==control::Knockdown::accepted,"exact recovery permits next knockdown");
    check(state.movement_integral(movement,2200,8000)==1400,"adjacent pauses do not leak movement");
    check(state.knock_down(scope,true,UINT64_MAX-1,2000)==control::Knockdown::invalid,"duration overflow");

    Definition definition;definition.binding={146,100,1,1234,0,0,8,1};definition.relation=Relation::hostile_to_players;
    definition.spawn_health=combat::Health{100,100};definition.damageable=true;definition.conditions_allowed=false;
    definition.knockdown_allowed=true;definition.knockdown_evidence="authored-test";
    const Lifetime life{1,1,2,146,100};LiveActor npc(definition,life);
    const auto pending=npc.begin_action(life,control::Action::spell,100,500);
    check(pending&&npc.active_action(life,100)==pending&&!npc.active_action(life,500),"NPC current spell snapshot");
    check(pending&&npc.knock_down(life,200)==control::Knockdown::accepted,"knockdown independent of condition allowance");
    check(!npc.complete_action(life,*pending,3000)&&npc.control_epoch(life)==1,"NPC interrupted spell epoch");
    auto wrong=life;++wrong.instance_generation;
    check(!npc.active_action(wrong,3000),"stale lifetime cannot inspect action");
    check(!npc.control_epoch(wrong)&&npc.knock_down(wrong,3000)==control::Knockdown::invalid,"full lifetime required");
    const auto last=npc.begin_action(life,control::Action::attack,3000,3500);check(last.has_value(),"post-pause action");
    check(npc.apply_damage(life,{combat::Kind::armor_ignoring,100,20,133},3100).killed,"fixture death");
    check(!npc.active_action(life,3200),"dead NPC has no current action snapshot");
    check(!npc.attacking(life,3200)&&!npc.complete_action(life,*last,4000)&&!npc.begin_action(life,control::Action::spell,4000,4500),"dead NPC action");
    npc.actor(life)->health->current=100;npc.actor(life)->effects=0;
    check(!npc.complete_action(life,*last,4000)&&npc.control_epoch(life)==2,"revival must not restore pre-death action");
    for(unsigned cause=0;cause<5;++cause){
        auto d=definition;d.spawn_health=combat::Health{10,100};d.conditions_allowed=true;
        if(cause==4)d.relation=Relation::neutral;
        LiveActor victim(d,life);const auto old=victim.begin_action(life,control::Action::attack,100,5000);
        check(old.has_value(),"death action fixture");
        if(cause==0)check(victim.apply_weapon_damage(life,20)->killed,"weapon death");
        if(cause==1)check(victim.apply_life_steal(life,*skills::effects::life_steal(153),{50,100})->killed,"life-steal death");
        if(cause==2)check(victim.apply_shadow_damage(life,*skills::effects::shadow_damage(133),{50,100},20)->hit.killed,"shadow death");
        if(cause==3){check(victim.apply_condition(life,combat::conditions::Kind::burning,42,100,5000),"burn fixture");check(victim.advance_conditions(life,2100)->killed,"degeneration death");}
        if(cause==4)check(victim.apply_guardian_weapon_damage(life,20)->killed,"guardian weapon death");
        victim.actor(life)->health->current=100;victim.actor(life)->effects=0;
        check(victim.control_epoch(life)==1&&!victim.complete_action(life,*old,6000),"death then revival replayed action token");
    }

    // A stationary ward protects a current body, not a cached susceptibility
    // bit. Preflight copies must observe movement and exact expiry too.
    {
        auto d=definition;d.knockdown_allowed.reset();d.knockdown_evidence.clear();
        LiveActor guarded(d,life);
        const auto ward=*skills::effects::defensive_ground_ward(
            {life.world,25,1},80,2,0,0,0,*skills::effects::defensive_ground_ward_plan(938,0),100);
        skills::effects::DefensiveWardRecipient recipient{guarded.hex_scope(),2,0,0,0,true,false};
        unsigned queries=0;
        guarded.environmental_knockdown_protection([&](auto candidate,auto now){
            ++queries;return candidate==recipient.scope&&
                skills::effects::defensive_ward_prevents_knockdown(ward,recipient,now);
        });
        const auto action=guarded.begin_action(life,control::Action::attack,100,500);
        check(action.has_value(),"ward attack fixture");
        auto proposal=guarded;
        check(!guarded.knockdown_allowed(life).has_value(),"environment must not rewrite base evidence");
        check(proposal.knockdown_allowed(life,200)==std::optional(false),"ward resolves unknown susceptibility in admission");
        check(guarded.knock_down(life,200)==control::Knockdown::immune&&
            guarded.active_action(life,200)==action&&guarded.control_epoch(life)==0,"ward immunity preserves action and epoch");
        recipient.x=323;
        check(!proposal.knockdown_allowed(life,300).has_value()&&
            guarded.knock_down(life,300)==control::Knockdown::unknown,"copied preflight sees exit without inventing susceptibility");
        recipient.x=0;recipient.spirit=true;
        check(guarded.knock_down(life,300)==control::Knockdown::unknown,"spirit excluded");
        recipient.spirit=false;recipient.team=3;
        check(guarded.knock_down(life,300)==control::Knockdown::unknown,"opposing team excluded");
        recipient.team=2;recipient.plane=1;
        check(guarded.knock_down(life,300)==control::Knockdown::unknown,"other plane excluded");
        recipient.plane=0;
        check(proposal.knockdown_allowed(life,10099)==std::optional(false)&&
            !proposal.knockdown_allowed(life,10100).has_value(),"exact environmental expiry");
        const auto before=queries;
        check(!guarded.knockdown_allowed(wrong,300).has_value()&&queries==before,"stale lifetime must not query current environment");
        LiveActor susceptible(definition,life);
        susceptible.environmental_knockdown_protection([&](auto candidate,auto now){
            return candidate==recipient.scope&&skills::effects::defensive_ward_prevents_knockdown(ward,recipient,now);
        });
        check(susceptible.knock_down(life,10099)==control::Knockdown::immune&&
            susceptible.knock_down(life,10100)==control::Knockdown::accepted,"base susceptibility resumes after expiry");
    }

    const std::string hash(64,'a');
    const std::string row="npc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 - 1 0 test test - - -"
        " - - - - - - 1 - - - - - - - - - - - - - -";
    for(const auto value:{"-","0","1"}){
        const std::string evidence=std::string(value)=="-"?"-":"test";
        const std::string text="gw-npc-combat 10 "+hash+"\nsource test authored "+hash+" tests/npc_action_control.cpp\n"+row+" "+value+" "+evidence+"\n";
        std::istringstream input(text);const auto catalog=load_catalog(input,hash);
        const auto trait=catalog.find(146,100)->knockdown_allowed;
        check(trait==(std::string(value)=="-"?std::optional<bool>{}:std::optional(std::string(value)=="1")),"catalog trait value");
        if(evidence=="test"){
            auto bad=text;bad.replace(bad.rfind(" test\n"),6," -\n");bool rejected=false;
            try{std::istringstream input_bad(bad);load_catalog(input_bad,hash);}catch(const std::invalid_argument&){rejected=true;}
            check(rejected,"explicit false/true trait lacks evidence");
        }
    }
    std::istringstream legacy("gw-npc-combat 9 "+hash+"\nsource test authored "+hash+" tests/npc_action_control.cpp\n"+row+"\n");
    check(!load_catalog(legacy,hash).find(146,100)->knockdown_allowed,"older metadata inferred susceptibility");
    std::cout<<"NPC action/control tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}



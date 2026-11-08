#include "gw_npc_catalog.h"
#include "gw_presearing_items.h"
#include "gw_skill_casting.h"
#include "gw_quest_encounters.h"
#include "gw_map_assets_generated.h"
#include <iostream>

int main()try{
    using namespace fw::gw;using namespace world;using namespace combat::npcs;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const std::string hash(64,'a');
    {
        using combat::ai::Pursuit;using combat::ai::retains;
        const skills::effects::Scope source{1,100,1},target{1,2,1};
        const Pursuit held{source,target,100,0,0};
        check(retains(held,source,target,300,0,0,300,500,600),"hostile pursuit forgot acquired target outside danger radius");
        check(!retains(held,source,target,300,0,0,501,500,600),"pursuit ignored separation break");
        check(!retains(held,source,target,701,0,0,10,500,600),"pursuit ignored distance from acquisition origin");
        check(!retains(held,source,target,300,0,1,10,500,600),"pursuit crossed plane");
        auto stale=target;++stale.generation;
        check(!retains(held,source,stale,300,0,0,10,500,600),"pursuit retained recycled target");
        auto recycled=source;++recycled.generation;
        check(!retains(held,recycled,target,300,0,0,10,500,600),"pursuit survived recycled NPC");
        check(!retains(held,source,target,300,0,0,10,0,600),"disabled pursuit retained target");
        auto returning=held;returning.returning=true;
        check(!retains(returning,source,target,300,0,0,10,500,600),"returning NPC immediately reacquired player");
    }
    {
        const auto content=std::filesystem::path(__FILE__).parent_path().parent_path()/"data/gw/quests";
        std::ifstream actors(content/"actors/marksmanship-worms.txt"),metadata(content/"marksmanship-development-combat.txt");
        check(bool(actors)&&bool(metadata),"prepared Marksmanship encounter files missing");
        auto captured=load_npc_catalog(actors);
        const auto combat=load_catalog(metadata,"c50d237b0d17fe1d8d836725c966d71909d717a119abb756a6c84f5bdde715f2");
        bind_combat_catalog(captured,combat,"c50d237b0d17fe1d8d836725c966d71909d717a119abb756a6c84f5bdde715f2");
        const auto worms=captured.for_map(162);
        check(worms.size()==5,"prepared encounter includes giver or loses worms");
        Instance challenge;challenge.configure_npcs(worms);
        for(const auto& worm:worms){
            const auto scope=challenge.effect_scope(worm.id);check(bool(scope),"captured worm lifetime unavailable");
            const auto state=challenge.npc_combat_snapshot(*scope);
            check(state&&state->health&&state->health->current==80&&state->health->maximum==80,"wiki worm health not installed");
        }
    }
    {
        Instance instance;Instance::Npc ele,monk,guide;
        ele.id=10;ele.definition.id=10;ele.captured_name="Howland the Elementalist";
        monk.id=11;monk.definition.id=11;monk.captured_name="Ciglo the Monk";
        ele.definition.visual_adjustment={0,0,0,100};monk.definition.visual_adjustment={0,0,0,100};
        guide.id=12;guide.definition.id=12;guide.definition.visual_adjustment={0,0,0,100};guide.captured_name="Haversdan";
        instance.configure_npcs({ele,monk,guide});
        instance.join(1,"Elementalist",0,0,std::make_shared<Outbox>());
        check(instance.set_starter_trainer_stage(1,{6,false}),"trainer stage refused");
        check(instance.nearby_npc_name(1,10,100).has_value(),"primary trainer hidden");
        check(!instance.nearby_npc_name(1,11,100),"foreign trainer visible");
        check(!instance.nearby_npc_name(1,12,100),"guide appeared before primary reward");
        const auto spawned=[](const std::vector<Packet>& packets,unsigned actor){
            return std::any_of(packets.begin(),packets.end(),[&](const auto& p){
                return p.size()>=6&&p[0]==32&&p[1]==0&&
                    (unsigned(p[2])|(unsigned(p[3])<<8)|(unsigned(p[4])<<16)|(unsigned(p[5])<<24))==actor;
            });
        };
        check(spawned(instance.npc_packets(),10)&&!spawned(instance.npc_packets(),11),"entry publishes foreign trainer");
        instance.join(2,"Monk",0,0,std::make_shared<Outbox>());
        check(instance.set_starter_trainer_stage(2,{3,false}),"party stage refused");
        check(instance.nearby_npc_name(1,11,100).has_value(),"party trainer absent");
        check(spawned(instance.npc_packets(),10)&&spawned(instance.npc_packets(),11),"party entry missing trainer spawn");
        check(instance.set_starter_trainer_stage(1,{6,true}),"reward stage refused");
        check(!instance.nearby_npc_name(1,10,100),"rewarded trainer retained");
        check(instance.nearby_npc_name(1,12,100).has_value(),"primary reward did not publish guide");
        check(instance.nearby_npc_name(2,11,100).has_value(),"other member trainer retired");
        check(!spawned(instance.npc_packets(),10)&&spawned(instance.npc_packets(),11),"reward publication retained trainer");
        instance.leave(2);
        check(!instance.nearby_npc_name(1,11,100)&&!spawned(instance.npc_packets(),11),"departed member trainer retained");
        instance.join(3,"Reentered Ele",0,0,std::make_shared<Outbox>());
        check(instance.set_starter_trainer_stage(3,{6,false}),"reentry stage refused");
        check(spawned(instance.npc_packets(),10)&&!spawned(instance.npc_packets(),11),"reentry population not reconstructed");
    }
    std::istringstream appearance("gw-npcs 1\n# Synthetic quest enemy\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 50 0 0 - -\n");
    auto population=load_npc_catalog(appearance);
    std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+
        " tests/npc_world_combat.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 0 test test test - - 60 - test -\n");
    const auto reviewed=load_catalog(metadata,hash);
    {
        auto bound=population;bind_combat_catalog(bound,reviewed,hash);
        std::vector<Instance::Npc> worms;
        for(unsigned i=0;i<5;++i){auto npc=bound.for_map(146).front();npc.id=100+i;npc.definition.id=npc.id;
            npc.combat_definition->binding.actor=npc.id;npc.combat_definition->binding.map=162;worms.push_back(npc);}
        Instance timed;auto output=std::make_shared<Outbox>();timed.join(2,"Timed Tester",0,0,output);timed.set_vitals(2,{100,100,20,20},100);
        const auto owner=*timed.effect_scope(2);output->drain();
        check(!timed.spawn_kill_encounter(owner,42,1,worms,[]{return true;}),"timed quest admitted through ordinary encounter path");
        check(!timed.spawn_marksmanship_encounter(owner,1,worms,[]{return false;},100)&&!timed.effect_scope(60000)&&output->drain().empty(),"failed timed acceptance published enemies or clock");
        check(timed.spawn_marksmanship_encounter(owner,1,worms,[]{return true;},100),"timed world encounter refused");
        const auto published=output->drain();check(std::any_of(published.begin(),published.end(),[](const auto& p){return p==arena::countdown_2022(u"\x56b",0,90000,4);}),"timed acceptance omitted verified clock");
        for(unsigned i=0;i<5;++i){const auto target=*timed.effect_scope(60000+i);
            check(timed.damage_npc(owner,target,{combat::Kind::earth,500,20,174},1248,2000+i).killed,"timed fixture kill failed");}
        const auto credits=timed.pending_quest_credits(owner);check(credits.size()==5&&timed.marksmanship_state(owner,1)==quests::MarksmanshipObjective::State::completed,"timely five kills did not complete or queue credits");
        check(std::all_of(credits.begin(),credits.end(),[](const auto& credit){return credit.occurred_mono_ms>=2000&&credit.occurred_mono_ms<=2004;}),"world lost kill occurrence timestamp");
        auto changed=credits.front();++changed.occurred_mono_ms;
        check(!timed.acknowledge_quest_credit(owner,changed)&&timed.acknowledge_quest_credit(owner,credits.front()),"credit acknowledgement ignored occurrence identity");
        check(!timed.expire_marksmanship(owner,1,90100),"completed encounter expired");
        output->drain();check(timed.clear_kill_objective(owner,42,1,true),"timed cleanup refused");
        const auto cleared=output->drain();check(std::find(cleared.begin(),cleared.end(),arena::clear_clock_2022())!=cleared.end(),"timed cleanup retained native clock");
        Instance expired;auto discarded=std::make_shared<Outbox>();expired.join(2,"Late Tester",0,0,discarded);expired.set_vitals(2,{100,100,20,20},100);
        const auto late_owner=*expired.effect_scope(2);check(expired.spawn_marksmanship_encounter(late_owner,2,worms,[]{return true;},100),"late fixture setup failed");
        const auto first=*expired.effect_scope(60000);check(expired.damage_npc(late_owner,first,{combat::Kind::earth,500,20,174},1248,90100).killed,"late physical kill rejected");
        check(expired.pending_quest_credits(late_owner).empty()&&expired.expire_marksmanship(late_owner,2,90100),"late kill credited or expiry refused");
        check(expired.marksmanship_state(late_owner,2)==quests::MarksmanshipObjective::State::failed,"expiry state lost");
        Instance resumed;auto resumed_output=std::make_shared<Outbox>();
        resumed.join(2,"Returning Tester",0,0,resumed_output);resumed.set_vitals(2,{100,100,20,20},100);
        const auto returning=*resumed.effect_scope(2);
        check(resumed.restore_marksmanship_encounter(returning,3,worms,[]{return true;},100,35000,5),"partial challenge restore failed");
        check(!resumed.effect_scope(60000)&&resumed.effect_scope(60001)&&!resumed.effect_scope(60002),"credited worms respawned after reconnect");
        const auto restored_packets=resumed_output->drain();
        check(std::find(restored_packets.begin(),restored_packets.end(),arena::countdown_2022(u"\x56b",35000,90000,4))!=restored_packets.end(),"reconnect restarted client timer");
        const auto surviving=*resumed.effect_scope(60001);
        check(resumed.damage_npc(returning,surviving,{combat::Kind::earth,500,20,174},1248,200).killed,"restored worm kill failed");
        const auto restored_credits=resumed.pending_quest_credits(returning);
        check(restored_credits.size()==1&&restored_credits.front().credit==2,"logical credit identity changed across instance");
        check(resumed.expire_marksmanship(returning,3,55100),"restored world timer received fresh ninety seconds");
    }
    {
        auto enemies=population;bind_combat_catalog(enemies,reviewed,hash);
        Instance world;world.configure_npcs(enemies.for_map(146));
        world.join(2,"Necromancer",0,0,std::make_shared<Outbox>());world.set_vitals(2,{40,100,20,20},100);
        const auto source=*world.effect_scope(2),target=*world.effect_scope(100);
        const auto plan=*skills::effects::life_steal(153,0);
        auto stale=target;++stale.generation;
        check(!world.life_steal(source,stale,plan,100),"stale NPC lifetime accepted life steal");
        check(world.life_steal(source,target,plan,100),"NPC life steal rejected");
        check(world.npc_combat_snapshot(target)->health->current==82,"NPC life steal incorrectly reduced by armor");
        check(world.resources(2)->health==58,"NPC life steal failed to heal caster");
        for(unsigned i=0;i<5;++i)check(world.life_steal(source,target,plan,101+i),"NPC repeated life steal rejected");
        check(world.npc_combat_snapshot(target)->health->current==0,"NPC life steal did not kill");
        check((world.npc_combat_snapshot(target)->effects&0x10)&&world.resources(2)->health==100,"NPC life steal death presentation or capped healing failed");
        check(world.take_npc_deaths(source).size()==1,"NPC life steal death credit missing or duplicated");
        check(!world.life_steal(source,target,plan,110)&&world.take_npc_deaths(source).empty(),"dead NPC life steal duplicated death credit");
    }
    {
        auto bow_population=population;bind_combat_catalog(bow_population,reviewed,hash);
        bow_population.maps.at(146).front().combat_definition->physical_armor=60;
        bow_population.maps.at(146).front().combat_definition->physical_armor_evidence="test";
        Instance bow_world;bow_world.configure_npcs(bow_population.for_map(146));
        bow_world.join(2,"Ranger Test",0,0,std::make_shared<Outbox>());bow_world.set_vitals(2,{100,100,20,20},100);
        const auto owner=*bow_world.effect_scope(2),target=*bow_world.effect_scope(100);
        auto inventory=inventory::presearing::creation_weapon(inventory::baseline(),2,u"Ranger Test");
        combat::weapon::Policy policy;policy.primary=2;policy.character="Ranger Test";policy.caster_weapon=true;
        const auto bow=combat::weapon::owned_weapon(*inventory,1,policy);
        check(bow&&bow_world.ordinary_weapon(owner,*bow,1)&&bow_world.ordinary_attack(owner,100,100),"starter bow attack rejected");
        bow_world.tick(600);
        check(bow_world.npc_combat_snapshot(target)->health->current==100,"bow damage applied before flight");
        bow_world.tick(700);
        const auto after=bow_world.npc_combat_snapshot(target)->health->current;
        check(after<100,"bow projectile failed to hit physical armor");
        {
            Instance shot;shot.configure_npcs(bow_population.for_map(146));
            shot.join(2,"Ranger Test",0,0,std::make_shared<Outbox>());shot.set_vitals(2,{100,100,20,20},100);
            const auto source=*shot.effect_scope(2),foe=*shot.effect_scope(100);
            check(shot.ordinary_weapon(source,*bow,1),"Point Blank Shot weapon setup failed");
            const auto plan=*combat::weapon::attack_skill(407,12);
            check(plan.required_type==5&&plan.bonus==34&&shot.melee_target(source,100,600),"Point Blank Shot bow/rank/target gate failed");
            check(shot.melee_attack(source,foe,*bow,plan,600),"Point Blank Shot launch failed");
            check(shot.npc_combat_snapshot(foe)->health->current==100,"Point Blank Shot applied instant melee damage");
            shot.tick(700);
            check(std::abs(shot.npc_combat_snapshot(foe)->health->current-(after-34))<.001,"Point Blank Shot Expertise bonus not applied on arrival");
            auto wrong=*bow;wrong.item_type=27;
            check(!shot.melee_attack(source,foe,wrong,plan,800),"Point Blank Shot allowed non-bow weapon");
        }
        for(const bool prepared:{false,true}){
            Instance dual;dual.configure_npcs(bow_population.for_map(146));
            dual.join(2,"Ranger Test",0,0,std::make_shared<Outbox>());dual.set_vitals(2,{100,100,20,20},100);
            const auto source=*dual.effect_scope(2),foe=*dual.effect_scope(100);
            check(dual.ordinary_weapon(source,*bow,1),"Dual Shot equipment failed");
            if(prepared)check(dual.status_mutation(source,432,1,12,100,24100,true),"Dual Shot preparation failed");
            check(dual.melee_attack(source,foe,*bow,*combat::weapon::attack_skill(396,0),600),"Dual Shot NPC launch failed");
            check(dual.npc_combat_snapshot(foe)->health->current==100,"Dual Shot damage preceded flight");
            dual.tick(700);
            check(std::abs(dual.npc_combat_snapshot(foe)->health->current-(100-(100-after)*1.5-(prepared?18:0)))<.001,
                "Dual Shot did not hit twice or reduced preparation bonus");
        }
        {
            auto unknown_armor=bow_population.for_map(146);unknown_armor.front().combat_definition->physical_armor.reset();
            Instance shot;shot.configure_npcs(unknown_armor);shot.join(2,"Ranger Test",0,0,std::make_shared<Outbox>());shot.set_vitals(2,{100,100,20,20},100);
            const auto source=*shot.effect_scope(2),foe=*shot.effect_scope(100);
            check(shot.ordinary_weapon(source,*bow,1)&&shot.melee_target(source,100,100,394),"Power Shot incorrectly needs target physical armor");
            check(shot.melee_attack(source,foe,*bow,*combat::weapon::attack_skill(394,12),100),"Power Shot NPC flight rejected");
            shot.tick(200);check(shot.npc_combat_snapshot(foe)->health->current==55,"Power Shot is not fixed45 armor-ignoring damage");
        }
        const auto prepared_hit=[&](bool expire_in_flight,bool already_expired){
            Instance prepared;prepared.configure_npcs(bow_population.for_map(146));
            prepared.join(2,"Ranger Test",0,0,std::make_shared<Outbox>());
            prepared.set_vitals(2,{100,100,20,20},100);
            const auto actor=*prepared.effect_scope(2),foe=*prepared.effect_scope(100);
            check(prepared.ordinary_weapon(actor,*bow,1),"prepared bow equipment failed");
            const std::uint64_t start=already_expired?24100:expire_in_flight?23499:100;
            check(prepared.status_mutation(actor,432,1,12,0,24000,true),"Read the Wind registry refused preparation");
            check(prepared.ordinary_attack(actor,100,start),"prepared arrow attack failed");
            prepared.tick(start+500);
            check(prepared.npc_combat_snapshot(foe)->health->current==100,"prepared arrow hit before flight");
            if(!already_expired){
                prepared.tick(start+540);
                check(prepared.npc_combat_snapshot(foe)->health->current==100,"prepared arrow flight too short");
            }
            prepared.tick(start+600);
            return prepared.npc_combat_snapshot(foe)->health->current;
        };
        check(std::abs(prepared_hit(false,false)-(after-9))<.001,"Read the Wind NPC arrow bonus missing");
        check(std::abs(prepared_hit(true,false)-(after-9))<.001,"preparation expiry changed arrow already in flight");
        check(std::abs(prepared_hit(false,true)-after)<.001,"expired preparation changed subsequent arrow");
        bow_world.tick(2500);
        check(bow_world.npc_combat_snapshot(target)->health->current==after,"recurve repeat fired early");
        bow_world.tick(3075);bow_world.tick(3175);
        check(bow_world.npc_combat_snapshot(target)->health->current<after,"recurve repeat failed");
    }
    std::istringstream missing_evidence("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+
        " tests/npc_world_combat.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 0 test test test - - 60 - - -\n");
    bool missing_armor_source=false;try{load_catalog(missing_evidence,hash);}catch(const std::invalid_argument&){missing_armor_source=true;}
    check(missing_armor_source,"fire armor accepted without evidence reference");
    const auto general_row="\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 0 test test test - - 60 - test - - - 1 - - - 80 test\n";
    std::istringstream general_metadata("gw-npc-combat 6 "+hash+"\nsource test authored "+hash+" tests/npc_world_combat.cpp"+general_row);
    const auto general_catalog=load_catalog(general_metadata,hash);
    check(general_catalog.find(146,100)->general_armor==80,"general armor catalog field lost");
    auto unreferenced_general=std::string(general_row);
    unreferenced_general.replace(unreferenced_general.rfind("80 test"),7,"80 -");
    std::istringstream bad_general("gw-npc-combat 6 "+hash+"\nsource test authored "+hash+" tests/npc_world_combat.cpp"+unreferenced_general);
    bool missing_general_source=false;try{load_catalog(bad_general,hash);}catch(const std::invalid_argument&){missing_general_source=true;}
    check(missing_general_source,"general armor accepted without evidence reference");
    std::istringstream energy_metadata("gw-npc-combat 7 "+hash+"\nsource test authored "+hash+" tests/npc_world_combat.cpp"+std::string(general_row).substr(0,std::string(general_row).size()-1)+" 2 20 test\n");
    const auto energy_catalog=load_catalog(energy_metadata,hash);
    check(energy_catalog.find(146,100)->spawn_energy==Energy{2,20},"NPC energy metadata lost");
    const auto resource_prefix=std::string(general_row).substr(0,std::string(general_row).size()-1);
    for(const auto& suffix:std::vector<std::pair<unsigned,std::string>>{
            {7," 2 - test"},{7," 2 0 test"},{7," 21 20 test"},{7," 2 20 -"},
            {8," - - - 5 test"},{8," 2 20 test 11 test"},{8," 2 20 test 5 -"}}){
        std::istringstream invalid_resource("gw-npc-combat "+std::to_string(suffix.first)+" "+hash+"\nsource test authored "+hash+" tests/npc_world_combat.cpp"+resource_prefix+suffix.second+"\n");
        bool rejected=false;try{load_catalog(invalid_resource,hash);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"Invalid NPC resource metadata accepted");
    }
    auto regenerating=*energy_catalog.find(146,100);regenerating.energy_pips=5;
    LiveActor regenerating_actor(regenerating,{1,1,1,146,100});const auto energy_life=regenerating_actor.lifetime();
    check(regenerating_actor.advance_energy(energy_life,100)&&regenerating_actor.lose_energy(energy_life,2),"NPC energy clock setup failed");
    check(regenerating_actor.advance_energy(energy_life,699)&&regenerating_actor.energy(energy_life)->current==0,"NPC energy rounded up early");
    check(regenerating_actor.advance_energy(energy_life,700)&&regenerating_actor.energy(energy_life)->current==1,"NPC five-pip fractional regeneration lost");
    check(!regenerating_actor.advance_energy(energy_life,699)&&regenerating_actor.energy(energy_life)->current==1,"NPC energy clock moved backwards");
    bind_combat_catalog(population,reviewed,hash);
    {
        auto casters=population.for_map(146);casters.front().combat_definition->spawn_energy=Energy{5,5};
        casters.front().combat_definition->energy_pips=0;
        Instance ai;ai.configure_npcs(casters);ai.join(2,"NPC Spell Target",0,0,std::make_shared<Outbox>());
        ai.set_vitals(2,{100,100,20,20},100);
        const auto caster=*ai.effect_scope(100),target=*ai.effect_scope(2);
        check(ai.configure_npc_spellbook(caster,{{{194,0},1248,"synthetic reviewed Flare"}}),"World reviewed spellbook rejected");
        const auto token=ai.begin_npc_spell(caster,target,1000);
        check(token&&token->skill==skills::Slot{194,0}&&ai.npc_energy_snapshot(caster)->current==0,
            "World NPC AI spell did not select/pay real Energy");
        check(!ai.begin_npc_spell(caster,target,1001)&&ai.complete_npc_action(*token,token->completes),
            "World NPC spell busy/completion lifecycle wrong");
        check(!ai.begin_npc_spell(caster,target,10000),"World NPC spell fabricated Energy after payment");
        auto stale=caster;++stale.generation;
        check(!ai.configure_npc_spellbook(stale,{})&&!ai.begin_npc_spell(stale,target,10000),"NPC AI accepted stale caster generation");
    }
    {
        auto casters=population.for_map(146);casters.front().combat_definition->spawn_energy=Energy{20,20};
        casters.front().combat_definition->energy_pips=0;
        Instance ai;ai.configure_npcs(casters);ai.join(2,"AI Target",0,0,std::make_shared<Outbox>());
        ai.set_vitals(2,{100,100,20,20},100);
        const auto caster=*ai.effect_scope(100),target=*ai.effect_scope(2);
        check(ai.configure_npc_spellbook(caster,{{{194,0},1248,"synthetic Fire Magic priority"},
            {{214,0},1248,"synthetic Water Magic fallback"}}),"World fallback spellbook rejected");
        const auto first=ai.begin_npc_spell(caster,target,1000);
        check(first&&first->skill==skills::Slot{194,0}&&ai.npc_energy_snapshot(caster)->current==15,
            "World AI initial priority/payment wrong");
        // NPC-targeted Power Block is not implemented by interrupt_skill;
        // exercise the existing supported NPC Diversion authority instead.
        check(ai.diversion(target,caster,*skills::effects::diversion(12),1100)&&
            ai.complete_npc_action(*first,first->completes),"NPC Diversion/completion fixture failed");
        check(ai.diversion_skill_used(caster,{194,0},1,2000,0,0)==49000,
            "completed NPC spell did not acquire Diversion disable");
        check(ai.skill_disabled_slot(caster,{194,0},2800)==49000&&ai.skill_disabled_slot(caster,{214,0},2800)==0,
            "Diversion disabled unrelated AI spell slot");
        const auto fallback=ai.begin_npc_spell(caster,target,2800);
        check(fallback&&fallback->skill==skills::Slot{214,0}&&ai.npc_energy_snapshot(caster)->current==10,
            "externally disabled priority spell stalled usable World AI fallback or paid wrong Energy");
        check(!ai.complete_npc_action(*first,2801)&&ai.complete_npc_action(*fallback,fallback->completes),
            "old AI token replayed or fallback failed completion");
        const auto restored=ai.begin_npc_spell(caster,target,49000);
        check(restored&&restored->skill==skills::Slot{194,0},"Diversion expiry failed to restore World AI priority");
    }
    {
        auto fixture=[&](bool measured=true,unsigned energy=20){
            auto actors=population.for_map(146);auto& definition=*actors.front().combat_definition;
            definition.spawn_health=combat::Health{500,500};
            if(measured){definition.spawn_energy=Energy{energy,40};definition.energy_pips=0;}
            auto world=std::make_unique<Instance>();world->configure_npcs(actors);
            world->join(2,"Interrupter",0,0,std::make_shared<Outbox>());world->set_vitals(2,{500,500,10,100},100);
            return world;
        };
        for(unsigned skill:{5u,23u,25u,61u,24u,931u,860u})for(unsigned protection:{0u,16u,17u}){
            auto ai=fixture();const auto source=*ai->effect_scope(2),target=*ai->effect_scope(100);
            if(protection)check(ai->npc_interrupt_stance(target,protection,12,10000,900),"NPC interrupt stance rejected");
            if(skill==860)check(ai->diversion(source,target,*skills::effects::diversion(12),900),"hexed interrupt fixture rejected");
            const skills::Slot active{static_cast<std::uint16_t>(skill==860?1:194),7};
            const auto token=ai->begin_npc_action(target,combat::npcs::control::Action::spell,1000,5000,active);
            const auto plan=*skills::effects::interrupt_skill(skill,12);
            check(token&&ai->interrupt_skill(source,target,plan,20,1100),"reviewed player-to-NPC interrupt rejected");
            check(ai->complete_npc_action(*token,5000)==bool(protection),"NPC interrupt protection/control result wrong");
            check(ai->npc_combat_snapshot(target)->health->current==500-plan.damage,"NPC interrupt damage wrong or protection wrongly suppressed damage");
            const unsigned stance_cost=protection==17?5:0;
            const unsigned expected_energy=20-stance_cost-plan.target_loss+plan.target_gain;
            check(ai->npc_energy_snapshot(target)->current==expected_energy,"NPC interrupt target Energy cost/loss/gain wrong");
            check(ai->resources(2)->energy==10+plan.energy,"NPC interrupt source spell reward wrong");
            if(skill==5)check(ai->skill_disabled_slot(target,{194,7},5000)==11100&&
                ai->skill_disabled_slot(target,{214,0},5000)==0,"NPC Power Block protection/attribute disable wrong");
        }
        for(unsigned protection:{16u,17u}){
            auto ai=fixture(true,3);const auto source=*ai->effect_scope(2),target=*ai->effect_scope(100);
            check(ai->npc_interrupt_stance(target,protection,12,10000,900),"final interrupt protection fixture failed");
            const auto first=ai->begin_npc_action(target,combat::npcs::control::Action::spell,1000,5000,skills::Slot{194,0});
            check(first&&ai->interrupt_skill(source,target,*skills::effects::interrupt_skill(23,12),20,1100)&&
                ai->npc_energy_snapshot(target)->current==(protection==17?0u:3u)&&ai->complete_npc_action(*first,5000),"final stance charge failed to protect interrupt");
            const auto second=ai->begin_npc_action(target,combat::npcs::control::Action::spell,6000,10000,skills::Slot{194,0});
            check(second&&ai->interrupt_skill(source,target,*skills::effects::interrupt_skill(23,12),20,6100)&&
                !ai->complete_npc_action(*second,10000),"consumed interrupt stance survived another attempt");
        }
        for(unsigned skill:{24u,931u}){
            auto ai=fixture(false);const auto source=*ai->effect_scope(2),target=*ai->effect_scope(100);
            const auto token=ai->begin_npc_action(target,combat::npcs::control::Action::spell,1000,5000,skills::Slot{194,0});
            check(token&&!ai->interrupt_skill(source,target,*skills::effects::interrupt_skill(skill,12),20,1100)&&
                ai->complete_npc_action(*token,5000),"unknown NPC Energy partially committed interrupt");
        }
        {
            auto ai=fixture(false);const auto source=*ai->effect_scope(2),target=*ai->effect_scope(100);
            check(ai->npc_interrupt_stance(target,17,12,10000,900),"unknown-Energy Resolve fixture failed");
            const auto token=ai->begin_npc_action(target,combat::npcs::control::Action::spell,1000,5000,skills::Slot{194,0});
            check(token&&!ai->interrupt_skill(source,target,*skills::effects::interrupt_skill(23,12),20,1100)&&
                ai->npc_combat_snapshot(target)->health->current==500&&ai->complete_npc_action(*token,5000),
                "unknown protection Energy guessed or damage partially committed");
        }
        {
            auto ai=fixture(false);const auto source=*ai->effect_scope(2),target=*ai->effect_scope(100);
            check(ai->npc_interrupt_stance(target,16,12,10000,900),"Concentration fixture failed");
            const auto token=ai->begin_npc_action(target,combat::npcs::control::Action::spell,1000,5000,skills::Slot{194,0});
            check(token&&ai->interrupt_skill(source,target,*skills::effects::interrupt_skill(23,12),20,1100)&&
                ai->complete_npc_action(*token,5000),"Concentration unnecessarily required NPC Energy");
        }
        {
            auto ai=fixture();const auto source=*ai->effect_scope(2),target=*ai->effect_scope(100);
            const auto attack=ai->begin_npc_action(target,combat::npcs::control::Action::attack,1000,5000);
            check(attack&&ai->interrupt_skill(source,target,*skills::effects::interrupt_skill(61,12),20,1100)&&
                ai->resources(2)->energy==10&&!ai->complete_npc_action(*attack,5000),"Power Drain gained spell Energy from basic NPC attack");
            const auto unknown=ai->begin_npc_action(target,combat::npcs::control::Action::spell,6000,10000);
            check(unknown&&!ai->interrupt_skill(source,target,*skills::effects::interrupt_skill(5,12),20,6100)&&
                ai->complete_npc_action(*unknown,10000),"unknown NPC skill identity fabricated Power Block eligibility");
            auto stale=target;++stale.generation;
            check(!ai->interrupt_skill(source,stale,*skills::effects::interrupt_skill(23,12),20,11000),"interrupt accepted stale NPC generation");
        }
    }
    {
        const auto directory=std::filesystem::path(QUEST_CONTENT_DIRECTORY);
        std::ifstream giver_file(directory/"actors/monk-test-givers.txt"),giver_metadata(directory/"monk-test-giver-combat.txt");
        auto givers=load_npc_catalog(giver_file);const std::string giver_hash="14b6cc1b3b97e5ad5b4bcb922486890c0675036440f23e93ff38d6ccb2001b1f";
        bind_combat_catalog(givers,load_catalog(giver_metadata,giver_hash),giver_hash);
        std::ifstream baseline_file(directory.parent_path()/"npcs.txt");auto baseline=load_npc_catalog(baseline_file);
        merge_quest_givers(baseline,givers,{"Gwen"});
        const auto merged_count=baseline.for_map(146).size();
        merge_quest_givers(baseline,givers,{"Gwen"},true);
        check(baseline.for_map(146).size()==merged_count,"Repeated same-placement quest givers duplicated actors");
        const auto& replaced=baseline.for_map(146);
        check(std::count_if(replaced.begin(),replaced.end(),[](const auto& npc){return npc.captured_name=="Gwen";})==1,"Quest placement left duplicate Gwen");
        const auto found_gwen=std::find_if(replaced.begin(),replaced.end(),[](const auto& npc){return npc.captured_name=="Gwen";});
        check(found_gwen->x==3802&&found_gwen->y==-4559&&found_gwen->combat_definition&&!found_gwen->combat_definition->damageable,"Quest Gwen replacement lost captured placement or noncombat policy");
        check(found_gwen->literal_display_name==std::optional<std::u16string>(u"Gwen")&&!found_gwen->definition.encoded_name.empty(),"Quest literal name lost captured source codes");
        Instance named_population;named_population.configure_npcs(replaced);const auto name_packets=named_population.npc_packets();
        check(std::find(name_packets.begin(),name_packets.end(),npc::literal_name_2022(found_gwen->id,u"Gwen"))!=name_packets.end(),"Quest literal name override not published after spawn");
        auto scoped_population=baseline;
        auto& scoped_gwen=*std::find_if(scoped_population.maps.at(146).begin(),scoped_population.maps.at(146).end(),[](const auto& n){return n.captured_name=="Gwen";});
        scoped_population.primary_placements.push_back({146,scoped_gwen.id,3,scoped_gwen.x,scoped_gwen.y,scoped_gwen.facing,scoped_gwen.plane});
        scoped_gwen.x=8023;scoped_gwen.y=2545;
        for(unsigned primary=1;primary<=6;++primary){
            const auto actors=scoped_population.for_primary(146,primary);
            const auto gwen=std::find_if(actors.begin(),actors.end(),[](const auto& n){return n.captured_name=="Gwen";});
            check(gwen!=actors.end()&&gwen->id==scoped_gwen.id&&gwen->definition.id==scoped_gwen.definition.id,"Scoped Gwen changed identity");
            check(gwen->x==(primary==3?3802:8023)&&gwen->y==(primary==3?-4559:2545),"Gwen river position escaped primary Monk scope");
            check(actors.size()==scoped_population.for_map(146).size(),"Primary placement duplicated population");
        }
        check(scoped_gwen.x==8023&&scoped_population.for_primary(145,3).size()==scoped_population.for_map(145).size(),"Scoped placement mutated base or other map");
        auto mismatched=givers;mismatched.maps.at(146)[1].definition.model_file=123;
        bool wrong_identity=false;try{merge_quest_givers(baseline,mismatched,{"Gwen"});}catch(const std::invalid_argument&){wrong_identity=true;}
        check(wrong_identity&&std::count_if(baseline.for_map(146).begin(),baseline.for_map(146).end(),[](const auto& npc){return npc.captured_name=="Gwen";})==1,"Mismatched placement override changed installed population");
        std::ifstream enemy_file(directory/"actors/monk-test-skale.txt",std::ios::binary);const std::string enemy_bytes((std::istreambuf_iterator<char>(enemy_file)),{});
        const std::string enemy_hash="faa268470eba50f6fe4ba66517c51c6823d5ffc045afe51f0ee6665a5c02a5ec";
        std::ifstream manifest(directory/"monk-test-escorts.txt");const auto encounters=load_quest_escorts(manifest,directory,[&](std::string_view bytes){return bytes==enemy_bytes?enemy_hash:std::string(64,'0');});
        Instance monk;monk.configure_navigation(std::make_shared<navigation::Mesh>(navigation::Mesh::load(directory.parent_path()/"navigation"/(std::to_string(map_asset_file(146))+".fwnav"))));
        monk.configure_npcs(givers.for_map(146));monk.join(42,"Monk Content",3802,-4559,std::make_shared<Outbox>());monk.set_vitals(42,{100,100,20,20},100);
        const auto owner=*monk.effect_scope(42);const auto roles=escort_participants(monk,givers,encounters.at(84));check(roles.has_value(),"Captured Monk roles failed to resolve");
        check(accept_escort_encounter(monk,owner,roles->first,roles->second,146,1,encounters.at(84),[]{return true;}),"Captured Monk encounter failed to spawn on mesh");
        for(unsigned id=60000;id<60003;++id)check(monk.npc_combat_snapshot(*monk.effect_scope(id))->health->maximum==10,"Monk measured blocker capacity changed");
        for(const auto& blocker:encounters.at(84).blockers)check(blocker.combat_definition->fire_armor==0&&blocker.combat_definition->cold_armor==0,"Monk caster armor policy incomplete");
        check(monk.discover_escort(owner,1,1248,101)&&monk.escort_objective_state(owner,1)==quests::EscortObjective::State::blocked&&!monk.follow_escort(owner,1,220,128,101),"Captured Gwen followed before blockers cleared");
        const auto holy_gear=*inventory::presearing::creation_weapon(inventory::baseline(),3,u"Monk Content");
        const auto holy_rod=combat::weapon::owned_weapon(holy_gear,1,{3,"Monk Content",{},true,{}});
        check(holy_rod&&monk.ordinary_weapon(owner,*holy_rod,1)&&monk.ordinary_attack(owner,60000,102),"Monk Holy Rod attack approach rejected by actual blocker content");
        monk.cancel_ordinary_attack(owner);
        monk.move(42,4702,-2843,102);monk.tick(20102);
        for(unsigned id=60000;id<60003;++id)check(monk.damage_npc(owner,*monk.effect_scope(id),{combat::Kind::armor_ignoring,20,20,194},1248,20103).killed,"Captured Monk blocker did not die");
        check(monk.pending_quest_completions(owner)==std::vector<Instance::QuestCompletion>{{84,1}}&&!monk.escort_reward_ready(owner,1),"Monk blocker completion granted early reward");
        check(monk.follow_escort(owner,1,220,128,20104),"Gwen could not route from captured spawn to player");
        const auto gwen_path=monk.maintained_range_path(roles->first,20104,40104);
        check(gwen_path&&!gwen_path->empty(),"NPC route forecast missing");monk.tick(40104);
        const auto gwen_actual=monk.maintained_range_path(roles->first,40104,40104);
        check(gwen_actual&&std::hypot(gwen_path->back().x-gwen_actual->front().x,gwen_path->back().y-gwen_actual->front().y)<.01,
            "Read-only NPC forecast disagrees with materialized movement");
        monk.move(42,5942,4172,40105);
        for(std::uint64_t now=41105;now<=160105;now+=1000){monk.tick(now);monk.follow_escort(owner,1,220,128,now);}
        check(monk.arrive_escort(owner,1,128,160106)&&monk.escort_reward_ready(owner,1),"Captured Gwen could not follow real Lakeside route to Ciglo");
    }
    {
        auto follower=population.for_map(146).front();follower.id=follower.definition.id=110;
        follower.combat_definition->binding.actor=follower.combat_definition->binding.definition=110;follower.combat_definition->relation=Relation::neutral;
        auto recipient=follower;recipient.id=recipient.definition.id=111;recipient.combat_definition->binding.actor=recipient.combat_definition->binding.definition=111;
        follower.captured_name="Gwen";recipient.captured_name="Ciglo the Monk";
        Instance escort;escort.configure_npcs({follower,recipient});escort.join(2,"Escort Owner",0,0,std::make_shared<Outbox>());escort.set_vitals(2,{100,100,20,20},100);
        const auto owner=*escort.effect_scope(2),gwen=*escort.effect_scope(110),ciglo=*escort.effect_scope(111);
        const std::vector enemies(3,population.for_map(146).front());
        const NpcCatalog::EscortEncounter encounter{146,enemies,"Gwen","Ciglo the Monk",100,100,10,100};
        const auto manifest_directory=std::filesystem::path(TEST_DIRECTORY)/"escort-parser";std::filesystem::create_directories(manifest_directory);
        std::ostringstream fixture_population,fixture_combat;fixture_population<<"gw-npcs 1\n";fixture_combat<<"gw-npc-combat 1 "<<hash<<"\nsource fixture authored "<<hash<<" tests/npc_world_combat.cpp\n";
        for(unsigned id=100;id<103;++id){
            fixture_population<<"npc 146 "<<id<<' '<<id<<" 1234 0 1677721600 0 8 1 1 0 50 0 0 - -\n";
            fixture_combat<<"npc 146 "<<id<<' '<<id<<" 1234 0 0 8 1 hostile-to-players - 100 100 60 1 0 fixture fixture fixture - -\n";
        }
        {std::ofstream file(manifest_directory/"actors.txt",std::ios::binary);file<<fixture_population.str();}
        {std::ofstream file(manifest_directory/"combat.txt");file<<fixture_combat.str();}
        const auto fixture_hash=[&](std::string_view bytes){return bytes==fixture_population.str()?hash:std::string(64,'0');};
        const auto row="escort 84 146 actors.txt combat.txt "+hash+" \"Gwen\" \"Ciglo the Monk\" 100 100 10 100\n";
        std::istringstream manifest("gw-quest-escorts 1\n"+row);const auto loaded=load_quest_escorts(manifest,manifest_directory,fixture_hash);
        check(loaded.at(84).blockers.size()==3&&loaded.at(84).recipient=="Ciglo the Monk","Escort manifest lost blockers or quoted recipient");
        bool duplicate=false;try{std::istringstream repeated("gw-quest-escorts 1\n"+row+row);load_quest_escorts(repeated,manifest_directory,fixture_hash);}catch(const std::invalid_argument&){duplicate=true;}
        check(duplicate,"Duplicate escort manifest accepted");
        NpcCatalog escort_population;escort_population.maps[146]={follower,recipient};
        check(escort_participants(escort,escort_population,encounter)==std::optional(std::pair{gwen,ciglo}),"Escort role resolution failed");
        auto ambiguous=escort_population;ambiguous.maps[146].push_back(follower);check(!escort_participants(escort,ambiguous,encounter),"Ambiguous Gwen role accepted");
        check(!escort.spawn_escort_encounter(owner,gwen,ciglo,1,enemies,[]{return false;})&&!escort.effect_scope(60000)&&!escort.escort_objective_state(owner,1),"Failed escort acceptance exposed blockers or objective");
        auto stale_gwen=gwen;++stale_gwen.generation;
        check(!escort.spawn_escort_encounter(owner,stale_gwen,ciglo,1,enemies,[]{return true;}),"Recycled Gwen accepted escort");
        check(accept_escort_encounter(escort,owner,gwen,ciglo,146,1,encounter,[]{return true;}),"Content-driven atomic escort acceptance failed");
        check(!escort.spawn_kill_encounter(owner,84,1,enemies,[]{return true;}),"Kill controller replaced escort objective");
        check(!escort.discover_escort(owner,1,10,100)&&!escort.arrive_escort(owner,1,100,100)&&!escort.escort_reward_ready(owner,1),"Distant discovery or early arrival advanced escort");
        check(!escort.follow_escort(owner,1,100,10,100),"Undiscovered or blocked Gwen followed player");
        for(unsigned id=60000;id<60003;++id)check(escort.damage_npc(owner,*escort.effect_scope(id),{combat::Kind::armor_ignoring,200,20,194},1248,101).killed,"Escort blocker survived");
        check(escort.escort_objective_state(owner,1)==quests::EscortObjective::State::searching&&escort.pending_quest_completions(owner).empty(),"Undiscovered escort completed on kills alone");
        check(escort.discover_escort(owner,1,100,102)&&escort.escort_objective_state(owner,1)==quests::EscortObjective::State::following&&
              escort.pending_quest_completions(owner).size()==1&&!escort.escort_reward_ready(owner,1),"Spatial discovery failed or granted early reward");
        check(!escort.follow_escort(owner,2,100,10,102)&&!escort.follow_escort(owner,1,100,10,102),"Gwen left nearby recipient or accepted wrong attempt");
        escort.tick(602);check(!escort.follow_escort(owner,1,100,10,603),"Gwen did not reach player follow range");
        check(!escort.arrive_escort(owner,2,100,604)&&escort.arrive_escort(owner,1,100,604)&&escort.escort_reward_ready(owner,1),"Spatial arrival attempt fence failed");
        check(escort.clear_kill_objective(owner,84,1,true)&&!escort.effect_scope(60000)&&!escort.escort_objective_state(owner,1),"Escort cleanup leaked blockers");
        check(!escort.restore_completed_escort_objective(owner,gwen,ciglo,145,1)&&escort.restore_completed_escort_objective(owner,gwen,ciglo,146,1),"Escort restoration map fence failed");
        check(escort.escort_objective_state(owner,1)==quests::EscortObjective::State::following&&!escort.escort_reward_ready(owner,1)&&!escort.effect_scope(60003),"Escort restoration recreated blockers or granted reward");
        check(!escort.restore_completed_escort_objective(owner,gwen,ciglo,146,1)&&escort.arrive_escort(owner,1,100,605)&&escort.escort_reward_ready(owner,1),"Restored escort lost arrival gate");
        follower.x=260;recipient.x=0;Instance boundary;boundary.configure_npcs({follower,recipient});boundary.join(2,"Interaction Border",150,0,std::make_shared<Outbox>());boundary.set_vitals(2,{100,100,20,20},100);
        const auto boundary_owner=*boundary.effect_scope(2),boundary_gwen=*boundary.effect_scope(110),boundary_ciglo=*boundary.effect_scope(111);
        check(boundary.restore_completed_escort_objective(boundary_owner,boundary_gwen,boundary_ciglo,146,1)&&
              boundary.follow_escort(boundary_owner,1,100,128,100),"Gwen stopped at player instead of approaching nearby recipient");
        boundary.tick(3100);check(boundary.arrive_escort(boundary_owner,1,128,3101)&&boundary.escort_reward_ready(boundary_owner,1),"Normal NPC interaction distance blocked escort reward");
    }
    {
        const auto content=std::filesystem::path(QUEST_CONTENT_DIRECTORY);
        std::ifstream appearance(content/"actors/necromancer-test-verata.txt"),metadata(content/"necromancer-test-trainer-combat.txt");
        auto trainers=load_npc_catalog(appearance);const std::string trainer_hash="34e37c8c6184cd3846d03332af680de485bdcbc974cafdc4ee0b8f1e1a223435";
        const auto trainer_combat=load_catalog(metadata,trainer_hash);bind_combat_catalog(trainers,trainer_combat,trainer_hash);
        NpcCatalog installed;merge_quest_givers(installed,trainers);
        Instance world;world.configure_npcs(installed.for_map(146));world.join(2,"Trainer Test",0,0,std::make_shared<Outbox>());
        world.set_vitals(2,{100,100,20,20},100);const auto owner=*world.effect_scope(2),trainer=*world.effect_scope(installed.for_map(146).front().id);
        check(!world.npc_combat_snapshot(trainer)->health,"Trainer content invented health");
        check(world.spawn_corpse_encounter(owner,trainer,1,population.for_map(146),[]{return true;}),"Captured noncombat trainer could not start objective");
        const auto enemy=*world.effect_scope(60000);
        check(world.damage_npc(owner,enemy,{combat::Kind::armor_ignoring,200,20,194},1248,100).killed,"Trainer test enemy survived");
        check(world.approach_quest_corpse(owner,1,trainer,enemy,1000,100),"Unknown-health trainer could not approach corpse");
        world.tick(10100);check(world.claim_quest_corpse(owner,82,1,trainer,enemy,10),"Unknown-health trainer stopped before reaching corpse");
    }
    {
        auto trainer=population.for_map(146).front();trainer.combat_definition->relation=Relation::neutral;trainer.x=1000;
        {
            auto mentor=trainer;mentor.x=0;Instance follow;follow.configure_npcs({mentor});
            follow.join(2,"Follow Owner",0,0,std::make_shared<Outbox>());follow.set_vitals(2,{100,100,20,20},100);
            const auto player=*follow.effect_scope(2),guide=*follow.effect_scope(100);
            check(follow.spawn_corpse_encounter(player,guide,1,population.for_map(146),[]{return true;}),"follow fixture acceptance failed");
            check(!follow.follow_corpse_trainer(player,1,220,100),"restoration automatically armed Verata follow");
            check(!follow.arm_corpse_follow(player,2,guide,100)&&follow.arm_corpse_follow(player,1,guide,100),"dialogue cycle fence failed");
            follow.move(2,500,0,101);follow.tick(2501);
            check(!follow.nearby_npc_name(2,100,2501),"follow fixture player did not move away");
            check(follow.follow_corpse_trainer(player,1,220,2502),"dialogue intent did not start Verata route");
            follow.tick(5002);
            check(follow.nearby_npc_name(2,100,5002).has_value(),"Verata did not reach following player");
            follow.leave(2);follow.join(2,"New Follow Owner",500,0,std::make_shared<Outbox>());follow.set_vitals(2,{100,100,20,20},100);
            const auto reentered=*follow.effect_scope(2);
            check(follow.spawn_corpse_encounter(reentered,guide,1,population.for_map(146),[]{return true;})&&!follow.follow_corpse_trainer(reentered,1,220,5101),"reentry restored follow without dialogue");
        }
        Instance staged;staged.configure_npcs({trainer});staged.join(2,"Corpse Owner",0,0,std::make_shared<Outbox>());
        staged.set_vitals(2,{100,100,20,20},100);const auto owner=*staged.effect_scope(2),verata=*staged.effect_scope(100);
        const auto enemies=population.for_map(146);
        check(!staged.spawn_corpse_encounter(owner,verata,1,enemies,[]{return false;})&&!staged.effect_scope(60000)&&!staged.corpse_objective_state(owner,82,1),"Failed corpse acceptance exposed world state");
        check(staged.spawn_corpse_encounter(owner,verata,1,enemies,[]{return true;}),"Corpse encounter activation failed");
        const auto enemy=*staged.effect_scope(60000);
        const std::array targets{enemy};check(!staged.begin_kill_objective(owner,82,1,targets),"Kill-only objective replaced staged corpse workflow");
        check(staged.damage_npc(owner,enemy,{combat::Kind::armor_ignoring,200,20,194},1248,100).killed,"Corpse fixture enemy survived");
        check(staged.pending_quest_completions(owner).empty()&&staged.corpse_objective_state(owner,82,1)==quests::CorpseObjective::State::waiting_corpse,"Necromancer quest completed on kill alone");
        check(!staged.reserve_quest_corpse(owner,82,2,enemy)&&staged.reserve_quest_corpse(owner,82,1,enemy),"Staged corpse attempt fence failed");
        unsigned publications=0;
        check(!staged.consume_quest_corpse(owner,82,1,enemy,[&]{++publications;return true;})&&publications==0,"Unclaimed corpse reached summon publication");
        check(!staged.claim_quest_corpse(owner,82,1,verata,enemy,10)&&staged.corpse_objective_state(owner,82,1)==quests::CorpseObjective::State::waiting_corpse,"Distant trainer claimed corpse");
        auto stale_trainer=verata;++stale_trainer.generation;check(!staged.claim_quest_corpse(owner,82,1,stale_trainer,enemy,1000),"Recycled trainer claimed corpse");
        check(!staged.approach_quest_corpse(owner,1,stale_trainer,enemy,1000,100)&&staged.approach_quest_corpse(owner,1,verata,enemy,1000,100),"Trainer corpse approach lifetime validation failed");
        check(!staged.approach_quest_corpse(owner,1,verata,enemy,1000,100),"Duplicate approach replaced active navigation");
        staged.tick(1100);
        check(staged.claim_quest_corpse(owner,82,1,verata,enemy,10)&&staged.corpse_objective_state(owner,82,1)==quests::CorpseObjective::State::waiting_summon&&staged.pending_quest_completions(owner).empty(),"Trainer did not approach corpse or claim completed quest before summon");
        check(!staged.claim_quest_corpse(owner,82,1,verata,enemy,1000),"Corpse claim advanced twice");
        check(!staged.consume_quest_corpse(owner,82,1,enemy,[&]{++publications;return false;})&&publications==1,"Failed publication consumed corpse");
        std::ifstream bone_file(std::filesystem::path(QUEST_CONTENT_DIRECTORY)/"actors/necromancer-test-bone.txt",std::ios::binary);
        const std::string bone_bytes((std::istreambuf_iterator<char>(bone_file)),{});
        const std::string bone_hash="23d4859323bacf6d5f9d9fde0eeda587f99bb964ba3af5b1e4a29a120440a4d2";
        const auto content=std::filesystem::path(QUEST_CONTENT_DIRECTORY);
        const auto hash_bone=[&](std::string_view bytes){return bytes==bone_bytes?bone_hash:std::string(64,'0');};
        auto bone=load_quest_summon(content/"actors/necromancer-test-bone.txt",content/"necromancer-test-summon-combat.txt",146,bone_hash,hash_bone);
        bool bad_map=false;try{load_quest_summon(content/"actors/necromancer-test-bone.txt",content/"necromancer-test-summon-combat.txt",145,bone_hash,hash_bone);}catch(const std::invalid_argument&){bad_map=true;}
        bool bad_hash=false;try{load_quest_summon(content/"actors/necromancer-test-bone.txt",content/"necromancer-test-summon-combat.txt",146,std::string(64,'0'),hash_bone);}catch(const std::invalid_argument&){bad_hash=true;}
        check(bad_map&&bad_hash,"Summon loader accepted wrong map or revision");
        std::ifstream enemy_file(content/"actors/necromancer-test-skale.txt",std::ios::binary);
        const std::string enemy_bytes((std::istreambuf_iterator<char>(enemy_file)),{});
        const std::string enemy_hash="3123ceecab078f5eb150b2c96657bfe4e293b8c68858c4ec96930a1c9d15882a";
        const auto manifest_hash=[&](std::string_view bytes){return bytes==enemy_bytes?enemy_hash:hash_bone(bytes);};
        const auto manifest_row="corpse 82 146 actors/necromancer-test-skale.txt necromancer-test-enemy-combat.txt "+enemy_hash+
            " actors/necromancer-test-bone.txt necromancer-test-summon-combat.txt "+bone_hash+" 220 10\n";
        std::istringstream manifest("gw-quest-corpses 1\n"+manifest_row);
        const auto loaded_corpses=load_quest_corpses(manifest,content,manifest_hash);
        check(loaded_corpses.at(82).enemies.size()==4&&loaded_corpses.at(82).summon.captured_name=="Bone Horror","Corpse manifest lost content");
        check(loaded_corpses.at(82).enemies[1].combat_definition->spawn_health->maximum==10&&
              loaded_corpses.at(82).enemies[3].combat_definition->spawn_health->maximum==10,"Captured quest-specific skale capacity replaced by generic wiki value");
        {
            const auto& actors=loaded_corpses.at(82).enemies;Instance attack;attack.configure_npcs(actors);
            attack.join(42,"Necro Trial",actors[1].x-50,actors[1].y,std::make_shared<Outbox>());attack.set_vitals(42,{100,100,20,20},100);
            const auto owner=*attack.effect_scope(42),victim=*attack.effect_scope(actors[1].id);
            const auto gear=inventory::presearing::creation_weapon(inventory::baseline(),4,u"Necro Trial");
            const auto rod=combat::weapon::owned_weapon(*gear,1,{4,"Necro Trial",{},true,{}});
            check(rod&&attack.ordinary_weapon(owner,*rod,1)&&attack.ordinary_attack(owner,actors[1].id,100),"Necromancer captured skale rejected starter Truncheon");
            attack.tick(600);attack.tick(700);
            check(attack.npc_combat_snapshot(victim)->health->current<10,"starter Truncheon did not damage captured skale");
        }
        bool duplicate=false;try{std::istringstream repeated("gw-quest-corpses 1\n"+manifest_row+manifest_row);load_quest_corpses(repeated,content,manifest_hash);}catch(const std::invalid_argument&){duplicate=true;}
        check(duplicate,"Duplicate corpse manifest accepted");
        check(bone.captured_name=="Bone Horror"&&bone.definition.model_file==48647&&bone.definition.level==5&&
              bone.definition.visual_adjustment==std::array<std::uint8_t,4>{0,0,0,150},"Captured Bone Horror appearance changed");
        check(!staged.publish_quest_corpse_summon(owner,82,1,verata,enemy,bone,[&]{++publications;return false;})&&publications==2&&
              !staged.effect_scope(60001)&&staged.pending_quest_completions(owner).empty(),"Failed summon commit exposed actor or completion");
        check(!staged.publish_quest_corpse_summon(owner,82,1,stale_trainer,enemy,bone,[&]{++publications;return true;})&&publications==2,"Stale trainer published summon");
        check(staged.publish_quest_corpse_summon(owner,82,1,verata,enemy,bone,[&]{++publications;return true;})&&publications==3&&
              staged.effect_scope(60001)&&staged.corpse_objective_state(owner,82,1)==quests::CorpseObjective::State::completed&&
              staged.pending_quest_completions(owner).size()==1,"Summon publication did not complete staged objective");
        const auto summon=staged.npc_combat_snapshot(*staged.effect_scope(60001));
        check(summon&&!summon->health&&!summon->damageable,"Summon publication invented uncaptured combat stats");
        check(!staged.consume_quest_corpse(owner,82,1,enemy,[&]{++publications;return true;})&&publications==3&&
              !staged.publish_quest_corpse_summon(owner,82,1,verata,enemy,bone,[]{return true;}),"Corpse published twice");
        check(staged.clear_kill_objective(owner,82,1,true)&&!staged.effect_scope(60000)&&!staged.effect_scope(60001)&&!staged.corpse_objective_state(owner,82,1),"Corpse objective cleanup leaked actors/state");
        check(!staged.restore_completed_corpse_objective(owner,stale_trainer,146,1)&&!staged.restore_completed_corpse_objective(owner,verata,145,1),"Completed corpse restoration accepted stale trainer or wrong map");
        check(staged.restore_completed_corpse_objective(owner,verata,146,1)&&staged.pending_quest_completions(owner).size()==1&&
              !staged.effect_scope(60002),"Completed restoration respawned quest actors or lost completion");
        check(!staged.restore_completed_corpse_objective(owner,verata,146,1)&&!staged.begin_wave_objective(owner,verata,82,1,1,1),"Completed corpse objective replaced by duplicate or wave objective");
        check(staged.clear_kill_objective(owner,82,1,true)&&staged.pending_quest_completions(owner).empty(),"Restored objective cleanup failed");
        const NpcCatalog::CorpseEncounter corpse_content{146,enemies,bone,220,10};
        auto invalid_content=corpse_content;invalid_content.claim_range=0;unsigned acceptance_commits=0;
        check(!accept_corpse_encounter(staged,owner,verata,146,2,invalid_content,[&]{++acceptance_commits;return true;})&&acceptance_commits==0,"Invalid corpse content committed durable acceptance");
        check(accept_corpse_encounter(staged,owner,verata,146,2,corpse_content,[&]{++acceptance_commits;return true;})&&acceptance_commits==1,"Second corpse attempt did not start");
        unsigned commits=0;
        check(advance_corpse_encounter(staged,owner,2,corpse_content,1200,[&]{++commits;return true;})==Instance::CorpseAdvance::pending&&commits==0,"Host controller summoned before kill");
        const auto next_enemy=*staged.effect_scope(60002);
        check(staged.damage_npc(owner,next_enemy,{combat::Kind::armor_ignoring,200,20,194},1248,1200).killed,"Second corpse attempt enemy survived");
        check(advance_corpse_encounter(staged,owner,2,corpse_content,1200,[&]{++commits;return true;})==Instance::CorpseAdvance::completed&&commits==1&&staged.effect_scope(60003),"Host controller did not publish captured summon");
        check(staged.advance_corpse_encounter(owner,2,bone,220,10,1201,[&]{++commits;return true;})==Instance::CorpseAdvance::completed&&commits==1,"Completed host controller republished summon");
        check(staged.clear_kill_objective(owner,82,2,true),"Second corpse attempt cleanup failed");
    }
    {
        LiveActor reactive(*reviewed.find(146,100),{1,1,1,146,100});const auto life=reactive.lifetime();
        check(reactive.apply_empathy(life,26,0,7,1,100)&&bool(reactive.actor(life)->effects&0x800),"NPC Empathy status failed");
        check(reactive.empathy_attack(life,1,101)->damage==10&&reactive.actor(life)->health->current==90,"NPC Empathy reaction failed");
        check(!reactive.empathy_attack(life,1,102)&&reactive.actor(life)->health->current==90,"Empathy repeated an attack event");
        auto stale=life;++stale.actor_generation;check(!reactive.empathy_attack(stale,2,103),"Empathy accepted stale NPC generation");
        check(reactive.empathy_attack(life,2,103)->damage==10&&reactive.empathy_attack_damage(life,5,5099)==4&&reactive.empathy_attack_damage(life,5,5100)==5,"Empathy trigger/impact boundary wrong");
        check(reactive.clear_hexes(life,104)&&!(reactive.actor(life)->effects&0x800)&&!reactive.empathy_attack(life,3,105),"NPC hex removal left Empathy active");
    }
    {
        auto enemies=population.for_map(146);enemies[0].x=10;enemies[0].y=0;
        enemies[0].combat_definition->spawn_energy=Energy{2,20};
        Instance feast;feast.configure_npcs(enemies);feast.join(2,"Feast",0,0,std::make_shared<Outbox>());
        feast.set_vitals(2,{100,480,20,20},100);const auto caster=*feast.effect_scope(2),target=*feast.effect_scope(100);
        auto stale=target;++stale.generation;
        check(!feast.energy_drain(caster,stale,*skills::effects::energy_drain(40,12),1,100)&&feast.resources(2)->health==100,
            "stale NPC Feast changed caster health");
        check(feast.energy_drain(caster,target,*skills::effects::energy_drain(40,12),1,100)&&feast.resources(2)->health==212,
            "NPC Feast did not heal for two actual energy");
        check(feast.energy_drain(caster,target,*skills::effects::energy_drain(40,12),1,100)&&feast.resources(2)->health==212,
            "NPC Feast did not retain depleted energy");
        enemies[0].combat_definition->spawn_energy.reset();Instance unknown;unknown.configure_npcs(enemies);
        unknown.join(2,"Unknown",0,0,std::make_shared<Outbox>());unknown.set_vitals(2,{100,480,20,20},100);
        check(!unknown.energy_drain(*unknown.effect_scope(2),*unknown.effect_scope(100),*skills::effects::energy_drain(40,12),1,100)&&unknown.resources(2)->health==100,
            "NPC Feast invented an unknown energy pool");
    }
    {
        auto first=population.for_map(146).front();first.combat_definition->fire_armor=0;
        first.combat_definition->spawn_health=combat::Health{35,35};first.x=20;first.y=0;
        auto second=first;second.id=101;second.combat_definition->binding.actor=101;second.x=1000;
        const NpcCatalog::KillEncounter encounter{146,{first,second},true};
        Instance any;any.join(2,"One Skale",0,0,std::make_shared<Outbox>());
        any.set_vitals(2,{100,100,20,20},100);const auto owner=*any.effect_scope(2);
        check(!accept_kill_encounter(any,owner,146,81,1,encounter,[]{return false;})&&!any.effect_scope(60000),
            "one-kill encounter published before failed acceptance");
        check(accept_kill_encounter(any,owner,146,81,1,encounter,[]{return true;}),"one-kill encounter acceptance failed");
        const auto target=*any.effect_scope(60000),survivor=*any.effect_scope(60001);
        check(any.pending_quest_completions(owner).empty(),"one-kill quest completed without a death");
        check(any.fire_storm(owner,target,*skills::effects::fire_storm(12),20,1000),"one-kill combat setup failed");
        any.tick(3100);
        check(any.npc_combat_snapshot(target)->health->current==0&&any.npc_combat_snapshot(survivor)->health->current==35,
            "one-kill test did not preserve surviving encounter actor");
        check(any.pending_quest_completions(owner)==std::vector<Instance::QuestCompletion>{{81,1}},
            "one-kill quest required every assigned actor to die");
        check(!any.reserve_quest_corpse(owner,81,1,survivor)&&!any.reserve_quest_corpse(owner,81,2,target),"Corpse reservation accepted live actor/wrong attempt");
        auto recycled=target;++recycled.generation;check(!any.reserve_quest_corpse(owner,81,1,recycled),"Corpse reservation accepted recycled NPC lifetime");
        check(any.reserve_quest_corpse(owner,81,1,target),"Dead owned quest actor could not be reserved");
        unsigned publications=0;
        check(!any.consume_quest_corpse(owner,81,1,target,[&]{++publications;return false;}),"Failed summon publication consumed corpse");
        check(any.consume_quest_corpse(owner,81,1,target,[&]{++publications;return true;})&&publications==2,"Reserved corpse did not survive failed publication");
        check(!any.consume_quest_corpse(owner,81,1,target,[&]{++publications;return true;})&&publications==2&&!any.reserve_quest_corpse(owner,81,1,target),"Consumed corpse was used/published twice");
        any.leave(2);any.join(2,"New Session",0,0,std::make_shared<Outbox>());
        check(any.pending_quest_completions(*any.effect_scope(2)).empty(),"one-kill completion leaked across reconnect");
    }
    {
        auto enemies=population.for_map(146);enemies[0].x=2000;enemies[0].combat_definition->fire_armor=0;
        Instance approach;auto box=std::make_shared<Outbox>();approach.configure_npcs(enemies);
        approach.join(2,"Approach",0,0,box);approach.set_vitals(2,{100,100,20,20},100);
        const auto owner=*approach.effect_scope(2),target=*approach.effect_scope(100);
        const auto gear=*inventory::presearing::creation_weapon(inventory::baseline(),6,u"Approach");
        const auto rod=*combat::weapon::owned_weapon(gear,1,{6,"Approach",{},true,{}});
        check(approach.ordinary_weapon(owner,rod,1)&&approach.configure_projectiles({100,2}),"approach equipment failed");
        check(approach.ordinary_attack(owner,100,100),"distant NPC attack did not schedule approach");
        approach.tick(3000);check(approach.npc_combat_snapshot(target)->health->current==100,"distant NPC damaged before reaching weapon range");
        approach.tick(4100);const auto point=approach.position(2,4100);
        check(point.first>800&&point.first<1000,"attack approach teleported or did not move");
        approach.tick(4600);check(approach.npc_combat_snapshot(target)->health->current==100,"approach skipped projectile flight");
        approach.tick(16000);check(approach.npc_combat_snapshot(target)->health->current<100,"approach never started ordinary attack");
        check(approach.position(2,16000)==point,"attacker continued to target center after reaching range");

        Instance cancelled;cancelled.configure_npcs(enemies);cancelled.join(2,"Approach",0,0,std::make_shared<Outbox>());
        cancelled.set_vitals(2,{100,100,20,20},100);const auto cancelled_owner=*cancelled.effect_scope(2),cancelled_target=*cancelled.effect_scope(100);
        cancelled.ordinary_weapon(cancelled_owner,rod,1);
        check(cancelled.ordinary_attack(cancelled_owner,100,100),"cancel test approach failed");
        cancelled.move(2,100,0,200);cancelled.tick(20000);
        check(cancelled.npc_combat_snapshot(cancelled_target)->health->current==100&&cancelled.position(2,20000).first==100,"manual movement failed to cancel attack approach");
    }
    {
        Instance reconnect;auto entering=std::make_shared<Outbox>(),ready=std::make_shared<Outbox>();
        reconnect.join(2,"Connecting",0,0,entering,0,{}, {},0,0,false);
        reconnect.join(3,"Already Loaded",0,0,ready);entering->drain();ready->drain();
        const auto owner=*reconnect.effect_scope(2);
        check(reconnect.spawn_loot_encounter(owner,83,1,population.for_map(146),100,[]{return true;}),"deferred reconnect encounter failed");
        check(entering->drain().empty(),"restored encounter queued duplicate spawn before initial NPC snapshot");
        auto count_spawns=[](const auto& packets){return std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>1&&p[0]==32&&p[1]==0;});};
        check(count_spawns(ready->drain())==1,"already loaded peer missed restored encounter");
        check(count_spawns(reconnect.npc_entry_packets(2))==1,"initial NPC snapshot lost restored encounter");
        bool repeated=false;try{reconnect.npc_entry_packets(2);}catch(const std::runtime_error&){repeated=true;}
        check(repeated,"initial NPC snapshot repeated actor spawns");
        check(reconnect.clear_quest_loot(owner,83,1),"deferred encounter cleanup failed");entering->drain();ready->drain();
        check(reconnect.spawn_loot_encounter(owner,83,2,population.for_map(146),200,[]{return true;})&&
              count_spawns(entering->drain())==1,"loaded owner missed later encounter publication");
    }
    for(unsigned primary:{3u,4u,5u,6u}){
        auto enemies=population.for_map(146);auto& def=*enemies[0].combat_definition;
        def.fire_armor=0;def.general_armor=80;def.spawn_health=combat::Health{35,35};enemies[0].x=10;
        Instance world;auto box=std::make_shared<Outbox>();world.configure_npcs(enemies);world.join(2,"Rod Tester",0,0,box);
        world.set_vitals(2,{100,100,20,20},100);const auto owner=*world.effect_scope(2),target=*world.effect_scope(100);
        const auto gear=*inventory::presearing::creation_weapon(inventory::baseline(),primary,u"Rod Tester");
        const auto rod=combat::weapon::owned_weapon(gear,1,{primary,"Rod Tester",{},true,{}});
        check(rod&&world.ordinary_weapon(owner,*rod,1)&&world.configure_projectiles({100,2}),"starter rod equipment failed");
        const std::array targets{target};check(world.begin_kill_objective(owner,85,1,targets)&&world.ordinary_attack(owner,target.actor,100),"rod ordinary attack rejected");
        world.tick(500);check(world.npc_combat_snapshot(target)->health->current==35,"rod hit before windup");
        world.tick(600);check(world.npc_combat_snapshot(target)->health->current==35,"rod skipped projectile flight");
        world.tick(700);const auto health=world.npc_combat_snapshot(target)->health->current;
        check(primary==4||primary==5?health>33.5&&health<34:health>29&&health<30,
            "rod ignored its reviewed fire/general armor or bypassed armor");
        world.tick(800);check(world.npc_combat_snapshot(target)->health->current==health,"rod impact repeated");
        for(unsigned i=1;i<=32;++i){world.tick(600+i*1750);world.tick(700+i*1750);}
        check(world.npc_combat_snapshot(target)->health->current==0&&world.take_npc_deaths(owner).size()==1,"ordinary rod failed to kill quest enemy");
        auto missing=enemies;missing[0].combat_definition->fire_armor.reset();missing[0].combat_definition->general_armor.reset();Instance denied;denied.configure_npcs(missing);denied.join(2,"Rod Tester",0,0,std::make_shared<Outbox>());denied.set_vitals(2,{100,100,20,20},100);
        check(denied.ordinary_weapon(*denied.effect_scope(2),*rod,1)&&!denied.ordinary_attack(*denied.effect_scope(2),100,100),"rod invented missing fire armor");
    }
    {
        const auto directory=std::filesystem::path(QUEST_CONTENT_DIRECTORY);
        std::ifstream appearances(directory/"actors/grawl-invaders.txt"),metadata(directory/"grawl-development-combat.txt");
        auto content=load_npc_catalog(appearances);
        const std::string content_hash="8dfa8ac87a5443a7762d1541ba0f5d3a1ca87c1839af2a51cad08e0fc456c4a3";
        bind_combat_catalog(content,load_catalog(metadata,content_hash),content_hash);
        check(content.for_map(160).size()==3,"installed Grawl encounter incomplete");
        for(const auto& npc:content.for_map(160))check(npc.combat_definition->conditions_allowed,"installed Grawl is immune to quest skills");
        for(const auto& npc:content.for_map(160))check(npc.combat_definition->fleshy==std::optional(true),"installed development Grawl lacks authored flesh eligibility");
        Instance world;world.configure_npcs(content.for_map(160));const auto& first=content.for_map(160).front();
        world.join(2,"Grawl Tester",first.x-20,first.y,std::make_shared<Outbox>());world.set_vitals(2,{100,100,20,20},100);
        const auto owner=*world.effect_scope(2),target=*world.effect_scope(first.id);
        combat::weapon::Owned sword{3,1,{1,1,2},12,false,false,27};check(world.ordinary_weapon(owner,sword,20),"installed Grawl sword equip failed");
        check(world.melee_attack(owner,target,sword,*combat::weapon::attack_skill(382,12),100),"installed Grawl Sever rejected");
        world.tick(14100);check(world.npc_combat_snapshot(target)->health->current==0&&world.take_npc_deaths(owner).size()==1,"installed Grawl did not die from bleeding");
    }
    {
        auto enemies=population.for_map(146);auto& def=*enemies[0].combat_definition;
        def.physical_armor=60;def.conditions_allowed=true;def.fleshy=true;def.fleshy_evidence="authored positive Sever/Bleeding fixture";def.spawn_health=combat::Health{35,35};
        Instance world;auto box=std::make_shared<Outbox>();world.configure_npcs(enemies);world.join(2,"Bleeding Tester",0,0,box);
        world.set_vitals(2,{100,100,20,20},100);const auto owner=*world.effect_scope(2),target=*world.effect_scope(100);
        combat::weapon::Owned sword{3,1,{1,1,2},12,false,false,27};check(world.ordinary_weapon(owner,sword,20),"bleed sword equip failed");
        const std::array targets{target};check(world.begin_kill_objective(owner,75,1,targets),"bleed objective failed");
        check(world.melee_attack(owner,target,sword,*combat::weapon::attack_skill(382,12),100),"bleeding attack failed");
        world.tick(1100);check(world.npc_combat_snapshot(target)->health->current==28,"NPC bleeding was not3pips/6health per second");
        world.tick(6100);check(world.npc_combat_snapshot(target)->health->current==0&&world.take_npc_deaths(owner).size()==1,"bleeding death not credited");
        check(world.pending_quest_completions(owner)==std::vector<Instance::QuestCompletion>{{75,1}},"bleeding did not complete objective");
        world.tick(7100);check(world.take_npc_deaths(owner).empty(),"bleeding credited death twice");
        Instance reconnect;reconnect.configure_npcs(enemies);reconnect.join(2,"Old Session",0,0,std::make_shared<Outbox>());
        reconnect.set_vitals(2,{100,100,20,20},100);const auto old_owner=*reconnect.effect_scope(2),old_target=*reconnect.effect_scope(100);
        check(reconnect.ordinary_weapon(old_owner,sword,20)&&reconnect.melee_attack(old_owner,old_target,sword,*combat::weapon::attack_skill(382,12),100),"reconnect bleed setup failed");
        reconnect.leave(2);reconnect.join(2,"New Session",0,0,std::make_shared<Outbox>());
        const auto new_owner=*reconnect.effect_scope(2);check(new_owner!=old_owner,"reconnect retained source generation");
        reconnect.tick(6100);check(reconnect.npc_combat_snapshot(old_target)->health->current==0&&reconnect.take_npc_deaths(new_owner).empty()&&reconnect.pending_quest_completions(new_owner).empty(),"reconnect inherited old bleeding credit");
        LiveActor live(def,{1,1,1,146,100});const auto life=live.lifetime();
        check(live.apply_condition(life,combat::conditions::Kind::bleeding,2,100,1000),"pure bleed setup failed");
        check(live.advance_conditions(life,5100)->after.current==29&&!live.has_condition(life,combat::conditions::Kind::bleeding,5100),"late poll damaged after expiry");
        check(!live.advance_conditions(life,5000),"condition clock moved backwards");
        auto durable=def;durable.spawn_health=combat::Health{200,200};LiveActor renewal(durable,life);
        check(renewal.apply_condition(life,combat::conditions::Kind::bleeding,2,100,5000),"renewal setup failed");
        check(renewal.apply_condition(life,combat::conditions::Kind::bleeding,2,2100,5000)&&renewal.actor(life)->health->current==188,"renewal discarded earlier bleeding damage");
        check(renewal.advance_conditions(life,7100)->after.current==158,"renewal damage interval incorrect");
        LiveActor cleanse(durable,life);check(cleanse.apply_condition(life,combat::conditions::Kind::bleeding,2,100,5000),"cleanse setup failed");
        check(cleanse.remove_condition(life,combat::conditions::Kind::bleeding,2100)&&cleanse.actor(life)->health->current==188,"cleansing discarded elapsed damage");
        check(cleanse.advance_conditions(life,7100)->after.current==188,"cleansed bleeding kept damaging");
        auto healthy=def;healthy.spawn_health=combat::Health{200,200};LiveActor wound(healthy,life);
        check(wound.apply_condition(life,combat::conditions::Kind::deep_wound,2,100,1000),"NPC wound setup failed");
        check(wound.actor(life)->health->current==160&&wound.actor(life)->health->maximum==160,"NPC wound capacity incorrect");
        const auto recovered=wound.advance_conditions(life,1100);check(recovered&&recovered->after.current==200&&recovered->after.maximum==200,"NPC wound expiry failed");
    }
    {
        auto enemies=population.for_map(146);auto& def=*enemies[0].combat_definition;
        def.physical_armor=60;def.conditions_allowed=true;def.fleshy=true;def.fleshy_evidence="authored positive Sever/Bleeding fixture";def.spawn_health=combat::Health{200,200};
        Instance melee;auto box=std::make_shared<Outbox>();melee.configure_npcs(enemies);melee.join(2,"Sword Tester",0,0,box);
        melee.set_vitals(2,{100,100,20,20},100);const auto owner=*melee.effect_scope(2),foe=*melee.effect_scope(100);
        combat::weapon::Owned sword{3,1,{10,10,2},12,false,false,27};
        check(melee.ordinary_weapon(owner,sword,20)&&melee.melee_target(owner,100,100),"NPC melee eligibility failed");
        const auto sever=*combat::weapon::attack_skill(382,12),gash=*combat::weapon::attack_skill(384,12);
        check(melee.melee_attack(owner,foe,sword,gash,100),"NPC Gash failed");
        check(melee.npc_combat_snapshot(foe)->health->current==190&&!melee.has_condition(foe,combat::conditions::Kind::deep_wound,100),"Gash bonus without bleeding");
        check(melee.melee_attack(owner,foe,sword,sever,101)&&melee.has_condition(foe,combat::conditions::Kind::bleeding,101),"NPC Sever failed");
        check(melee.melee_attack(owner,foe,sword,gash,102)&&melee.has_condition(foe,combat::conditions::Kind::deep_wound,102),"NPC conditional Gash failed");
        check(std::abs(melee.npc_combat_snapshot(foe)->health->current-112.994)<.0001&&melee.npc_combat_snapshot(foe)->health->maximum==160,"NPC Gash bonus/Deep Wound incorrect");
        auto stale=foe;++stale.generation;check(!melee.melee_attack(owner,stale,sword,sever,103),"stale NPC target accepted");
        auto changed=sword;++changed.revision;check(!melee.melee_attack(owner,foe,changed,sever,103),"changed weapon accepted");
        check(!melee.melee_attack(owner,foe,sword,sever,UINT64_MAX),"overflow condition deadline accepted");
        const std::array targets{foe};check(melee.begin_kill_objective(owner,75,1,targets),"sword objective binding failed");
        const auto execution=*combat::weapon::attack_skill(336,12);auto axe=sword;axe.item_type=2;
        check(melee.ordinary_weapon(owner,axe,20),"axe equip failed");
        for(unsigned i=0;i<3;++i)check(melee.melee_attack(owner,foe,axe,execution,200+i),"NPC execution failed");
        check(melee.pending_quest_completions(owner)==std::vector<Instance::QuestCompletion>{{75,1}}&&melee.take_npc_deaths(owner).size()==1,"melee skill death did not complete assigned quest");
        check(!melee.melee_attack(owner,foe,axe,execution,210)&&melee.take_npc_deaths(owner).empty(),"dead NPC supplied duplicate objective credit");
    }
    {
        auto supplement=population;supplement.maps.at(146)[0].combat_definition->relation=Relation::neutral;
        auto base=population;merge_quest_givers(base,supplement);
        check(base.for_map(146).size()==2&&base.for_map(146)[0].combat_definition->relation==Relation::hostile_to_players,
            "guardian merge changed base combat population");
        const auto& guardian=base.for_map(146)[1];
        check(guardian.id==101&&guardian.definition.id==2&&guardian.combat_definition->binding.actor==101&&
            guardian.combat_definition->binding.definition==2,"guardian remap lost combat binding");
        Instance installed;installed.configure_npcs(base.for_map(146));
        check(installed.quest_guardian_available(*installed.effect_scope(101),146),"merged neutral guardian unusable");
        auto bad=supplement;bad.maps.at(146)[0].combat_definition->relation=Relation::hostile_to_players;
        bool rejected=false;try{merge_quest_givers(base,bad);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected&&base.for_map(146).size()==2,"hostile supplement changed installed population");
    }
    {
        auto guardian=population.for_map(146).front();guardian.combat_definition->relation=Relation::neutral;
        guardian.combat_definition->physical_armor=0;
        auto enemies=population.for_map(146);enemies[0].x=100;
        Instance ai;auto ai_box=std::make_shared<Outbox>();ai.configure_npcs({guardian});ai.join(2,"AI Tester",0,0,ai_box);
        ai.set_vitals(2,{100,100,20,20},100);const auto owner=*ai.effect_scope(2),target=*ai.effect_scope(100);
        check(ai.accept_quest_waves(owner,target,73,1,{enemies},[]{return true;}),"AI acceptance failed");
        const Instance::GuardianAttackPolicy policy{100,1,10,1000,"synthetic development fixture"};
        {
            Instance pursuit;pursuit.configure_npcs({guardian});pursuit.join(2,"Pursuit Owner",0,0,std::make_shared<Outbox>());
            pursuit.set_vitals(2,{100,100,20,20},100);
            const auto owner=*pursuit.effect_scope(2),guard=*pursuit.effect_scope(100);
            pursuit.physical_armor(owner,0);
            check(pursuit.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}),"pursuit fixture failed");
            auto chase=policy;chase.player_aggro_range=200;chase.player_follow_range=500;chase.player_leash_range=1000;
            auto invalid=chase;invalid.player_follow_range=100;
            check(!pursuit.drive_quest_guardians(owner,73,1,invalid,1000),"follow radius below acquisition accepted");
            invalid=chase;invalid.player_leash_range=0;
            check(!pursuit.drive_quest_guardians(owner,73,1,invalid,1000),"partial pursuit policy accepted");
            check(pursuit.drive_quest_guardians(owner,73,1,chase,1000),"pursuit acquisition failed");
            pursuit.move(2,400,0,1000);
            pursuit.tick(4000);pursuit.drive_quest_guardians(owner,73,1,chase,4000);
            pursuit.tick(8000);pursuit.drive_quest_guardians(owner,73,1,chase,8000);
            pursuit.drive_quest_guardians(owner,73,1,chase,9000);
            check(pursuit.resources(2)->health<100&&pursuit.npc_combat_snapshot(guard)->health->current==100,
                "NPC abandoned acquired player outside acquisition circle");
            const auto player_health=pursuit.resources(2)->health;
            chase.player_leash_range=100;
            pursuit.drive_quest_guardians(owner,73,1,chase,9001);
            pursuit.drive_quest_guardians(owner,73,1,chase,9002);
            pursuit.tick(13000);pursuit.drive_quest_guardians(owner,73,1,chase,13000);
            check(pursuit.resources(2)->health==player_health&&pursuit.npc_combat_snapshot(guard)->health->current<100,
                "leash break reacquired adjacent player before returning to guardian");
        }
        {
            Instance reactive;reactive.configure_npcs({guardian});auto output=std::make_shared<Outbox>();
            reactive.join(2,"Empathy Owner",100,0,output);reactive.set_vitals(2,{100,100,20,20},100);
            const auto caster=*reactive.effect_scope(2),guard=*reactive.effect_scope(100);
            check(reactive.physical_armor(caster,0)&&reactive.accept_quest_waves(caster,guard,73,1,{enemies},[]{return true;}),"Empathy world fixture failed");
            const auto attacker=*reactive.effect_scope(60000);
            skills::Policy cast_policy;cast_policy.primary=5;cast_policy.campaigns=0x1e;cast_policy.learned[26]=1;
            skills::Slots bar{};bar[0]={26,0};attributes::Ranks ranks{};skills::casting::Hooks hooks;
            hooks.events=[&](const auto& packets){output->enqueue(packets);};
            hooks.resources=[&](unsigned id){return reactive.resources(id);};
            hooks.consume_energy=[&](unsigned cost){return reactive.consume_energy(2,cost);};
            hooks.target_valid=[](auto,float,bool){return false;};
            hooks.projectile_target_valid=[&](unsigned id,float range){return id==attacker.actor&&range==1248&&reactive.effect_scope(id)==attacker;};
            hooks.target_scope=[&](unsigned id){return reactive.effect_scope(id);};
            hooks.empathy=[&](skills::effects::Scope target,const skills::effects::EmpathyPlan& plan,std::uint64_t now){return reactive.empathy_npc(caster,target,plan.skill,plan.rank,now);};
            skills::casting::Runtime cast(2);
            check(cast.start(100,{26,0,attacker.actor},bar,cast_policy,ranks,true,hooks)==skills::casting::Reason::accepted,"Integrated Empathy cast rejected");
            cast.update(2099,hooks);check(!(reactive.npc_combat_snapshot(attacker)->effects&0x800),"Integrated Empathy applied before cast completion");
            cast.update(2100,hooks);check(bool(reactive.npc_combat_snapshot(attacker)->effects&0x800),"Integrated Empathy cast failed to apply NPC hex");
            combat::weapon::Strike strike;strike.level=1;strike.minimum=strike.maximum=10;strike.base_roll=strike.miss_roll=.5;
            check(reactive.damage_quest_player(attacker,caster,128,2101,strike,1000)&&reactive.npc_combat_snapshot(attacker)->health->current==90&&reactive.resources(2)->health==91,
                "Empathy world reaction/reduction not applied to NPC attack");
            output->drain();reactive.tick(7100);const auto expired=output->drain();
            check(!(reactive.npc_combat_snapshot(attacker)->effects&0x800)&&std::find(expired.begin(),expired.end(),emotes::effects_2022(attacker.actor,0))!=expired.end(),"Idle NPC Empathy expiry did not publish status");
            check(reactive.damage_quest_player(attacker,caster,128,7100,strike,1000)&&reactive.npc_combat_snapshot(attacker)->health->current==90&&reactive.resources(2)->health==81,
                "Expired Empathy still reacted or reduced NPC hit");
            check(reactive.empathy_npc(caster,attacker,26,20,7101),"Empathy lethal setup failed");
            check(reactive.damage_quest_player(attacker,caster,128,7102,strike,1000)&&reactive.damage_quest_player(attacker,caster,128,7103,strike,1000)&&reactive.npc_combat_snapshot(attacker)->health->current==0&&reactive.take_npc_deaths(caster).size()==1,
                "Lethal NPC Empathy failed death attribution");
        }
        {
            Instance retaliation;retaliation.configure_npcs({guardian});auto output=std::make_shared<Outbox>();
            retaliation.join(2,"Quest Owner",0,0,output);retaliation.set_vitals(2,{100,100,20,20},100);
            const auto owner=*retaliation.effect_scope(2),guard=*retaliation.effect_scope(100);
            check(retaliation.physical_armor_locations(owner,std::array<int,5>{5,5,0,5,5})&&retaliation.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}),
                  "player-target quest setup failed");
            auto player_policy=policy;player_policy.player_aggro_range=200;
            check(retaliation.drive_quest_guardians(owner,73,1,player_policy,1000),"player chase rejected");
            retaliation.tick(2000);retaliation.drive_quest_guardians(owner,73,1,player_policy,2000);
            output->drain();retaliation.drive_quest_guardians(owner,73,1,player_policy,3000);
            const auto attacks=output->drain();
            check(retaliation.resources(2)->health<100&&retaliation.npc_combat_snapshot(guard)->health->current==100&&
                  std::find(attacks.begin(),attacks.end(),combat::weapon::start_2022(60000,2))!=attacks.end(),
                  "nearby player did not receive NPC damage/presentation or guardian was hit instead");
            auto stale=owner;++stale.generation;combat::weapon::Strike strike;
            strike.level=1;strike.minimum=strike.maximum=10;strike.base_roll=strike.miss_roll=.5;
            check(!retaliation.damage_quest_player(*retaliation.effect_scope(60000),stale,1,3100,strike,1000),
                  "stale player lifetime received quest NPC damage");
            retaliation.move(2,1000,0,3100);retaliation.tick(10000);output->drain();
            check(retaliation.drive_quest_guardians(owner,73,1,player_policy,10000),"lost-player target fallback failed");
            retaliation.tick(11000);retaliation.drive_quest_guardians(owner,73,1,player_policy,11000);
            retaliation.drive_quest_guardians(owner,73,1,player_policy,12000);
            check(retaliation.npc_combat_snapshot(guard)->health->current<100,"out-of-aggro player prevented guardian fallback");
            Instance dying;dying.configure_npcs({guardian});dying.join(2,"Owner",0,0,std::make_shared<Outbox>());
            dying.join(3,"Other Player",0,0,std::make_shared<Outbox>());dying.set_vitals(2,{1,100,20,20},100);
            dying.set_vitals(3,{100,100,20,20},100);const auto dying_owner=*dying.effect_scope(2),dying_guard=*dying.effect_scope(100);
            dying.physical_armor(dying_owner,0);dying.physical_armor(*dying.effect_scope(3),0);
            auto close=enemies;close[0].x=0;
            check(dying.accept_quest_waves(dying_owner,dying_guard,73,1,{close},[]{return true;}),"death-target fixture rejected");
            const auto enemy=*dying.effect_scope(60000);
            check(!dying.damage_quest_player(enemy,*dying.effect_scope(3),1,1000,strike,1000),"quest NPC damaged a foreign owner");
            dying.drive_quest_guardians(dying_owner,73,1,player_policy,1000);
            dying.drive_quest_guardians(dying_owner,73,1,player_policy,2000);
            check(!dying.resources(2)->health&&dying.resources(3)->health==100,"NPC player kill or ownership isolation failed");
            check(!dying.damage_quest_player(enemy,dying_owner,1,2100,strike,1000),"dead player took another quest strike");
            check(dying.drive_quest_guardians(dying_owner,73,1,player_policy,2200),"dead-owner guardian fallback rejected");
            dying.tick(3000);dying.drive_quest_guardians(dying_owner,73,1,player_policy,3000);
            dying.drive_quest_guardians(dying_owner,73,1,player_policy,4000);
            check(dying.npc_combat_snapshot(dying_guard)->health->current<100,"dead player retained NPC target");
        }
        {
            Instance reconnect;reconnect.configure_npcs({guardian});
            auto pending=std::make_shared<Outbox>();
            reconnect.join(2,"Entering",0,0,pending,0,{}, {},0,0,false);
            const auto arriving=*reconnect.effect_scope(2),guard=*reconnect.effect_scope(100);
            std::istringstream shard_input("gw-npcs 1\n# Ice Elemental Shard\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 0f22c16894126e13 -\n");
            auto named=load_npc_catalog(shard_input).for_map(146);
            named[0].combat_definition=enemies[0].combat_definition;
            check(named[0].definition.encoded_name==std::u16string{3874,49512,37906,28179},
                  "catalog changed captured Shard code units");
            check(reconnect.accept_quest_waves(arriving,guard,73,1,{named},[]{return true;}),"restored wave setup failed");
            pending->drain();
            check(reconnect.drive_quest_guardians(arriving,73,1,policy,1000)&&pending->drain().empty(),
                  "AI referenced restored NPC before initial snapshot");
            check(!reconnect.chase_quest_guardian(*reconnect.effect_scope(60000),100,1000),"direct AI bypassed entry gate");
            const auto snapshot=reconnect.npc_entry_packets(2);
            auto remapped=named[0].definition;remapped.id=60000;remapped.encoded_name={u'\x108',u'\x107',u'\x1'};
            const auto definition=std::find(snapshot.begin(),snapshot.end(),npc::properties_2022(remapped));
            check(definition!=snapshot.end()&&std::any_of(definition,snapshot.end(),[](const auto& p){return p.size()>5&&p[0]==32&&p[2]==0x60&&p[3]==0xea;}),
                  "restored Shard captured definition missing or followed spawn");
            check(std::find(snapshot.begin(),snapshot.end(),npc::literal_name_2022(60000,u"Ice Elemental Shard"))!=snapshot.end(),
                  "restored Shard literal name missing");
            check(reconnect.drive_quest_guardians(arriving,73,1,policy,1100)&&!pending->drain().empty(),"AI did not resume after snapshot");
        }
        auto invalid=policy;invalid.evidence.clear();
        check(!ai.drive_quest_guardians(owner,73,1,invalid,1000),"unlabeled AI policy accepted");
        check(!ai.drive_quest_guardians(owner,73,2,policy,1000),"stale cycle drove AI");
        check(ai.drive_quest_guardians(owner,73,1,policy,1000),"AI chase did not start");
        ai.tick(1500);check(ai.drive_quest_guardians(owner,73,1,policy,1500),"AI arrival failed");
        check(ai.npc_combat_snapshot(target)->health->current==100,"AI hit before first attack delay");
        ai.drive_quest_guardians(owner,73,1,policy,2499);
        check(ai.npc_combat_snapshot(target)->health->current==100,"AI cadence fired early");
        ai_box->drain();ai.drive_quest_guardians(owner,73,1,policy,2500);
        const auto strike_packets=ai_box->drain();
        check(strike_packets.size()>=2&&strike_packets[0]==combat::weapon::timing_2022(60000,1.f,1.f)&&
              strike_packets[1]==combat::weapon::start_2022(60000,100),"NPC attack animation lacked policy interval before start");
        check(ai.npc_combat_snapshot(target)->health->current==90,"ordinary guardian strike did not apply physical armor");
        ai.drive_quest_guardians(owner,73,1,policy,10000);
        check(ai.npc_combat_snapshot(target)->health->current==80,"delayed AI poll caused burst attacks");
        for(unsigned i=1;i<=8;++i)ai.drive_quest_guardians(owner,73,1,policy,10000+i*1000);
        check(ai.wave_objective_state(owner,73)==quests::WaveObjective::State::failed&&ai.pending_quest_completions(owner).empty(),
            "automatic guardian death did not fail quest");
        check(!ai.drive_quest_guardians(owner,73,1,policy,20000),"failed quest kept attacking");
    }
    {
        const auto directory=std::filesystem::path(QUEST_CONTENT_DIRECTORY);
        std::ifstream appearances(directory/"actors/presearing-givers.txt"),metadata(directory/"aziure-development-combat.txt");
        auto givers=load_npc_catalog(appearances);
        const std::string giver_hash="aff5dcb7e8f95df444cbffc3ca1623a0dc331b6b667a85deb863953968eadb19";
        bind_combat_catalog(givers,load_catalog(metadata,giver_hash),giver_hash);
        NpcCatalog installed;merge_quest_givers(installed,givers);
        const auto& aziure=installed.for_map(161).front();
        check(aziure.captured_name=="Elementalist Aziure"&&aziure.definition.model_file==116225&&
            aziure.definition.level==5&&aziure.x==-7774&&aziure.y==1246,"guardian capture identity changed");
        Instance world;world.configure_navigation(std::make_shared<navigation::Mesh>(navigation::Mesh::load(
            directory.parent_path()/"navigation"/(std::to_string(map_asset_file(161))+".fwnav"))));
        world.configure_npcs(installed.for_map(161));
        check(world.quest_guardian_available(*world.effect_scope(aziure.id),161),"actual Aziure guardian binding rejected");
    }
    {
        const auto directory=std::filesystem::path(TEST_DIRECTORY);std::filesystem::create_directories(directory);
        const auto path=directory/"guardian-disconnected.fwnav";
        std::ofstream out(path,std::ios::binary);
        auto word=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)out.put(char(n>>(8*i)));};
        auto real=[&](float n){word(std::bit_cast<std::uint32_t>(n));};
        out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})word(n);
        real(0);real(0);real(30);real(10);word(1);word(2);
        for(auto left:{0.f,20.f}){for(unsigned i=0;i<5;++i)word(0xffffffff);
            real(0);real(10);real(left);real(left+10);real(left);real(left+10);}
        out.close();auto mesh=std::make_shared<navigation::Mesh>(navigation::Mesh::load(path));
        for(const auto enemy_x:{5.f,25.f}){
            auto guardian=population.for_map(146).front();guardian.x=4;guardian.y=5;guardian.combat_definition->relation=Relation::neutral;
            auto enemy=population.for_map(146);enemy[0].x=enemy_x;enemy[0].y=5;
            Instance fight;fight.configure_navigation(mesh);fight.configure_npcs({guardian});
            fight.join(2,"Sight Tester",4,5,std::make_shared<Outbox>());fight.set_vitals(2,{100,100,20,20},100);
            const auto owner=*fight.effect_scope(2),target=*fight.effect_scope(100);
            check(fight.quest_guardian_available(target,146)&&!fight.quest_guardian_available(target,161),
                "guardian offer check ignored map or living neutral state");
            auto stale_guardian=target;++stale_guardian.generation;
            check(!fight.quest_guardian_available(stale_guardian,146),"stale guardian enabled quest offer");
            auto invalid_later=enemy;invalid_later[0].x=40;
            bool invalid_navigation_commit=false;
            check(!fight.accept_quest_waves(owner,target,73,1,{enemy,invalid_later},[&]{
                invalid_navigation_commit=true;return true;
            })&&!invalid_navigation_commit&&!fight.wave_objective_state(owner,73),
                "invalid later wave navigation committed acceptance");
            check(fight.begin_wave_objective(owner,target,73,1,1,1)&&
                fight.spawn_quest_wave(owner,73,1,enemy,[]{return true;}),"guardian navigation fixture setup failed");
            check(fight.chase_quest_guardian(*fight.effect_scope(60000),10,800)==(enemy_x==5),
                "guardian chase crossed disconnected banks or rejected valid route");
            fight.tick(900);
            const auto hit=fight.damage_quest_guardian(*fight.effect_scope(60000),target,{combat::Kind::earth,500,1,174},1248,1000);
            if(enemy_x==25){check(hit.reason!=combat::Reason::accepted&&fight.npc_combat_snapshot(target)->health->current==100,
                "guardian damaged across disconnected navigation");}
            else check(hit.killed&&!fight.quest_guardian_available(target,146)&&fight.wave_objective_state(owner,73)==quests::WaveObjective::State::failed,
                "visible guardian attack rejected");
        }
    }
    {
        const std::array hashes{EXPERIMENT_WAVE_1_SHA256,EXPERIMENT_WAVE_2_SHA256,EXPERIMENT_WAVE_3_SHA256};
        const auto directory=std::filesystem::path(QUEST_CONTENT_DIRECTORY);
        std::vector<std::vector<Instance::Npc>> captured_waves;
        std::map<std::string,std::string> verified_populations;
        for(unsigned wave=1;wave<=3;++wave){
            std::ifstream bytes_file(directory/("actors/experiment-wave-"+std::to_string(wave)+".txt"),std::ios::binary);
            verified_populations.emplace(std::string((std::istreambuf_iterator<char>(bytes_file)),{}),hashes[wave-1]);
            std::ifstream appearance(directory/("actors/experiment-wave-"+std::to_string(wave)+".txt"));
            std::ifstream metadata(directory/("experiment-wave-"+std::to_string(wave)+"-development-combat.txt"));
            auto shards=load_npc_catalog(appearance);
            bind_combat_catalog(shards,load_catalog(metadata,hashes[wave-1]),hashes[wave-1]);
            captured_waves.push_back(shards.for_map(161));
            check(shards.for_map(161).size()==4,"captured Experiment wave count changed");
            for(const auto& npc:shards.for_map(161))check(npc.captured_name=="Ice Elemental Shard"&&
                npc.definition.encoded_name==std::u16string{3874,49512,37906,28179}&&
                npc.definition.model_file==129706&&npc.combat_definition->spawn_health->maximum==80,
                "Experiment identity/wiki health changed");
            Instance validation;validation.configure_npcs(shards.for_map(161));
            const auto& definition=*shards.for_map(161).front().combat_definition;
            check(definition.fire_multiplier==2&&definition.fire_multiplier_evidence=="vulnerability","shard fire weakness missing");
            const Lifetime lifetime{1,1,1,161,definition.binding.actor};
            LiveActor fire_target(definition,lifetime),earth_target(definition,lifetime);
            const auto fire=fire_target.apply_damage(lifetime,{combat::Kind::fire,5,1,197},1000);
            const auto earth=earth_target.apply_damage(lifetime,{combat::Kind::earth,5,1,174},1000);
            check(fire.reason==combat::Reason::accepted&&earth.reason==combat::Reason::accepted&&
                std::abs(fire.damage-2*earth.damage)<0.00001,"shard fire multiplier altered nonfire damage");
        }
        std::ifstream wave_manifest_file(directory/"experiment-waves.txt");
        const auto wave_content=load_quest_waves(wave_manifest_file,directory,[&](std::string_view bytes){
            const auto found=verified_populations.find(std::string(bytes));
            return found==verified_populations.end()?std::string{}:found->second;
        });
        check(wave_content.size()==1&&wave_content.at(73).map==161&&wave_content.at(73).waves.size()==3,
            "actual Experiment manifest failed validation");
        captured_waves=wave_content.at(73).waves;
        // Actual captured guardian identity and map geometry, with explicitly
        // authored combat values. This remains separate from live UI proof.
        std::ifstream giver_appearance(directory/"actors/presearing-givers.txt"),giver_metadata(directory/"aziure-development-combat.txt");
        auto givers=load_npc_catalog(giver_appearance);
        const std::string giver_hash="aff5dcb7e8f95df444cbffc3ca1623a0dc331b6b667a85deb863953968eadb19";
        bind_combat_catalog(givers,load_catalog(giver_metadata,giver_hash),giver_hash);
        const auto guardian=givers.for_map(161).front();
        Instance experiment;experiment.configure_navigation(std::make_shared<navigation::Mesh>(navigation::Mesh::load(
            directory.parent_path()/"navigation"/(std::to_string(map_asset_file(161))+".fwnav"))));
        experiment.configure_npcs({guardian});
        experiment.join(2,"Experiment Tester",guardian.x,guardian.y,std::make_shared<Outbox>());
        experiment.set_vitals(2,{100,100,20,20},100);const auto owner=*experiment.effect_scope(2);
        const auto guardian_scope=*experiment.effect_scope(guardian.id);
        check(!experiment.accept_quest_waves(owner,guardian_scope,73,1,captured_waves,[]{return false;})&&
            !experiment.wave_objective_state(owner,73)&&!experiment.effect_scope(60000),
            "failed acceptance leaked wave objective or actors");
        auto malformed=captured_waves;malformed[2][0].combat_definition->binding.map=146;
        bool invalid_commit=false;
        check(!experiment.accept_quest_waves(owner,guardian_scope,73,1,malformed,[&]{invalid_commit=true;return true;})&&
            !invalid_commit,"invalid later wave committed acceptance");
        check(experiment.accept_quest_waves(owner,guardian_scope,73,1,captured_waves,[]{return true;}),
            "atomic captured wave acceptance failed");
        bool duplicate_commit=false;
        check(!experiment.accept_quest_waves(owner,guardian_scope,73,1,captured_waves,[&]{duplicate_commit=true;return true;})&&
            !duplicate_commit,"duplicate wave acceptance committed");
        std::uint64_t now=1000;
        auto incomplete_waves=captured_waves;incomplete_waves.pop_back();
        check(!experiment.advance_quest_waves(owner,73,1,incomplete_waves,[]{return true;}),"partial wave catalog activated");
        for(unsigned wave=0;wave<3;++wave){
            bool stale_commit=false;
            check(!experiment.advance_quest_waves(owner,73,2,captured_waves,[&]{stale_commit=true;return true;})&&
                !stale_commit,"old quest controller published a wave");
            if(wave){
                check(!experiment.advance_quest_waves(owner,73,1,captured_waves,[]{return false;}),"failed wave transaction accepted");
                check(experiment.advance_quest_waves(owner,73,1,captured_waves,[]{return true;}),"captured wave controller failed");
            }
            check(!experiment.advance_quest_waves(owner,73,1,captured_waves,[]{return true;}),"controller skipped living wave");
            check(experiment.pending_quest_completions(owner).empty(),"captured quest completed before allwaves");
            for(unsigned n=0;n<4;++n){
                const auto target=*experiment.effect_scope(60000+wave*4+n);
                if(experiment.npc_combat_snapshot(target)->health->current<=0)continue;
                const auto& npc=captured_waves[wave][n];
                experiment.move(2,npc.x,npc.y,now);experiment.tick(now+90000);now+=90000;
                check(experiment.fire_storm(owner,target,*skills::effects::fire_storm(12),20,now),"captured shard Fire Storm rejected");
                experiment.tick(now+1100);now+=1100;
            }
            check(experiment.wave_objective_state(owner,73)==(wave==2?quests::WaveObjective::State::completed:quests::WaveObjective::State::ready),
                "captured wave deaths did not advance objective");
        }
        check(experiment.pending_quest_completions(owner)==std::vector<Instance::QuestCompletion>{{73,1}},"captured finalwave lost completion");
        const auto deaths=experiment.take_npc_deaths(owner);
        check(deaths.size()==12,"captured wave combat duplicated or lost deaths");
        check(!experiment.advance_quest_waves(owner,73,1,captured_waves,[]{return true;}),"completed controller restarted waves");
        for(const auto& death:deaths)check(death.captured_name=="Ice Elemental Shard","wave death attributed to unrelated NPC");
        experiment.leave(2);experiment.configure_npcs({guardian});
        experiment.join(2,"Experiment Tester",guardian.x,guardian.y,std::make_shared<Outbox>());
        const auto reconnect_owner=*experiment.effect_scope(2),reconnect_guardian=*experiment.effect_scope(guardian.id);
        check(!experiment.restore_completed_wave_objective(owner,reconnect_guardian,161,73,1,3,4),
            "old session restored completed quest");
        check(!experiment.restore_completed_wave_objective(reconnect_owner,guardian_scope,161,73,1,3,4),
            "old guardian restored completed quest");
        check(experiment.restore_completed_wave_objective(reconnect_owner,reconnect_guardian,161,73,1,3,4)&&
            experiment.pending_quest_completions(reconnect_owner)==std::vector<Instance::QuestCompletion>{{73,1}}&&
            !experiment.effect_scope(60000),"completed reconnect lost reward or spawned enemies");
        check(!experiment.advance_quest_waves(reconnect_owner,73,1,captured_waves,[]{return true;}),
            "completed reconnect replayed waves");
        check(experiment.clear_kill_objective(reconnect_owner,73,1,true)&&experiment.pending_quest_completions(reconnect_owner).empty(),
            "reward acknowledgment retained restored objective");
        check(experiment.accept_quest_waves(reconnect_owner,reconnect_guardian,73,2,captured_waves,[]{return true;})&&
            experiment.wave_objective_state(reconnect_owner,73)==quests::WaveObjective::State::fighting,
            "active reconnect could not restart first wave");
        const Instance::GuardianAttackPolicy development_ai{220,128,5,2000,"data/gw/quests/experiment-ai-development-policy.txt"};
        for(std::uint64_t tick=now+1000;tick<=now+61000&&
            experiment.wave_objective_state(reconnect_owner,73)==quests::WaveObjective::State::fighting;tick+=250){
            experiment.tick(tick);
            experiment.drive_quest_guardians(reconnect_owner,73,2,development_ai,tick);
        }
        check(experiment.wave_objective_state(reconnect_owner,73)==quests::WaveObjective::State::failed&&
            experiment.npc_combat_snapshot(reconnect_guardian)->health->current==0&&
            experiment.pending_quest_completions(reconnect_owner).empty(),
            "captured enemies could not navigate to and kill actual guardian");
    }
    {
        auto actors=population.for_map(146);
        for(unsigned id:{101u,102u}){auto copy=actors[0];copy.id=id;copy.combat_definition->binding.actor=id;actors.push_back(copy);}
        Instance waves;waves.configure_npcs(actors);auto output=std::make_shared<Outbox>();
        waves.join(2,"Wave Tester",0,0,output);waves.set_vitals(2,{100,100,20,20},100);
        const auto owner=*waves.effect_scope(2),guardian=*waves.effect_scope(100);
        check(waves.begin_wave_objective(owner,guardian,73,1,2,1),"world wave objective rejected");
        const std::array first{*waves.effect_scope(101)},second{*waves.effect_scope(102)};
        check(waves.bind_wave_objective(owner,73,1,first),"first world wave bind failed");
        check(!waves.begin_kill_objective(owner,73,1,first),"kill objective bypassed wave progression");
        bool conflicting_commit=false;
        check(!waves.spawn_kill_encounter(owner,73,1,population.for_map(146),[&]{conflicting_commit=true;return true;})&&
            !conflicting_commit,"conflicting encounter committed while wave objective active");
        check(waves.damage_npc(owner,first[0],{combat::Kind::earth,100,20,194},1248,1000).killed,"first wave death failed");
        check(waves.wave_objective_state(owner,73)==quests::WaveObjective::State::ready&&
            waves.pending_quest_completions(owner).empty(),"first wave completed whole quest");
        check(waves.bind_wave_objective(owner,73,1,second),"second wave bind failed");
        check(waves.damage_npc(owner,second[0],{combat::Kind::earth,100,20,194},1248,2000).killed&&
            waves.pending_quest_completions(owner)==std::vector<Instance::QuestCompletion>{{73,1}},"final wave lost completion");
        check(waves.damage_npc(owner,guardian,{combat::Kind::earth,100,20,194},1248,3000).killed&&
            waves.pending_quest_completions(owner).empty()&&waves.wave_objective_state(owner,73)==quests::WaveObjective::State::failed,
            "guardian death allowed pending reward");
        check(!waves.clear_kill_objective(owner,73,2)&&waves.clear_kill_objective(owner,73,1),"wave cleanup cycle mismatch");
        // New attempt with a living guardian; failed publication must be retryable.
        auto live_actors=population.for_map(146);Instance activation;activation.configure_npcs(live_actors);
        activation.join(2,"Wave Spawn Tester",0,0,std::make_shared<Outbox>());activation.set_vitals(2,{100,100,20,20},100);
        const auto active_owner=*activation.effect_scope(2);
        check(activation.begin_wave_objective(active_owner,*activation.effect_scope(100),73,2,2,1),"spawn wave setup failed");
        auto foreign_wave=live_actors;foreign_wave[0].combat_definition->binding.map=161;bool foreign_commit=false;
        check(!activation.spawn_quest_wave(active_owner,73,2,foreign_wave,[&]{foreign_commit=true;return true;})&&
            !foreign_commit&&!activation.effect_scope(60000),"foreign-map wave bypassed guardian scope");
        check(!activation.spawn_quest_wave(active_owner,73,2,live_actors,[]{return false;})&&
            !activation.effect_scope(60000)&&activation.wave_objective_state(active_owner,73)==quests::WaveObjective::State::ready,
            "failed wave exposed actors or consumed readiness");
        check(activation.spawn_quest_wave(active_owner,73,2,live_actors,[]{return true;}),"first atomic wave failed");
        check(!activation.spawn_quest_wave(active_owner,73,2,live_actors,[]{return true;}),"spawned next wave before deaths");
        check(activation.damage_npc(active_owner,*activation.effect_scope(60000),{combat::Kind::earth,100,20,194},1248,1000).killed,
            "spawned wave NPC did not die");
        check(activation.spawn_quest_wave(active_owner,73,2,live_actors,[]{return true;})&&activation.effect_scope(60001),
            "second atomic wave failed or reused lifetime");
        check(activation.damage_npc(active_owner,*activation.effect_scope(60001),{combat::Kind::earth,100,20,194},1248,2000).killed&&
            activation.pending_quest_completions(active_owner)==std::vector<Instance::QuestCompletion>{{73,2}},"atomic wave final completion failed");
        check(activation.clear_kill_objective(active_owner,73,2,true)&&!activation.effect_scope(60000)&&!activation.effect_scope(60001),
            "wave cleanup leaked spawned actors");
        auto neutral_guardian=population.for_map(146).front();neutral_guardian.combat_definition->relation=Relation::neutral;
        Instance threatened;threatened.configure_npcs({neutral_guardian});
        threatened.join(2,"Guardian Tester",0,0,std::make_shared<Outbox>());threatened.set_vitals(2,{100,100,20,20},100);
        const auto threatened_owner=*threatened.effect_scope(2),guard=*threatened.effect_scope(100);
        auto enemy_placement=population.for_map(146);enemy_placement[0].x=100;
        check(threatened.begin_wave_objective(threatened_owner,guard,73,1,3,1)&&
            threatened.spawn_quest_wave(threatened_owner,73,1,enemy_placement,[]{return true;}),"guardian attack setup failed");
        auto enemy=*threatened.effect_scope(60000);auto stale_enemy=enemy;++stale_enemy.generation;
        check(threatened.damage_quest_guardian(enemy,guard,{combat::Kind::earth,500,1,174},49,1000).reason!=combat::Reason::accepted,
            "guardian attacked beyond range");
        check(threatened.damage_quest_guardian(enemy,guard,{combat::Kind::earth,500,20,174},1248,1000).reason!=combat::Reason::accepted,
            "guardian attacker invented caster level");
        check(threatened.damage_quest_guardian(stale_enemy,guard,{combat::Kind::earth,500,1,174},1248,1000).reason!=combat::Reason::accepted,
            "stale enemy damaged guardian");
        check(threatened.damage_npc(threatened_owner,guard,{combat::Kind::earth,500,20,174},1248,1000).reason!=combat::Reason::accepted,
            "player damaged neutral guardian");
        check(!threatened.chase_quest_guardian(stale_enemy,100,1000)&&
            threatened.chase_quest_guardian(enemy,100,1000)&&!threatened.chase_quest_guardian(enemy,100,1000),
            "chase accepted stale or duplicate intent");
        threatened.tick(1250);
        check(threatened.damage_quest_guardian(enemy,guard,{combat::Kind::earth,500,1,174},20,1250).reason!=combat::Reason::accepted,
            "chase teleported before travel duration");
        threatened.tick(1500);
        check(threatened.damage_quest_guardian(enemy,guard,{combat::Kind::earth,500,1,174},1,1500).killed&&
            threatened.wave_objective_state(threatened_owner,73)==quests::WaveObjective::State::failed&&
            threatened.pending_quest_completions(threatened_owner).empty(),"enemy guardian death did not fail quest");
    }
    {
        auto base=population;auto supplement=population;
        supplement.maps.at(146)[0].combat_definition.reset();
        merge_quest_givers(base,supplement);
        check(base.for_map(146).size()==2&&base.for_map(146)[1].id==101&&base.for_map(146)[1].definition.id==2,
            "isolated quest giver IDs collided with base population");
        check(base.for_map(146)[0].combat_definition->binding==population.for_map(146)[0].combat_definition->binding,
            "quest giver merge changed base combat binding");
        const auto count=base.for_map(146).size();bool rejected=false;
        try{merge_quest_givers(base,population);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected&&base.for_map(146).size()==count,"rejected quest giver supplement partially installed");
    }
    check(population.for_map(146).front().combat_definition.has_value(),"host dropped validated metadata");
    Instance instance;instance.configure_npcs(population.for_map(146));
    const auto spawn=instance.npc_packets();
    check(spawn.size()>3&&spawn[0][0]==85&&spawn[1][0]==86&&spawn[2][0]==32,"health referenced NPC before spawn");
    const auto target=*instance.effect_scope(100);
    const auto box=std::make_shared<Outbox>();instance.join(2,"Quest Tester",0,0,box);
    instance.set_vitals(2,{100,100,20,20},100);const auto owner=*instance.effect_scope(2);box->drain();
    auto stale=target;++stale.generation;
    check(instance.damage_npc(owner,stale,{combat::Kind::earth,40,20,194},1248,0).reason==combat::Reason::invalid,"stale target damaged NPC");
    check(instance.damage_npc(owner,target,{combat::Kind::earth,40,20,194},10,0).reason==combat::Reason::invalid,"out of range hit accepted");
    auto first=instance.damage_npc(owner,target,{combat::Kind::earth,40,20,194},1248,0);
    check(first.reason==combat::Reason::accepted&&!first.killed&&first.after.current==60,"authoritative NPC damage failed");
    check(instance.npc_combat_snapshot(target)->health->current==60&&!box->drain().empty(),"damage did not reach state/client");
    check(instance.take_npc_deaths(owner).empty(),"nonlethal hit completed objective");
    check(instance.npc_packets()!=spawn,"reconnect snapshot restored spawn health");
    const auto lethal=instance.damage_npc(owner,target,{combat::Kind::earth,100,20,194},1248,1);
    check(lethal.killed&&instance.npc_combat_snapshot(target)->health->current==0,"NPC did not die");
    auto stale_owner=owner;++stale_owner.generation;
    check(instance.take_npc_deaths(stale_owner).empty(),"stale player consumed quest death");
    const auto deaths=instance.take_npc_deaths(owner);
    check(deaths.size()==1&&deaths.front().target==target&&deaths.front().binding.map==146&&
          deaths.front().captured_name=="Synthetic quest enemy","death lost objective identity");
    check(instance.take_npc_deaths(owner).empty(),"death objective delivered twice");
    check(instance.damage_npc(owner,target,{combat::Kind::earth,100,20,194},1248,2).reason==combat::Reason::dead&&
          instance.take_npc_deaths(owner).empty(),"dead actor farmed another kill");
    {
        Instance loot;loot.configure_npcs(population.for_map(146));
        const auto output=std::make_shared<Outbox>();loot.join(2,"Loot Tester",0,0,output);
        loot.set_vitals(2,{100,100,20,20},100);output->drain();
        const auto killer=*loot.effect_scope(2),enemy=*loot.effect_scope(100);
        auto definition=inventory::presearing::shimmering_scale_definition();
        check(!loot.loot_npc_death(killer,enemy,definition,0,60000),"living NPC granted loot");
        const std::array assigned{enemy};unsigned accepts=0;
        auto stale_enemy=enemy;++stale_enemy.generation;
        const std::array stale_targets{stale_enemy};
        check(!loot.begin_quest_loot(killer,83,1,stale_targets,0,[&]{++accepts;return true;})&&accepts==0,
              "stale loot assignment reached acceptance");
        check(!loot.begin_quest_loot(killer,83,1,assigned,0,[]{return false;}),"failed acceptance armed loot");
        check(loot.begin_quest_loot(killer,83,1,assigned,0,[&]{++accepts;return true;})&&accepts==1,
              "reviewed living encounter failed to arm loot");
        check(!loot.begin_quest_loot(killer,83,2,assigned,0,[&]{++accepts;return true;})&&accepts==1,
              "existing loot attempt overwritten");
        check(loot.damage_npc(killer,enemy,{combat::Kind::earth,200,20,194},1248,1).killed,"loot fixture not killed");
        check(loot.pending_quest_loot(killer,83,1).size()==1&&loot.pending_quest_loot(killer,83,2).empty(),
              "pending loot did not filter quest attempt");
        auto obsolete=killer;++obsolete.generation;
        check(!loot.loot_npc_death(obsolete,enemy,definition,1,60000),"stale killer granted loot");
        auto invalid=definition;invalid.file=0;
        check(!loot.loot_npc_death(killer,enemy,invalid,1,60000)&&loot.ground_item_count()==0,"invalid loot published");
        check(loot.pending_quest_loot(killer,83,1).size()==1,"failed loot consumed retry event");
        check(!loot.loot_npc_death(killer,enemy,definition,1,60000,83,2)&&
              !loot.loot_npc_death(killer,enemy,definition,1,60000,85,1)&&
              !loot.clear_quest_loot(killer,83,2),"quest loot ignored cycle or quest binding");
        const auto dropped=loot.loot_npc_death(killer,enemy,definition,1,60000,83,1);
        check(dropped&&dropped->source==enemy&&dropped->allocation->owner==killer&&
              dropped->position.x==population.for_map(146).front().x&&dropped->position.y==population.for_map(146).front().y,
              "loot lost corpse position or owner");
        check(!loot.loot_npc_death(killer,enemy,definition,2,60000)&&loot.ground_item_count()==1&&
              loot.take_npc_deaths(killer).empty(),"NPC death generated duplicate loot");
        check(loot.clear_quest_loot(killer,83,1)&&!loot.clear_quest_loot(killer,83,1),"quest loot cleanup fence failed");
    }
    {
        Instance encounter;const auto output=std::make_shared<Outbox>();encounter.join(2,"Collection Tester",0,0,output);
        encounter.set_vitals(2,{100,100,20,20},100);const auto collector=*encounter.effect_scope(2);output->drain();
        check(!encounter.spawn_loot_encounter(collector,83,1,population.for_map(146),0,[]{return false;})&&
              !encounter.effect_scope(60000),"failed collection acceptance exposed enemies");
        check(encounter.spawn_loot_encounter(collector,83,1,population.for_map(146),0,[]{return true;}),
              "atomic collection encounter did not spawn");
        const auto enemy=*encounter.effect_scope(60000);
        check(encounter.damage_npc(collector,enemy,{combat::Kind::earth,200,20,194},1248,1).killed&&
              encounter.pending_quest_completions(collector).empty(),"collection completed by kill instead of possession");
        check(encounter.pending_quest_loot(collector,83,1).size()==1&&
              encounter.loot_npc_death(collector,enemy,inventory::presearing::shimmering_scale_definition(),1,60000,83,1).has_value(),
              "spawned collection NPC lost loot binding");
        check(encounter.clear_quest_loot(collector,83,1)&&!encounter.effect_scope(60000),"collection cleanup leaked encounter NPC");
    }
    instance.leave(2);instance.configure_npcs(population.for_map(146));
    const auto respawn=*instance.effect_scope(100);
    check(respawn!=target&&!instance.npc_combat_snapshot(target)&&instance.npc_combat_snapshot(respawn)->health->current==100,"replacement reused stale lifetime");
    auto bad=population.for_map(146);bad.front().combat_definition->binding.model_file++;
    bool rejected=false;try{instance.configure_npcs(bad);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected&&instance.effect_scope(100)==respawn,"rejected replacement mutated instance");
    instance.join(2,"Quest Tester",0,0,box);instance.set_vitals(2,{100,100,20,20},100);
    const auto caster=*instance.effect_scope(2);
    check(instance.projectile_target_valid(caster,100,1248,10),"NPC cannot be selected by caster");
    const auto flare=*skills::effects::elemental(194,12);
    check(!instance.elemental(caster,target,flare,20,10),"Flare accepted replaced NPC lifetime");
    skills::Policy policy;policy.primary=6;policy.campaigns=0x1e;policy.learned={{194,1}};
    skills::Slots slots{};slots[0]={194,0};attributes::Ranks ranks{};ranks[10]=12;
    skills::casting::Hooks hooks;
    hooks.resources=[&](unsigned id){return instance.resources(id);};
    hooks.consume_energy=[&](unsigned amount){return instance.consume_energy(2,amount);};
    hooks.events=[&](const auto& packets){box->enqueue(packets);};
    hooks.target_valid=[](auto,float,bool){return false;};
    hooks.projectile_target_valid=[&](unsigned id,float range){return instance.projectile_target_valid(caster,id,range,10);};
    hooks.target_scope=[&](unsigned id){return instance.effect_scope(id);};
    hooks.elemental=[&](skills::effects::Scope target,const auto& plan,std::uint64_t now){return instance.elemental(caster,target,plan,20,now);};
    skills::casting::Runtime casting(2);
    check(!instance.resources(100),"test fabricated NPC player energy profile");
    check(casting.start(10,{194,0,100},slots,policy,ranks,true,hooks)==skills::casting::Reason::accepted,"Flare cast did not start at NPC");
    casting.update(1010,hooks);
    check(instance.npc_combat_snapshot(respawn)->health->current==100,"Flare skipped projectile flight");
    instance.tick(2000);
    check(instance.npc_combat_snapshot(respawn)->health->current==44&&instance.take_npc_deaths(caster).empty(),"Flare impact/armor incorrect");
    check(casting.start(2500,{194,0,100},slots,policy,ranks,true,hooks)==skills::casting::Reason::accepted,"second Flare failed");
    casting.update(3500,hooks);instance.tick(4000);
    check(instance.npc_combat_snapshot(respawn)->health->current==0&&instance.take_npc_deaths(caster).size()==1,"Flare kill not available to quest controller");
    instance.leave(2);
    auto unknown_armor=population.for_map(146);unknown_armor.front().combat_definition->fire_armor.reset();
    instance.configure_npcs(unknown_armor);instance.join(2,"Quest Tester",0,0,box);instance.set_vitals(2,{100,100,20,20},100);
    check(!instance.elemental(*instance.effect_scope(2),*instance.effect_scope(100),flare,20,0),"Flare invented NPC fire armor");
    instance.leave(2);
    auto encounter=population.for_map(146);
    for(unsigned id:{101u,102u}){auto copy=encounter.front();copy.id=id;copy.combat_definition->binding.actor=id;encounter.push_back(copy);}
    instance.configure_npcs(encounter);instance.join(2,"Quest Tester",0,0,box);instance.set_vitals(2,{100,100,20,20},100);
    const auto quest_owner=*instance.effect_scope(2);
    const std::array group{*instance.effect_scope(100),*instance.effect_scope(101)};
    const std::array duplicate{group[0],group[0]};
    check(!instance.begin_kill_objective(quest_owner,85,1,duplicate),"duplicate objective inflated kill requirement");
    check(instance.begin_kill_objective(quest_owner,85,1,group),"reviewed encounter did not bind");
    const std::array wrong_group{*instance.effect_scope(102)};
    check(!instance.begin_kill_objective(quest_owner,85,1,wrong_group),"active objective replaced with easier group");
    check(instance.damage_npc(quest_owner,wrong_group[0],{combat::Kind::earth,200,20,194},1248,0).killed&&
          instance.pending_quest_completions(quest_owner).empty(),"same-model unrelated kill counted");
    check(instance.damage_npc(quest_owner,group[0],{combat::Kind::earth,200,20,194},1248,1).killed&&
          instance.pending_quest_completions(quest_owner).empty(),"quest completed with remaining actor alive");
    check(instance.begin_kill_objective(quest_owner,85,1,group),"idempotent bind reset/rejected partial progress");
    check(instance.damage_npc(quest_owner,group[1],{combat::Kind::earth,200,20,194},1248,2).killed,"last encounter actor survived");
    const auto completion=instance.pending_quest_completions(quest_owner);
    check(completion==std::vector<Instance::QuestCompletion>{{85,1}}&&
          instance.pending_quest_completions(quest_owner)==completion,"completion lost before storage acknowledgment");
    check(!instance.clear_kill_objective(quest_owner,85,2)&&instance.pending_quest_completions(quest_owner)==completion,"different attempt acknowledged completion");
    check(instance.clear_kill_objective(quest_owner,85,1)&&instance.pending_quest_completions(quest_owner).empty(),"committed completion remained queued");
    const auto baseline=instance.npc_packets();unsigned commits=0;
    auto templates=population.for_map(146);
    check(!instance.spawn_kill_encounter(quest_owner,85,2,templates,[&]{++commits;return false;})&&
          commits==1&&!instance.effect_scope(60000)&&instance.npc_packets()==baseline,"failed acceptance exposed encounter");
    auto invalid=templates;invalid[0].definition.visual_adjustment[3]=0;
    check(!instance.spawn_kill_encounter(quest_owner,85,2,invalid,[&]{++commits;return true;})&&commits==1,"invalid placement/definition called storage");
    check(instance.spawn_kill_encounter(quest_owner,85,2,templates,[&]{++commits;return true;})&&commits==2,"accepted quest did not activate encounter");
    const auto spawned=*instance.effect_scope(60000);
    check(instance.npc_combat_snapshot(spawned)->health->current==100&&!box->drain().empty(),"spawn health/packets missing");
    check(!instance.spawn_kill_encounter(quest_owner,85,2,templates,[&]{++commits;return true;})&&commits==2,"duplicate acceptance spawned another group");
    check(instance.damage_npc(quest_owner,spawned,{combat::Kind::earth,200,20,194},1248,3).killed&&
          instance.pending_quest_completions(quest_owner)==std::vector<Instance::QuestCompletion>{{85,2}},"spawned group not bound to objective");
    check(instance.clear_kill_objective(quest_owner,85,2,true)&&!instance.effect_scope(60000)&&
          instance.npc_packets()==baseline,"abandonment retained creatures in reconnect snapshot");
    check(instance.spawn_kill_encounter(quest_owner,85,3,templates,[]{return true;}),"reaccept failed");
    const auto reaccepted=*instance.effect_scope(60001);
    check(reaccepted.actor!=spawned.actor&&!instance.npc_combat_snapshot(spawned),"reaccept reused actor identity");
    instance.leave(2);check(!instance.effect_scope(reaccepted.actor)&&instance.npc_packets()==baseline,"disconnect leaked encounter");
    {
        std::istringstream physical_metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+
            " tests/npc_world_combat.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 35 35 60 1 0 test test test - - 60 - test - 60 test\n");
        auto sword_population=population;
        bind_combat_catalog(sword_population,load_catalog(physical_metadata,hash),hash);
        {
            // Explicit synthetic lightning AR fixture, not inferred retail data.
            auto enemies=sword_population.for_map(146);enemies[0].x=100;enemies[0].y=0;
            enemies[0].combat_definition->lightning_armor=80;
            enemies[0].combat_definition->lightning_armor_evidence="test";
            Instance javelin;auto out=std::make_shared<Outbox>();javelin.join(2,"Elementalist",0,0,out);
            javelin.set_vitals(2,{100,100,20,20},100);const auto caster=*javelin.effect_scope(2);
            check(javelin.configure_projectiles({100,2})&&javelin.spawn_kill_encounter(caster,85,1,enemies,[]{return true;}),"NPC Javelin fixture failed");
            const auto target=*javelin.effect_scope(60000);const auto plan=*skills::effects::lightning_javelin(0);
            check(javelin.lightning_javelin(caster,target,plan,20,5,100),"NPC Javelin launch rejected");
            javelin.tick(1000);check(javelin.npc_combat_snapshot(target)->health->current==35,"NPC Javelin hit early");
            javelin.tick(1100);check(javelin.npc_combat_snapshot(target)->health->current==20,"NPC Javelin damage or penetration wrong");
            check(javelin.lightning_javelin(caster,target,plan,20,5,1200),"second NPC Javelin rejected");javelin.tick(2200);
            check(javelin.npc_combat_snapshot(target)->health->current==5,"second NPC Javelin damage wrong");
            check(javelin.lightning_javelin(caster,target,plan,20,5,2300),"third NPC Javelin rejected");javelin.tick(3300);
            check(javelin.npc_combat_snapshot(target)->health->current==0&&javelin.pending_quest_completions(caster)==std::vector<Instance::QuestCompletion>{{85,1}},"NPC Javelin did not complete quest objective");
            check(javelin.take_npc_deaths(caster).size()==1,"NPC Javelin death attribution lost");javelin.tick(3400);check(javelin.take_npc_deaths(caster).empty(),"NPC Javelin duplicate death");
            Instance stale;stale.configure_npcs(enemies);auto box=std::make_shared<Outbox>();stale.join(2,"Old Caster",0,0,box);
            stale.set_vitals(2,{100,100,20,20},100);const auto old_caster=*stale.effect_scope(2),permanent=*stale.effect_scope(100);
            check(stale.configure_projectiles({100,2})&&stale.lightning_javelin(old_caster,permanent,plan,20,5,100),"stale Javelin fixture failed");
            stale.leave(2);stale.join(2,"New Caster",0,0,box);stale.set_vitals(2,{100,100,20,20},100);
            stale.tick(1100);check(stale.npc_combat_snapshot(permanent)->health->current==35&&stale.configure_projectiles({100,2}),"reconnected caster retained old Javelin");
            auto unknown=enemies;unknown[0].combat_definition->lightning_armor.reset();Instance missing;missing.configure_npcs(unknown);missing.join(2,"Caster",0,0,box);missing.set_vitals(2,{100,100,20,20},100);
            check(!missing.lightning_javelin(*missing.effect_scope(2),*missing.effect_scope(100),plan,20,5,100),"Javelin invented NPC lightning armor");
        }
        {
            Instance accelerated;auto speed_out=std::make_shared<Outbox>();accelerated.join(2,"Warrior",0,0,speed_out);
            accelerated.set_vitals(2,{100,100,20,20},100);const auto owner=*accelerated.effect_scope(2);
            combat::weapon::Owned weapon{3,1,{20,20,2},12,false,false,27};
            check(accelerated.ordinary_weapon(owner,weapon,20)&&
                accelerated.spawn_kill_encounter(owner,85,1,sword_population.for_map(146),[]{return true;}),"Frenzy fixture failed");
            const auto target=*accelerated.effect_scope(60000);
            check(accelerated.ordinary_attack(owner,target.actor,100),"Frenzy attack rejected");
            check(accelerated.status_mutation(owner,346,1,0,350,8350,true),"Frenzy status rejected");
            accelerated.tick(350);accelerated.tick(450);
            check(accelerated.npc_combat_snapshot(target)->health->current==35,"Frenzy released early");
            accelerated.tick(550);check(accelerated.npc_combat_snapshot(target)->health->current==15,"Frenzy did not accelerate current windup");
            check(accelerated.status_mutation(owner,346,1,0,650,{},false),"Frenzy removal failed");
            accelerated.tick(650);accelerated.tick(1730);
            check(accelerated.npc_combat_snapshot(target)->health->current==15,"Frenzy removal retained accelerated repeat");
            accelerated.tick(1831);check(accelerated.npc_combat_snapshot(target)->health->current==0,"Frenzy removal lost ordinary repeat");
        }
        auto physical_missing="gw-npc-combat 3 "+hash+"\nsource test authored "+hash+
            " tests/npc_world_combat.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 35 35 60 1 0 test test test - - 60 - test - 60 -\n";
        bool denied=false;try{std::istringstream in(physical_missing);load_catalog(in,hash);}catch(const std::invalid_argument&){denied=true;}
        check(denied,"physical armor accepted without evidence");
        Instance melee;auto out=std::make_shared<Outbox>();melee.join(2,"Warrior",0,0,out);
        melee.set_vitals(2,{100,100,20,20},100);const auto attacker=*melee.effect_scope(2);
        combat::weapon::Owned sword{3,1,{20,20,2},12,false,false,27};
        check(melee.ordinary_weapon(attacker,sword,20),"sword equip failed");
        check(melee.spawn_kill_encounter(attacker,85,1,sword_population.for_map(146),[]{return true;}),"melee encounter failed");
        const auto tad=*melee.effect_scope(60000);out->drain();
        check(melee.ordinary_attack(attacker,tad.actor,100),"ordinary NPC sword attack rejected");
        melee.tick(500);check(melee.npc_combat_snapshot(tad)->health->current==35,"NPC attack skipped windup");
        melee.tick(600);check(melee.npc_combat_snapshot(tad)->health->current==15&&melee.pending_quest_completions(attacker).empty(),"NPC sword damage failed");
        melee.tick(1930);check(melee.npc_combat_snapshot(tad)->health->current==0&&
            melee.pending_quest_completions(attacker)==std::vector<Instance::QuestCompletion>{{85,1}},"ordinary attack did not complete encounter");
        check(melee.take_npc_deaths(attacker).size()==1,"ordinary attack lost death identity");
        melee.tick(4000);check(melee.take_npc_deaths(attacker).empty(),"ordinary attack farmed dead NPC");
        check(!melee.ordinary_attack(attacker,tad.actor,4001),"dead target accepted attack");
        melee.clear_kill_objective(attacker,85,1,true);
        auto unreviewed=sword_population.for_map(146);unreviewed[0].combat_definition->physical_armor.reset();
        check(melee.spawn_kill_encounter(attacker,85,2,unreviewed,[]{return true;}),"unknown-armor encounter failed");
        check(!melee.ordinary_attack(attacker,60001,4100),"ordinary attack invented physical armor");
        melee.clear_kill_objective(attacker,85,2,true);
        check(melee.spawn_kill_encounter(attacker,85,3,sword_population.for_map(146),[]{return true;}),"caster encounter failed");
        const auto ranged_target=*melee.effect_scope(60002);
        combat::weapon::Owned wand{4,1,{16,16,11},0,false,true,26};
        check(melee.ordinary_weapon(attacker,wand,20)&&melee.configure_projectiles({100,32})&&
            melee.ordinary_attack(attacker,ranged_target.actor,5000),"NPC wand attack rejected");
        melee.tick(5500);check(melee.npc_combat_snapshot(ranged_target)->health->current==35,"wand skipped flight");
        melee.tick(6000);check(melee.npc_combat_snapshot(ranged_target)->health->current==19,"wand NPC impact failed");
        check(melee.ordinary_attack(attacker,ranged_target.actor,6500),"second wand attack rejected");
        melee.tick(7000);melee.clear_kill_objective(attacker,85,3,true);
        check(melee.spawn_kill_encounter(attacker,85,4,sword_population.for_map(146),[]{return true;}),"replacement encounter failed");
        const auto replacement=*melee.effect_scope(60003);melee.tick(8000);
        check(melee.npc_combat_snapshot(replacement)->health->current==35,"old projectile hit replacement");
    }
    const std::filesystem::path fixtures=TEST_DIRECTORY;std::filesystem::create_directories(fixtures);
    {
        const auto directory=std::filesystem::path(QUEST_CONTENT_DIRECTORY);
        const std::string queen_hash="dcd0f2629e5394771ec46fe8aca78e523394e6c532fdb66ef469bdc01d124797";
        std::ifstream source(directory/"actors/ranger-test-queen.txt",std::ios::binary);
        const std::string captured((std::istreambuf_iterator<char>(source)),{});
        std::istringstream manifest("gw-quest-encounters 3\nkill 86 146 actors/ranger-test-queen.txt ranger-test-development-combat.txt "+queen_hash+"\n");
        const auto encounters=load_quest_encounters(manifest,directory,[&](std::string_view bytes){return bytes==captured?queen_hash:std::string{};});
        const auto& encounter=encounters.at(86);check(encounter.actors.size()==1&&encounter.actors.front().captured_name=="River Skale Queen","Ranger quest queen identity lost");
        Instance ranger;ranger.configure_navigation(std::make_shared<navigation::Mesh>(navigation::Mesh::load(directory.parent_path()/"navigation"/(std::to_string(map_asset_file(146))+".fwnav"))));
        const auto& queen=encounter.actors.front();ranger.join(2,"Ranger Trial",queen.x,queen.y,std::make_shared<Outbox>());ranger.set_vitals(2,{100,100,20,20},100);
        const auto owner=*ranger.effect_scope(2);unsigned commits=0;
        check(accept_kill_encounter(ranger,owner,146,86,1,encounter,[&]{++commits;return true;})&&commits==1,"Ranger acceptance failed");
        const auto snapshot=ranger.npc_packets();
        const auto literal=npc::literal_name_2022(60000,u"River Skale Queen");
        check(std::find(snapshot.begin(),snapshot.end(),literal)!=snapshot.end(),"captured encounter name lacks safe2022 publication");
        check(!queen.definition.encoded_name.empty(),"source localized name was discarded");
        const auto gear=inventory::presearing::creation_weapon(inventory::baseline(),2,u"Ranger Trial");
        const auto bow=combat::weapon::owned_weapon(*gear,1,{2,"Ranger Trial",{},true,{}});
        check(bow&&ranger.ordinary_weapon(owner,*bow,1)&&ranger.ordinary_attack(owner,60000,100),"Ranger queen bow attack failed");
        for(unsigned i=0;i<40;++i){ranger.tick(600+i*2475);ranger.tick(700+i*2475);}
        const auto completed=ranger.pending_quest_completions(owner);
        check(completed.size()==1&&completed.front().quest==86,"captured Ranger queen did not complete quest");
    }
    {
        const auto directory=std::filesystem::path(QUEST_CONTENT_DIRECTORY)/"development-elementalist";
        std::ifstream actors(directory/"actors.txt",std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(actors)),{});
        std::ifstream manifest(directory/"encounters.txt");
        std::map<std::uint32_t,NpcCatalog::KillEncounter> collections;
        load_quest_encounters(manifest,directory,[&](std::string_view input){
            return input==bytes?"aa5207c1a1cba7317cd71a5f641aef3ea6d6cef182199ad274f09c5eea1f2e23":std::string{};
        },&collections);
        check(collections.size()==1&&collections.at(83).actors.size()==3,"development collection lacks three assigned Tads");
        Instance placement;placement.configure_navigation(std::make_shared<navigation::Mesh>(navigation::Mesh::load(
            directory.parent_path().parent_path()/"navigation"/(std::to_string(map_asset_file(146))+".fwnav"))));
        placement.configure_npcs(collections.at(83).actors);
        check(placement.effect_scope(30001).has_value()&&placement.effect_scope(30003).has_value(),"development collection placement failed");
        // Exercise the installed authored encounter and real navigation mesh,
        // using the exact creation weapon rather than a synthetic damage hit.
        // Starting beside the first Tad is a fixture, not a recovered spawn.
        const auto& first=collections.at(83).actors.front();
        Instance first_quest;first_quest.configure_navigation(std::make_shared<navigation::Mesh>(navigation::Mesh::load(
            directory.parent_path().parent_path()/"navigation"/(std::to_string(map_asset_file(146))+".fwnav"))));
        first_quest.join(2,"Rod Quest Tester",first.x,first.y,std::make_shared<Outbox>());
        first_quest.set_vitals(2,{100,100,20,20},100);const auto owner=*first_quest.effect_scope(2);
        const auto gear=inventory::presearing::creation_weapon(inventory::baseline(),6,u"Rod Quest Tester");
        check(gear.has_value(),"installed Elementalist creation gear unavailable");
        const auto rod=combat::weapon::owned_weapon(*gear,1,{6,"Rod Quest Tester",{},true,{}});
        check(rod&&first_quest.ordinary_weapon(owner,*rod,1),"installed Elementalist creation rod unavailable");
        check(first_quest.spawn_loot_encounter(owner,83,1,collections.at(83).actors,100,[]{return true;}),
              "installed Elementalist encounter acceptance failed");
        const auto tad=*first_quest.effect_scope(60000);
        check(first_quest.ordinary_attack(owner,tad.actor,100),"installed Tad rejected starter rod attack");
        for(std::uint64_t now=200;now<=20100;now+=100)first_quest.tick(now);
        check(first_quest.npc_combat_snapshot(tad)->health->current==0&&
              first_quest.pending_quest_completions(owner).empty(),"installed rod kill bypassed collection possession");
        check(first_quest.pending_quest_loot(owner,83,1).size()==1,"installed rod kill lost scale objective source");
        const auto scale=first_quest.loot_npc_death(owner,tad,inventory::presearing::shimmering_scale_definition(),20100,60000,83,1);
        check(scale&&scale->source==tad&&scale->allocation->owner==owner&&first_quest.ground_item_count()==1,
              "installed rod kill did not produce owned quest scale");
        check(!first_quest.loot_npc_death(owner,tad,inventory::presearing::shimmering_scale_definition(),20200,60000,83,1),
              "installed Tad produced a duplicate scale");
    }
    {
        const auto directory=std::filesystem::path(QUEST_CONTENT_DIRECTORY);
        const std::string source="67549b59a78d10cd3f2601c0294698931ac0dc31eb4ecfce5f5057ed7af3e10c";
        std::ifstream appearances(directory/"actors/warrior-test-tads.txt"),metadata(directory/"warrior-test-development-combat.txt");
        auto population=load_npc_catalog(appearances);bind_combat_catalog(population,load_catalog(metadata,source),source);
        const auto& actors=population.for_map(146);check(actors.size()==3,"installed Warrior Test lacks three Tads");
        Instance quest;quest.configure_navigation(std::make_shared<navigation::Mesh>(navigation::Mesh::load(
            directory.parent_path()/"navigation"/(std::to_string(map_asset_file(146))+".fwnav"))));
        quest.join(2,"Hammer Quest Tester",actors.front().x,actors.front().y,std::make_shared<Outbox>());
        quest.set_vitals(2,{100,100,20,20},100);const auto owner=*quest.effect_scope(2);
        const auto gear=inventory::presearing::creation_weapon(inventory::baseline(),1,u"Hammer Quest Tester");
        check(gear.has_value(),"installed Warrior creation equipment unavailable");
        const auto hammer=combat::weapon::owned_weapon(*gear,1,{1,"Hammer Quest Tester",{},false,19});
        check(hammer&&quest.ordinary_weapon(owner,*hammer,1),"installed Warrior starter hammer unavailable");
        check(quest.spawn_kill_encounter(owner,85,1,actors,[]{return true;}),"installed Warrior Test acceptance failed");
        for(unsigned n=0;n<3;++n){
            const auto target=*quest.effect_scope(60000+n);const auto start=1000ull+n*100000ull;
            quest.move(2,actors[n].x,actors[n].y,start);quest.tick(start+80000);
            check(quest.ordinary_attack(owner,target.actor,start+80001),"installed Warrior Tad rejected starter hammer");
            for(auto now=start+80100;now<=start+99900;now+=100)quest.tick(now);
            check(quest.npc_combat_snapshot(target)->health->current==0,"starter hammer did not kill installed Warrior Tad");
            check(quest.pending_quest_completions(owner).size()==unsigned(n==2),"Warrior Test completed before all three hammer kills");
        }
        check(quest.pending_quest_completions(owner)==std::vector<Instance::QuestCompletion>{{85,1}},"installed Warrior completion identity wrong");
        check(quest.take_npc_deaths(owner).size()==3,"installed Warrior deaths duplicated or missing");
        quest.leave(2);check(!quest.effect_scope(60000),"disconnect leaked installed Warrior Test encounter");
    }
    const std::string fixture_population="gw-npcs 1\n# Synthetic quest enemy\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 50 0 0 - -\n";
    {std::ofstream file(fixtures/"npc.txt",std::ios::binary);file<<fixture_population;}
    {std::ofstream file(fixtures/"combat.txt");file<<"gw-npc-combat 2 "<<hash<<"\nsource test authored "<<hash<<
        " tests/npc_world_combat.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 0 test test test - - 60 - test -\n";}
    const auto fixture_hash=[&](std::string_view bytes){return bytes==fixture_population?hash:std::string(64,'b');};
    const auto manifest="gw-quest-encounters 1\nkill 85 146 npc.txt combat.txt "+hash+"\n";
    std::istringstream manifest_input(manifest);const auto catalog=load_quest_encounters(manifest_input,fixtures,fixture_hash);
    check(catalog.size()==1&&catalog.at(85).actors.size()==1&&catalog.at(85).actors[0].combat_definition->fire_armor==60,"encounter manifest failed to bind content");
    std::map<std::uint32_t,NpcCatalog::KillEncounter> collections;
    const auto scale_row="scale 83 146 npc.txt combat.txt "+hash+"\n";
    std::istringstream collection_input("gw-quest-encounters 2\n"+scale_row);
    check(load_quest_encounters(collection_input,fixtures,fixture_hash,&collections).empty()&&
          collections.size()==1&&collections.at(83).actors.size()==1,"collection manifest became kill objective");
    for(const auto text:{"gw-quest-encounters 1\n"+scale_row,
                        "gw-quest-encounters 2\n"+scale_row+scale_row,
                        "gw-quest-encounters 2\nscale 85 146 npc.txt combat.txt "+hash+"\n"}){
        bool rejected=false;try{std::istringstream input(text);load_quest_encounters(input,fixtures,fixture_hash,&collections);}
        catch(const std::invalid_argument&){rejected=true;}
        check(rejected&&collections.size()==1&&collections.contains(83),"invalid manifest mutated collection output");
    }
    bool missing_collection_output=false;
    try{std::istringstream input("gw-quest-encounters 2\n"+scale_row);load_quest_encounters(input,fixtures,fixture_hash);}
    catch(const std::invalid_argument&){missing_collection_output=true;}
    check(missing_collection_output,"collection silently entered kill-only loader");
    const auto wave_row=[&](unsigned index,unsigned map=146){return "wave 73 "+std::to_string(map)+" "+std::to_string(index)+" npc.txt combat.txt "+hash+"\n";};
    const auto wave_manifest=std::string("gw-quest-waves 1\n")+wave_row(1)+wave_row(2)+wave_row(3);
    std::istringstream wave_input(wave_manifest);const auto waves_catalog=load_quest_waves(wave_input,fixtures,fixture_hash);
    check(waves_catalog.size()==1&&waves_catalog.at(73).map==146&&waves_catalog.at(73).waves.size()==3&&
        waves_catalog.at(73).waves[2][0].combat_definition->fire_armor==60,"wave manifest lost reviewed combat");
    for(const auto& malformed:std::vector<std::string>{"gw-quest-waves 1\n", "gw-quest-waves 2\n",
        "gw-quest-waves 1\n"+wave_row(2), "gw-quest-waves 1\n"+wave_row(1)+wave_row(1),
        "gw-quest-waves 1\n"+wave_row(1)+wave_row(3),
        "gw-quest-waves 1\n"+wave_row(1)+wave_row(2,161),
        wave_manifest+"wave 73 146 4 npc.txt combat.txt "+std::string(64,'b')+"\n"}){
        bool invalid=false;try{std::istringstream input(malformed);load_quest_waves(input,fixtures,fixture_hash);}
        catch(const std::invalid_argument&){invalid=true;}check(invalid,"invalid wave content accepted");
    }
    {
        Instance deferred;auto output=std::make_shared<Outbox>();deferred.join(2,"Deferred",0,0,output);
        const auto owner=*deferred.effect_scope(2);unsigned commits=0;
        check(!accept_kill_encounter(deferred,owner,165,37,1,catalog.at(85),[&]{++commits;return false;})&&
            commits==1&&!deferred.effect_scope(60000),"failed cross-map acceptance exposed actors");
        check(accept_kill_encounter(deferred,owner,165,37,1,catalog.at(85),[&]{++commits;return true;})&&
            commits==2&&!deferred.effect_scope(60000),"cross-map acceptance spawned foreign actors");
        auto stale=owner;++stale.generation;
        check(!accept_kill_encounter(deferred,stale,165,37,1,catalog.at(85),[&]{++commits;return true;})&&
            commits==2,"stale owner committed cross-map acceptance");
        check(accept_kill_encounter(deferred,owner,146,37,1,catalog.at(85),[&]{++commits;return true;})&&
            deferred.effect_scope(60000).has_value(),"destination failed to activate encounter");
    }
    {std::istringstream input("gw-quest-encounters 4\nkill-count 43 146 npc.txt combat.txt "+hash+" 1\n");
     check(load_quest_encounters(input,fixtures,fixture_hash).at(43).required==1,"counted encounter threshold was lost");}
    for(const auto text:{std::string("gw-quest-encounters 5\n"),manifest+manifest.substr(manifest.find("kill")),
        std::string("gw-quest-encounters 4\nkill-count 43 146 npc.txt combat.txt ")+hash+" 2\n",
        std::string("gw-quest-encounters 4\nkill-count 43 146 npc.txt combat.txt ")+hash+" 0\n",
        std::string("gw-quest-encounters 1\nkill 85 161 npc.txt combat.txt ")+hash+"\n",
        std::string("gw-quest-encounters 1\nkill 85 146 npc.txt combat.txt ")+std::string(64,'b')+"\n"}){
        rejected=false;try{std::istringstream input(text);load_quest_encounters(input,fixtures,fixture_hash);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"invalid encounter manifest accepted");
    }
    for(unsigned service=0;service<3;++service){
        auto bad_population=population;
        auto& npc=bad_population.maps.at(146).front();
        npc.xunlai_storage=service==0;npc.guild_emblemer=service==1;npc.guild_trim_vendor=service==2;
        rejected=false;try{bind_combat_catalog(bad_population,reviewed,hash);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"protected service enabled for damage");
    }
    {
        // Real captured Grawl content, distinct from the synthetic mechanisms
        // above. This checks exact objective identities, not enemy AI/gameplay.
        const auto directory=std::filesystem::path(QUEST_CONTENT_DIRECTORY);
        std::ifstream appearance_file(directory/"actors/grawl-invaders.txt"),combat_file(directory/"grawl-development-combat.txt");
        check(bool(appearance_file)&&bool(combat_file),"Grawl content unavailable");
        auto grawl=load_npc_catalog(appearance_file);
        bind_combat_catalog(grawl,load_catalog(combat_file,GRAWL_POPULATION_SHA256),GRAWL_POPULATION_SHA256);
        const auto& actors=grawl.for_map(160);
        check(actors.size()==3,"Grawl objective contains unrelated population");
        for(const auto& actor:actors)check(actor.captured_name=="Grawl Invader"&&actor.definition.model_file==141274&&
            actor.combat_definition->spawn_health&&actor.combat_definition->spawn_health->current==80&&
            actor.combat_definition->spawn_health->maximum==80,"captured Grawl identity/wiki health changed");
        auto navigation=std::make_shared<navigation::Mesh>(navigation::Mesh::load(directory.parent_path()/"navigation"/
            (std::to_string(map_asset_file(160))+".fwnav")));navigation->discard_unverified_portals();
        Instance quest;quest.configure_navigation(navigation);auto output=std::make_shared<Outbox>();
        quest.join(2,"Quest Tester",actors[0].x,actors[0].y,output);
        quest.set_vitals(2,{100,100,20,20},100);const auto owner=*quest.effect_scope(2);
        check(!quest.spawn_kill_encounter(owner,75,1,actors,[]{return false;})&&!quest.effect_scope(60000),"failed grant exposed Grawl");
        check(quest.spawn_kill_encounter(owner,75,1,actors,[]{return true;}),"Grawl activation failed");
        for(unsigned n=0;n<3;++n){
            const auto target=*quest.effect_scope(60000+n);
            const auto now=1000ull+n*100000ull;
            quest.move(2,actors[n].x,actors[n].y,now);quest.tick(now+90000);
            check(quest.damage_npc(owner,target,{combat::Kind::earth,100,20,194},1248,now+90000).killed,"captured Grawl did not die");
            check(quest.pending_quest_completions(owner).size()==unsigned(n==2),"Grawl objective completed before all three deaths");
        }
        check(quest.pending_quest_completions(owner)==std::vector<Instance::QuestCompletion>{{75,1}},"Grawl completion identity changed");
        check(!quest.clear_kill_objective(owner,75,2,true),"stale attempt removed Grawl");
        check(quest.clear_kill_objective(owner,75,1,true)&&!quest.effect_scope(60000),"Grawl abandonment left stale actors");
        check(quest.spawn_kill_encounter(owner,75,2,actors,[]{return true;})&&quest.effect_scope(60003),"Grawl reaccept did not allocate fresh actors");
        quest.leave(2);check(!quest.effect_scope(60003),"Grawl disconnect leaked encounter");
    }
    {
        const auto directory=std::filesystem::path(QUEST_CONTENT_DIRECTORY);
        std::ifstream appearance(directory/"actors/tower-golem.txt"),metadata(directory/"tower-development-combat.txt");
        auto population=load_npc_catalog(appearance);
        bind_combat_catalog(population,load_catalog(metadata,TOWER_POPULATION_SHA256),TOWER_POPULATION_SHA256);
        const auto& actors=population.for_map(161);
        check(actors.size()==1&&actors[0].captured_name=="Tower Golem"&&actors[0].definition.model_file==141262,
            "Tower objective identity changed");
        Instance tower;auto output=std::make_shared<Outbox>();tower.join(2,"Air Tester",actors[0].x,actors[0].y,output);
        tower.set_vitals(2,{100,100,20,20},100);const auto owner=*tower.effect_scope(2);
        const NpcCatalog::KillEncounter encounter{161,actors};
        check(accept_kill_encounter(tower,owner,165,37,1,encounter,[]{return true;})&&!tower.effect_scope(60000),
            "Foible acceptance leaked Tower actor");
        check(accept_kill_encounter(tower,owner,161,37,1,encounter,[]{return true;}),"Tower destination activation failed");
        const auto golem=*tower.effect_scope(60000);
        // Exercise the actual production quest metadata, not a synthetic
        // fixture with armor supplied only by the test.
        Instance bolts;auto bolt_output=std::make_shared<Outbox>();
        bolts.join(2,"Air Tester",actors[0].x-100,actors[0].y,bolt_output);
        bolts.set_vitals(2,{100,100,20,20},100);const auto bolt_owner=*bolts.effect_scope(2);
        check(accept_kill_encounter(bolts,bolt_owner,161,37,1,encounter,[]{return true;}),"Tower Javelin activation failed");
        const auto bolt_golem=*bolts.effect_scope(60000);
        check(bolts.configure_projectiles({100,2}),"Tower projectile policy rejected");
        check(bolts.lightning_javelin(bolt_owner,bolt_golem,*skills::effects::lightning_javelin(0),1,5,100),
            "Quest-granted Lightning Javelin rejected Tower metadata");
        bolts.tick(1100);
        check(bolts.npc_combat_snapshot(bolt_golem)->health->current<100,
            "Quest-granted Lightning Javelin did not damage Tower");
        check(tower.fire_storm(owner,golem,*skills::effects::fire_storm(12),20,1000),"NPC Fire Storm rejected");
        tower.tick(1999);check(tower.npc_combat_snapshot(golem)->health->current==100,"Fire Storm hit before first pulse");
        const auto first_health=100-*combat::amount({combat::Kind::fire,29,20,197},0);
        tower.tick(2100);check(std::abs(tower.npc_combat_snapshot(golem)->health->current-first_health)<0.0001,"Fire Storm pulse damage incorrect");
        tower.tick(2200);check(std::abs(tower.npc_combat_snapshot(golem)->health->current-first_health)<0.0001,"Fire Storm duplicated pulse");
        tower.tick(5100);check(tower.npc_combat_snapshot(golem)->health->current==0,"Fire Storm failed to kill quest NPC");
        check(tower.pending_quest_completions(owner)==std::vector<Instance::QuestCompletion>{{37,1}},
            "Tower death did not complete Supremacy");
        tower.leave(2);check(!tower.effect_scope(60000),"Tower disconnect leaked actor");
    }
    {
        const auto directory=std::filesystem::path(QUEST_CONTENT_DIRECTORY);
        std::ifstream population_file(directory/"actors/mesmer-test-skale.txt",std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(population_file)),{});
        const std::string sha="beac6399f6d52c52b5ef9acd88c39c7acdbae750214dc88a9309cede5f5fd396";
        std::ifstream manifest_file(directory/"mesmer-test-encounters.txt");
        const auto catalog=load_quest_encounters(manifest_file,directory,[&](std::string_view input){return input==bytes?sha:std::string{};});
        check(catalog.size()==1&&catalog.at(81).any_death&&catalog.at(81).actors.size()==4,"Mesmer content lost one-kill policy");
        const auto& encounter=catalog.at(81);const auto& first=encounter.actors.front();
        Instance quest;quest.join(2,"Cane Quest Tester",first.x-10,first.y,std::make_shared<Outbox>());
        quest.set_vitals(2,{100,100,20,20},100);const auto owner=*quest.effect_scope(2);
        const auto gear=*inventory::presearing::creation_weapon(inventory::baseline(),5,u"Cane Quest Tester");
        const auto cane=combat::weapon::owned_weapon(gear,1,{5,"Cane Quest Tester",{},true,{}});
        check(cane&&quest.ordinary_weapon(owner,*cane,1)&&quest.configure_projectiles({100,2}),"Mesmer starter weapon unavailable");
        check(accept_kill_encounter(quest,owner,146,81,1,encounter,[]{return true;}),"Mesmer encounter activation failed");
        const auto target=*quest.effect_scope(60000),survivor=*quest.effect_scope(60001);
        check(first.combat_definition->spawn_energy==Energy{40,40}&&first.combat_definition->energy_pips==5,
            "Installed Mesmer skale lost reviewed energy reference profile");
        quest.set_vitals(2,{40,100,20,20},100);
        check(quest.energy_drain(owner,target,*skills::effects::energy_drain(40,0),1,100)&&quest.resources(2)->health==100&&quest.resources(2)->energy==20&&quest.npc_combat_snapshot(target)->health->current==10,
            "Ether Feast failed against installed Mesmer skale or stole energy/dealt damage");
        check(quest.ordinary_attack(owner,target.actor,100),"Mesmer ordinary attack rejected");
        for(unsigned n=0;n<=8;++n){quest.tick(600+n*1750);quest.tick(700+n*1750);}
        check(quest.npc_combat_snapshot(target)->health->current==0&&quest.npc_combat_snapshot(survivor)->health->current==10,"Mesmer ordinary damage or remaining population wrong");
        check(quest.pending_quest_completions(owner)==std::vector<Instance::QuestCompletion>{{81,1}},"Mesmer content did not complete after one kill");
    }
    std::cout<<"NPC metadata, authoritative damage, death identity and reconnect snapshots verified\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}




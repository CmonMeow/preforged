#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include "gw_npc_catalog.h"
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace fw::gw;
static void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}

int main()try{
    using namespace skills;
    const auto base=effects::holy_damage(1689,12);
    check(base&&base->damage==0&&effects::holy_damage(1689,12,2)->damage==58&&
          effects::holy_damage(1689,12,4)->damage==100,"ranked per-enchantment damage and cap");
    check(!effects::holy_damage(1689,21)&&!effects::holy_damage(1689,12,21),"invalid rank/count rejected");
    auto forged=*base;forged.damage=58;check(!effects::holy_strikes(forged,{false,false},20,false),"effect count cannot be forged");

    world::Instance w;auto caster_box=std::make_shared<world::Outbox>(),target_box=std::make_shared<world::Outbox>();
    w.join(1,"Monk",0,0,caster_box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
    w.join(2,"Foe",100,0,target_box,0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
    const auto caster=*w.effect_scope(1),foe=*w.effect_scope(2);
    check(w.player_holy_equipment(foe,0),"reviewed human target armor");
    check(w.status_mutation(caster,169,1,12,100,10000,true)&&
          w.status_mutation(caster,180,2,12,100,10000,true),"two owned enchantments");
    std::vector<world::Packet> visuals;
    check(w.holy_damage(caster,foe,*base,20,100,&visuals),"world applies completion-time enchantment count");
    check(w.resources(2)->health==942&&visuals.size()==1&&
          visuals[0]==*particles::targeted_body_2022(2,1,1689),"58 holy damage and native targeted1658 body");
    check(w.status_mutation(caster,169,1,0,200,{},false),"enchantment stripped");
    visuals.clear();check(w.holy_damage(caster,foe,*base,20,200,&visuals)&&w.resources(2)->health==913,
          "a stripped enchantment reduces next signet damage at completion");
    check(w.status_mutation(caster,180,2,0,300,{},false),"last enchantment stripped");
    check(w.holy_damage(caster,foe,*base,20,300)&&w.resources(2)->health==913,"zero enchantments cause zero damage");
    auto stale=foe;++stale.generation;check(!w.holy_damage(caster,stale,*base,20,400),"stale target denied");

    // NPC targets use independently reviewed holy traits, and a caller must
    // not bypass the completion-time enchantment count with a forged plan.
    for(unsigned light=0;light<2;++light){
        const std::string hash(64,'a');
        const std::string record="gw-npc-combat 9 "+hash+"\nsource test authored "+hash+
            " tests/mystic_wrath_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 - 1 0 test test - - -"
            " - - - - - - 1 - - - - - - - - - - "+std::to_string(light)+" test 0 test\n";
        std::istringstream metadata(record);const auto catalog=combat::npcs::load_catalog(metadata,hash);
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
        auto population=world::load_npc_catalog(appearance);
        world::bind_combat_catalog(population,catalog,hash);
        world::Instance nw;nw.configure_npcs(population.for_map(146));
        nw.join(1,"Monk",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        nw.set_vitals(1,{100,100,20,20},100);
        const auto owner=*nw.effect_scope(1),target=*nw.effect_scope(100);
        check(nw.status_mutation(owner,169,1,12,100,10000,true)&&
              nw.status_mutation(owner,180,2,12,100,10000,true),"NPC fixture enchantments");
        auto invalid=*base;invalid.damage=100;
        check(!nw.holy_damage(owner,target,invalid,20,100),"NPC target rejects forged damage plan");
        check(nw.holy_damage(owner,target,*base,20,100),"NPC holy target accepted");
        check(nw.npc_combat_snapshot(target)->health->current==500-58*(light?2:1),
              "NPC reviewed light-sensitive damage");
        auto reused=target;++reused.generation;
        check(!nw.holy_damage(owner,reused,*base,20,200),"NPC stale lifetime rejected");
    }

    Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[1689]=1;
    Slots slots{};slots[0]={1689,0};attributes::Ranks ranks{};ranks[14]=12;
    casting::Hooks hooks;std::vector<world::Packet> cast_packets;
    hooks.resources=[&](unsigned id){return w.resources(id);};
    hooks.target_scope=[&](unsigned id){return w.effect_scope(id);};
    hooks.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248.f;};
    hooks.consume_energy_at=[&](unsigned cost,auto now){return w.consume_energy(1,cost,caster,now);};
    hooks.events=[&](const auto& batch){cast_packets.insert(cast_packets.end(),batch.begin(),batch.end());};
    hooks.holy_damage=[&](auto target,const auto& plan,auto now)->casting::EffectCompletion{
        std::vector<world::Packet> out;const bool ok=w.holy_damage(caster,target,plan,20,now,&out);
        return {ok,std::move(out)};
    };
    casting::Runtime cast(1);
    check(cast.start(1000,{1689,0,2},slots,policy,ranks,true,hooks)==casting::Reason::accepted,"normal paid signet cast");
    cast.update(2999,hooks);check(w.resources(2)->health==913,"two-second cast not early");
    cast.update(3000,hooks);check(w.resources(2)->health==913&&
          std::find(cast_packets.begin(),cast_packets.end(),*particles::targeted_body_2022(2,1,1689))!=cast_packets.end(),
          "normal completion routes target body even with zero enchantments");
    std::cout<<"Signet of Mystic Wrath world passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

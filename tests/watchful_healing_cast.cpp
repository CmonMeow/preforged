#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Watchful cast line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned context=0;context<3;++context)for(unsigned mode=0;mode<22;++mode){
        const bool self=context==1,monk=context!=2;const unsigned target=self?1:2,rank=monk?12:0;
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{world::Packet{165,0,1,0,0,0,static_cast<std::uint8_t>(monk?3:6),0}}, {},100,1);
        for(unsigned id:{2u,3u})w.join(id,"Other",id*100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?1:2);
        for(unsigned id:{1u,2u,3u})w.set_vitals(id,{id==target?500u:1000u,1000,100,100},100);
        check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
        const auto source=*w.effect_scope(1),ally=*w.effect_scope(target);const auto plan=*effects::watchful_healing(rank);
        if(mode==11){w.set_vitals(target,{1,1000,100,100},100);check(w.soul_barbs(*w.effect_scope(3),ally,*effects::soul_barbs(12),20,100));}
        if(mode==12)check(w.degeneration_hex(*w.effect_scope(3),ally,*effects::degeneration_hex(129,0),100));
        inventory::Profile inventory;inventory.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item weapon;
        weapon.definition.id=100;weapon.definition.type=26;weapon.definition.quantity=1;weapon.definition.modifiers={0x22b80014};weapon.bag=1;inventory.items.emplace(100,weapon);inventory.weapons.sets[0].weapon=100;
        const auto quote=[&](auto at)->std::optional<effects::EnchantmentDuration>{
            const auto equipment=items::weapon_passives::equipped_enchanting(inventory,1);const auto active=w.status_snapshot(source,at);
            return equipment&&active?effects::enchanting_duration(1392,plan.duration,source,*equipment,*active,at):std::nullopt;
        };
        bool in_range=true;unsigned attempts=0;std::vector<world::Packet> sent;casting::Hooks h;
        h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());box->enqueue(packets);};
        h.consume_energy_at=[&](auto cost,auto at){check(cost==5);return w.consume_energy(1,cost,source,at);};
        h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[&](auto id,float range,bool allow_self){return in_range&&id==target&&range==1248&&allow_self;};
        h.ally_enchantment_target_valid=[&](auto selected,unsigned divine,auto at){return in_range&&w.valid_watchful_target(source,selected,divine,at);};
        h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
        if(mode>=17)h.enchantment_duration_ready=[&](unsigned used,auto base,auto at){check(used==1392&&base==10000);return bool(quote(at));};
        h.watchful_healing=[&](auto selected,const auto& p,unsigned divine,auto at)->casting::EffectCompletion{
            ++attempts;check(p==plan&&divine==rank&&selected==ally);
            auto caster=source;if(mode==10)++caster.generation;
            std::vector<world::Packet> visuals;std::function<void()> publish;
            const auto adjusted=mode>=17?quote(at):std::nullopt;
            const bool ok=(mode<17||adjusted)&&w.stage_watchful_healing(caster,selected,p,divine,at,&visuals,&publish,adjusted?&*adjusted:nullptr);
            check(box->drain().empty());return {ok,std::move(visuals),{},std::move(publish),true};
        };
        Policy policy;policy.primary=monk?3:6;policy.secondary=monk?0:3;policy.campaigns=255;policy.learned[1392]=1;
        Slots bar{};bar[0]={1392,0};attributes::Ranks ranks{};ranks[16]=rank;casting::Runtime cast(1);
        check(casting::Runtime::effect_supported(1392));
        if(mode==5)h.watchful_healing={};if(mode==7)h.before_spell_effect={};
        if(mode==13)h.target_scope={};if(mode==14)h.ally_enchantment_target_valid={};
        if(mode==19)inventory.items.at(100).definition.modifiers={0x22b800ff};
        if(mode==9)w.set_vitals(1,{self?500u:1000u,1000,0,100},100);
        check(cast.start(1000,{1392,0,target},bar,policy,ranks,true,h)==(mode==5||mode==7||mode==13||mode==14||mode==19?casting::Reason::unsupported:mode==9?casting::Reason::energy:casting::Reason::accepted));
        if(mode==5||mode==7||mode==9||mode==13||mode==14||mode==19){check(!cast.active()&&w.resources(1)->energy==(mode==9?0:100));continue;}
        check(w.resources(1)->energy==95);cast.update(1999,h);check(attempts==0);
        if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1999,h);
        if(mode==3)h.target_scope=[&](auto id)->std::optional<effects::Scope>{auto s=w.effect_scope(id);if(s&&id==target)++s->generation;return s;};
        if(mode==4)in_range=false;if(mode==6)h.watchful_healing={};if(mode==8)h.before_spell_effect={};
        if(mode==15)h.target_scope={};if(mode==16)h.ally_enchantment_target_valid={};
        if(mode==18)inventory.items.at(100).definition.modifiers={0x22b8000a};
        if(mode==20)inventory.items.at(100).definition.modifiers={0x22b800ff};if(mode==21)h.enchantment_duration_ready={};
        box->drain();cast.update(2000,h);check(!cast.active());
        const bool applied=mode==0||mode==11||mode==12||mode==17||mode==18;check(attempts==unsigned(applied||mode==10));
        const auto body=*particles::targeted_body_2022(target,1,1392);
        check(std::count(sent.begin(),sent.end(),body)==unsigned(applied));
        const double divine_heal=monk?38.*(mode==12?.66:1.):0;
        check(w.resources(target)->health==(mode==11?0:applied?unsigned(std::ceil(500+divine_heal)):500u));
        if(mode==0||mode==12||mode==17||mode==18){const auto status=w.status_snapshot(ally,2000);check(status&&status->size()==(mode==12?2:1));
            const auto watchful=std::find_if(status->begin(),status->end(),[](const auto& e){return e.skill==1392;});
            check(watchful!=status->end()&&watchful->rank==rank&&watchful->expires==(mode==17?14000u:mode==18?13000u:12000u));
            const auto packets=box->drain();const auto added=std::find_if(packets.begin(),packets.end(),[&](const auto& p){return p.size()>=8&&p[0]==65&&p[6]==112&&p[7]==5;});
            check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),body));
            check(added!=packets.end()&&std::find(packets.begin(),packets.end(),body)<added);
            const auto df=particles::health_change_2022(target,1,monk?divine_heal:38,1000);
            check(std::count(packets.begin(),packets.end(),df)==unsigned(monk));
            if(monk)check(added<std::find(packets.begin(),packets.end(),df));
            check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{1392,0},10))==1);
            cast.update(2000,h);check(attempts==1);
            check(cast.start(2001,{1392,0,target},bar,policy,ranks,true,h)==casting::Reason::busy);
            check(cast.start(2750,{1392,0,target},bar,policy,ranks,true,h)==casting::Reason::recharge);
            if(mode==12){check(w.enchantment_removal(*w.effect_scope(3),ally,*effects::enchantment_removal(68,0),20,2000));
                check(w.resources(target)->health==unsigned(std::ceil(500+divine_heal+plan.gain)));}
        }
    }
    std::cout<<"PASS66 Watchful Runtime-to-World equipment duration,malformed/lost quote, ally/self/secondary,5Energy/1s/10s,DF,target1647,status order and cast/reaction guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

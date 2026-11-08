#include "gw_presearing_skills.h"
#include "gw_skill_casting.h"
#include "gw_presearing_items.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace skills;using namespace skills::casting;
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    constexpr std::array<unsigned,6> files{39776,9380,94904,94936,93541,94937};
    for(unsigned primary:{1u,2u,3u,4u,5u,6u}){
        const auto baseline=inventory::baseline();const auto armed=inventory::presearing::creation_weapon(baseline,primary,u"Starter Test");
        check(armed&&armed->items.size()==1&&baseline.items.empty(),"creation weapon mutated input or failed");
        const auto& item=armed->items.begin()->second;
        check(item.bag==2&&item.slot==0&&armed->weapons.sets[0].weapon==item.definition.id,"starter weapon not equipped/selected");
        check(item.definition.type==(primary==1?15:primary==2?5:22)&&item.definition.file==files[primary-1],"captured starter definition changed");
        check(item.profession==primary&&item.customized==u"Starter Test","creation weapon ownership changed");
        check(!inventory::presearing::creation_weapon(*armed,primary,u"Starter Test"),"creation duplicated equipped weapon");
        const auto restored=inventory::decode(inventory::encode(*armed));check(restored.weapons==armed->weapons&&restored.items.size()==1,"starter weapon persistence failed");
        if(primary==1){combat::weapon::Policy policy{1,"Starter Test",{},false,19};const auto weapon=combat::weapon::owned_weapon(*armed,1,policy);
            check(weapon&&weapon->damage.minimum==3&&weapon->damage.maximum==5&&weapon->damage.raw_damage_type==0,"captured starter hammer cannot enter combat");}
        else if(primary>=3){combat::weapon::Policy policy{primary,"Starter Test",{},true,{}};const auto weapon=combat::weapon::owned_weapon(*armed,1,policy);
            const auto damage_type=primary==4?7u:primary==5?6u:5u;
            check(weapon&&weapon->caster_weapon&&weapon->item_type==22&&weapon->damage.minimum==3&&weapon->damage.maximum==5&&weapon->damage.raw_damage_type==damage_type,"captured rod cannot enter combat");
            auto unknown=*armed;unknown.items.begin()->second.definition.modifiers[1]=0x26980003;
            check(!combat::weapon::owned_weapon(unknown,1,policy),"unreviewed rod modifier admitted");}
    }
    check(!inventory::presearing::creation_weapon(inventory::baseline(),7,u"Starter Test"),"non-Prophecies starter invented");
    {
        const auto original=*inventory::presearing::creation_weapon(inventory::baseline(),6,u"Bow Trial");
        const auto granted=inventory::presearing::grant_starter_bow(original,u"Bow Trial");
        check(granted&&original.items.size()==1&&granted->items.size()==2,"quest bow failed or mutated input");
        const auto& bow=granted->items.at(original.next_item);
        check(bow.definition.type==5&&bow.definition.file==9380&&bow.definition.model==477&&bow.profession==0,
              "quest bow lost captured definition or restricted secondary trial");
        check(granted->bags.at(bow.bag).type==1&&granted->weapons==original.weapons,"quest bow auto-equipped or entered storage");
        auto full=*granted;
        while(const auto next=inventory::presearing::grant_starter_bow(full,u"Bow Trial"))full=*next;
        const auto saved=inventory::encode(full);
        check(!inventory::presearing::grant_starter_bow(full,u"Bow Trial")&&inventory::encode(full)==saved,"full bags changed inventory");
    }
    std::set<unsigned> ids;
    for(const auto& entry:presearing::skills){
        const auto* row=find_skill(entry.id);
        check(row&&row->profession<=6&&ids.insert(entry.id).second,"invalid/duplicate pre-Searing identity");
        check(presearing::obtainable_skill(entry.id),"listed skill unavailable");
    }
    check(ids.size()==44&&!presearing::obtainable_skill(3)&&!presearing::obtainable_skill(1043),"non-pre-Searing grant admitted");
    Policy policy;policy.primary=6;policy.campaigns=0x1e;policy.learned={{194,1}};Slots bar{};bar[0]={194,0};
    attributes::Ranks ranks{};ranks[10]=12;
    vitals::Resources self{100,100,20,20},foe{100,100,20,20};effects::Scope target{1,43,5};
    unsigned launched=0;std::vector<Packet> events;Hooks h;
    h.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());};
    h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):id==43?std::optional(foe):std::nullopt;};
    h.consume_energy=[&](unsigned cost){if(cost>self.energy)return false;self.energy-=cost;return true;};
    h.target_valid=[](auto,float,bool){return false;};
    h.projectile_target_valid=[](unsigned id,float range){return id==43&&range==1248;};
    h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?std::optional(target):std::nullopt;};
    h.elemental=[&](effects::Scope scope,const effects::ElementalPlan& plan,std::uint64_t){
        check(scope==target&&plan.skill==194&&plan.projectile&&plan.damage==56,"incorrect Flare projectile");++launched;return true;
    };
    Runtime cast(42);
    check(cast.start(0,{194,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==15,"Flare did not spend five energy");
    cast.update(999,h);check(!launched,"Flare launched before activation");
    cast.update(1000,h);check(launched==1,"Flare did not launch once");
    check(cast.start(1749,{194,0,43},bar,policy,ranks,true,h)==casting::Reason::busy,"Flare skipped aftercast");
    check(cast.start(1750,{194,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Flare acquired invented recharge");
    cast.cancel(h);cast.update(2750,h);check(launched==1,"cancelled Flare launched");
    check(cast.start(4000,{194,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Flare stale-target setup");
    ++target.generation;cast.update(5000,h);check(launched==1,"Flare hit recycled target");
    check(!events.empty(),"Flare omitted native cast events");
    std::cout<<"44 pre-Searing identities and Flare cast lifecycle verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

#include "gw_skill_casting.h"
#include "gw_character_skills.h"
#include "gw_world_wire.h"
#include <iostream>

int main()try{
    using namespace fw::gw;using namespace skills;using namespace skills::casting;
    auto check=[](bool condition,const char* msg){if(!condition)throw std::runtime_error(msg);};
    {
        Hooks hooks;unsigned snapshots=0,resource_reads=0;
        hooks.resources=[&](unsigned)->std::optional<vitals::Resources>{++resource_reads;return vitals::Resources{100,100,30,30};};
        hooks.effect_snapshot=[&](Milliseconds)->std::optional<std::vector<effects::Entry>>{++snapshots;return std::vector<effects::Entry>{};};
        Runtime idle(42);
        for(Milliseconds now=0;now<1000;++now)idle.update(now,hooks);
        check(snapshots==0,"idle casting unnecessarily locks/copies world effect snapshots");
        check(resource_reads==0,"idle casting unnecessarily locks world resources");
    }
    {
        Policy policy;policy.primary=5;policy.campaigns=0x1e;policy.learned[76]=1;
        Slots bar{};bar[0]={76,0};attributes::Ranks ranks{};ranks[1]=12;Hooks hooks;
        vitals::Resources self{100,100,30,30};effects::Scope target{1,43,1};unsigned commits=0,cost=0;
        hooks.events=[](const auto&){};hooks.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):std::nullopt;};
        hooks.consume_energy=[&](unsigned amount){cost+=amount;return true;};hooks.target_valid=[](auto,float,bool){return false;};
        hooks.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?std::optional(target):std::nullopt;};
        hooks.projectile_target_valid=[](unsigned id,float range){return id==43&&range==1248;};
        Runtime unavailable(42);check(unavailable.start(100,{76,0,43},bar,policy,ranks,true,hooks)==casting::Reason::unsupported&&!cost,"movement hook absence consumed energy");
        hooks.movement_hex=[&](auto scope,const auto& plan,auto now){check(scope==target&&plan.duration==18000&&plan.rank==12&&now==1100,"movement hex plan/target/timing");++commits;return true;};
        Runtime cast(42);check(cast.start(100,{76,0,43},bar,policy,ranks,true,hooks)==casting::Reason::accepted&&cost==15,"Imagined Burden cost or NPC target");
        cast.update(1099,hooks);check(!commits,"hex before cast completion");cast.update(1100,hooks);check(commits==1,"NPC movement hex never committed");
    }
    for(unsigned preparation:{431u,432u}){
        Policy policy;policy.primary=2;policy.campaigns=0x1e;policy.learned[preparation]=1;
        Slots bar{};bar[0]={preparation,0};attributes::Ranks ranks{};ranks[25]=12;ranks[24]=12;
        vitals::Resources body{480,480,20,20};Hooks hooks;unsigned commits=0,removes=0;
        std::vector<Packet> packets;
        hooks.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        hooks.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(body):std::nullopt;};
        hooks.target_valid=[](unsigned actor,float,bool self){return actor==42&&self;};
        hooks.consume_energy=[&](unsigned cost){if(cost>body.energy)return false;body.energy-=cost;return true;};
        hooks.status_mutation=[&](unsigned skill,unsigned,unsigned rank,Milliseconds now,std::optional<Milliseconds> until,bool add){
            check(skill==preparation,"preparation used wrong skill identity");
            if(add){check(rank==12&&now==2000&&until==26000,"preparation duration or rank changed");++commits;}
            else ++removes;
            return true;
        };
        Runtime runtime(42);
        check(runtime.start(0,{preparation,0,999},bar,policy,ranks,true,hooks)==casting::Reason::accepted,
              "Read the Wind must target self despite selected enemy");
        runtime.update(1999,hooks);check(!commits,"preparation applied before two-second cast");
        runtime.update(2000,hooks);check(commits==1,"preparation did not commit");
        check(std::find(packets.begin(),packets.end(),activation(42,42,preparation))!=packets.end(),"preparation native cast animation absent");
        runtime.update(26000,hooks);check(removes==1,"preparation did not remove timed native effect");
    }
    for(unsigned bow_skill:{394u,396u,1953u,2096u}){
        Policy ranger;ranger.primary=2;ranger.campaigns=0x1e;ranger.learned[bow_skill]=1;
        Slots bar{};bar[0]={bow_skill,0};attributes::Ranks ranks{};Hooks hooks;
        vitals::Resources self{100,100,20,20};effects::Scope target{1,43,1};unsigned hits=0,cost=0;
        hooks.events=[](const auto&){};hooks.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(self):std::nullopt;};
        hooks.consume_energy=[&](unsigned amount){cost+=amount;return true;};hooks.target_valid=[](auto,float,bool){return false;};
        hooks.weapon_snapshot=[](){return std::optional<combat::weapon::Owned>{{1,1,{20,20,1},12,false,false,5}};};
        hooks.weapon_attack_timing=[](auto){return std::optional<combat::weapon::Timing>{{500,2475,1200}};};
        hooks.melee_target=[](auto,auto){return false;};hooks.attack_activity=[](auto){};
        hooks.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
        hooks.allegiance_rank=[](unsigned){return std::optional<unsigned>{6};};
        hooks.melee_attack=[&](auto scope,const auto&,const auto& plan,auto){check(scope==target&&plan.arrows==((bow_skill==1953||bow_skill==2096)?3u:bow_skill==396?2u:1u),"multi-arrow plan or lifetime wrong");if(bow_skill==1953||bow_skill==2096)check(plan.rank==6&&plan.base_percent==65,"Triple Shot allegiance damage");++hits;return true;};
        Runtime unavailable(42);check(unavailable.start(100,{bow_skill,0,43},bar,ranger,ranks,true,hooks)==casting::Reason::unsupported&&!cost,"Dual Shot must require skill-aware range guard");
        hooks.attack_skill_target=[&](unsigned actor,unsigned skill,auto){return actor==43&&skill==bow_skill;};
        Runtime cast(42);check(cast.start(100,{bow_skill,0,43},bar,ranger,ranks,true,hooks)==casting::Reason::accepted&&cost==10,"Dual Shot energy or full-range validation failed");
        cast.update(599,hooks);check(!hits,"Dual Shot preceded attack windup");cast.update(600,hooks);check(hits==1,"Dual Shot did not dispatch exactly one two-arrow attack");
        check(cast.start(601,{bow_skill,0,43},bar,ranger,ranks,true,hooks)==casting::Reason::recharge,"Dual Shot recharge missing");
    }
    {
        Policy necro;necro.primary=4;necro.campaigns=0x1e;necro.learned[115]=1;
        Slots bar{};bar[0]={115,0};attributes::Ranks ranks{};
        vitals::Resources body{480,480,20,20};unsigned applied=0,sacrificed=0;Hooks renewal;
        renewal.events=[](const auto&){};
        renewal.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(body):std::nullopt;};
        renewal.target_valid=[](unsigned actor,float,bool self){return actor==42&&self;};
        renewal.consume_energy=[&](unsigned cost){if(cost>body.energy)return false;body.energy-=cost;return true;};
        renewal.apply_health_regeneration=[&](unsigned actor,const regeneration::Plan& plan,Milliseconds now,Milliseconds expiry,float,bool){
            check(actor==42&&plan.skill==115&&plan.pips==3&&now==1000&&expiry==8000,"Blood Renewal uses pinned self duration and rank");++applied;return true;};
        renewal.health_sacrifice=[&](unsigned id,Milliseconds now){check(id==115&&now==1000,"Blood Renewal sacrifice occurs on success");++sacrificed;auto loss=sacrifice::resolve(id,body.health,body.max_health);if(loss)body.health=static_cast<unsigned>(loss->remaining);return loss;};
        Runtime cast(42);check(cast.start(0,{115,0,999},bar,necro,ranks,true,renewal)==casting::Reason::accepted&&body.energy==19,"Blood Renewal targets self despite selected enemy and costs1 energy");
        cast.update(999,renewal);check(!applied&&!sacrificed,"Blood Renewal does not apply before activation");
        cast.update(1000,renewal);check(applied==1&&sacrificed==1&&body.health==408,"Blood Renewal sacrifices15percent after genuine source commit");
        cast.update(2000,renewal);check(applied==1&&sacrificed==1,"Blood Renewal cast commits once");
    }
    {
        Policy necro;necro.primary=4;necro.campaigns=0x1e;necro.learned[109]=1;Slots bar{};bar[0]={109,0};attributes::Ranks ranks{};
        Hooks h;unsigned applied=0;effects::Scope target{1,43,1};
        h.events=[](const auto&){};h.resources=[](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(vitals::Resources{50,100,20,20}):std::nullopt;};
        h.consume_energy=[](unsigned cost){return cost==10;};h.target_valid=[](auto,float,bool){return false;};
        h.projectile_target_valid=[](unsigned actor,float range){return actor==43&&range==1248;};
        h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
        h.degeneration_hex=[&](effects::Scope scope,const effects::DegenerationHexPlan& p,auto){check(scope==target&&p.skill==109&&p.life_drain,"NPC Siphon plan mismatch");++applied;return true;};
        Runtime runtime(42);check(runtime.start(100,{109,0,43},bar,necro,ranks,true,h)==casting::Reason::accepted,"NPC Siphon cast rejected");
        runtime.update(2100,h);check(applied==1,"NPC Siphon completion rejected missing player resources");
    }
    for(unsigned id:{26u,40u,31u,859u,27u}){
        Policy policy;policy.primary=5;policy.campaigns=0x1e;policy.learned[id]=1;
        Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};ranks[2]=12;ranks[3]=12;
        vitals::Resources self{100,100,100,100};effects::Scope npc{1,43,5};unsigned applied=0;Hooks h;
        h.events=[](const auto&){};h.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(self):std::nullopt;};
        h.consume_energy=[&](unsigned cost){if(cost>self.energy)return false;self.energy-=cost;return true;};
        h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned actor,float range){return actor==43&&range==1248;};
        h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(npc):std::nullopt;};
        h.empathy=[&](effects::Scope target,const effects::EmpathyPlan& p,std::uint64_t){check(target==npc&&p.damage==46,"Empathy scope/rank mismatch");++applied;return true;};
        h.shatter_delusions=[&](effects::Scope target,const effects::ShatterDelusionsPlan& p,std::uint64_t){check(target==npc&&p.damage==63,"Shatter scope/rank mismatch");++applied;return true;};
        h.degeneration_hex=[&](effects::Scope target,const effects::DegenerationHexPlan& p,std::uint64_t){check(target==npc&&p.skill==id,"NPC degeneration scope/identity mismatch");++applied;return true;};
        h.energy_drain=[&](effects::Scope target,const effects::EnergyDrainPlan& p,std::uint64_t){check(target==npc&&p.skill==40&&p.heal_per_energy==56,"NPC Feast scope/rank mismatch");++applied;return true;};
        Runtime runtime(42);const auto end=static_cast<unsigned>(find_skill(id)->activation*1000);
        check(runtime.start(0,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"NPC reward skill cast rejected");
        runtime.update(end-1,h);check(applied==0,"NPC reward skill applied before activation");
        runtime.update(end,h);check(applied==1,"NPC reward skill completion lost target without player Resources");
        check(runtime.start(50000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"NPC reward skill recast rejected");
        runtime.cancel(h);runtime.update(50000+end,h);check(applied==1,"Cancelled NPC reward skill applied");
        check(runtime.start(100000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"NPC generation fixture cast rejected");
        ++npc.generation;runtime.update(100000+end,h);check(applied==1,"NPC reward cast hit recycled target");
    }
    {
        vitals::Resources caster{480,480,20,20};Hooks h;std::vector<Packet> events;unsigned launched=0;
        h.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());};
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42||id==43?std::optional(caster):std::nullopt;};
        h.consume_energy=[&](unsigned cost){if(cost>caster.energy)return false;caster.energy-=cost;return true;};
        h.projectile_target_valid=[](unsigned id,float range){return id==43&&range==1248;};
        h.target_scope=[](unsigned id)->std::optional<effects::Scope>{return effects::Scope{1,id,1};};
        h.lightning_javelin=[&](effects::Scope target,const effects::LightningJavelinPlan& plan,Milliseconds now){check(target==effects::Scope{1,43,1}&&plan.rank==12&&plan.damage==43&&now==1000,"Javelin schedules exact reviewed plan after1second");++launched;return true;};
        Policy owner;owner.primary=6;owner.campaigns=0x1e;owner.learned={{230,1}};Slots bar{};bar[0]={230,0};attributes::Ranks ranks{};ranks[8]=12;Runtime runtime(42);
        check(runtime.start(0,{230,0,43},bar,owner,ranks,true,h)==casting::Reason::accepted&&caster.energy==15,"player Javelin accepts owned skill and spends5");
        runtime.update(999,h);check(launched==0,"Javelin launch before cast completes");runtime.update(1000,h);check(launched==1,"Javelin did not schedule flight");
        runtime.update(1001,h);check(launched==1,"Javelin duplicate flight");
        check(std::find(events.begin(),events.end(),activation(42,43,230))!=events.end(),"Javelin native activation target preserved");
        check(!success_body_effect(42,230),"Javelin2077 sentinel is not a fabricated particle");
    }
    {
        unsigned checked=0;Hooks denied;
        std::vector<Packet> acknowledgments;
        denied.events=[&](const auto& packets){acknowledgments.insert(acknowledgments.end(),packets.begin(),packets.end());};
        denied.resources=[](unsigned)->std::optional<vitals::Resources>{throw std::runtime_error("PvP-locked skill read resources");};
        denied.consume_energy=[](unsigned)->bool{throw std::runtime_error("PvP-locked skill spent energy");};
        auto idle=denied;idle.resources=[](unsigned)->std::optional<vitals::Resources>{return vitals::Resources{480,480,20,20};};
        for(const auto& row:skill_catalog_2022){
            if(!(row.special&0x80000u))continue;
            Policy locked;locked.primary=row.profession?row.profession:1;locked.campaigns=0x1e;
            locked.learned={{row.id,1}};locked.pvp_match=true;
            Slots bar{};bar[0]={row.id,0};Runtime match_cast(42);attributes::Ranks ranks{};
            const auto before=acknowledgments.size();
            check(match_cast.start(0,{row.id,0,43},bar,locked,ranks,true,denied)==casting::Reason::pve_only,"every flagged PvE-only skill is locked in PvP");
            check(acknowledgments.size()==before+1&&acknowledgments.back()==cancellation_packet(42,{row.id,0}),"locked skill clears only its pending reference");
            match_cast.update(10000,idle);check(acknowledgments.size()==before+1,"denied PvE skill leaves no pending cast");
            check(bar[0]==Slot{row.id,0}&&locked.learned.at(row.id)==1,"PvP lock preserves equipped and learned state");++checked;
        }
        check(checked>0,"PvE-only native classification has coverage");
        check(std::string(casting::reason_text(casting::Reason::pve_only))=="PvE-only skill unavailable in PvP","PvE-only denial has an explicit diagnostic");
    }
    check(map_allows_casts(1)&&map_allows_casts(4)&&!map_allows_casts(0)&&!map_allows_casts(3)&&!map_allows_casts(15),"map cast gate and instance advertised mode agree");
    const auto explorable_info=world::instance_info(1,162,map_allows_casts(1),1,0,0,client_2022);
    check(explorable_info==Packet({152,1,1,0,0,0,162,0,1,1,0,0,0,0,0}),"native408 explorable flag enables client UseSkill sender");
    const auto arena_info=world::instance_info(2,2,map_allows_casts(6),1,0,0,client_2022);
    check(map_allows_casts(6)&&!map_allows_casts(5)&&arena_info==Packet({152,1,2,0,0,0,2,0,1,1,0,0,0,0,0}),"active arena enables casting while its waiting outpost does not");
    // Exact original retail serializer fixtures (transport high bit removed).
    check(effect_add(50,169,12,1,55.f)==Packet({65,0,50,0,0,0,169,0,12,0,0,0,1,0,0,0,0,0,92,66}),"native65 duration fixture");
    check(effect_remove(50,1)==Packet({67,0,50,0,0,0,1,0,0,0}),"native67 removal fixture");
    check(activation(42,43,281)==Packet({159,0,60,0,0,0,42,0,0,0,43,0,0,0,25,1,0,0}),"native targeted caster/target/skill activation fixture");
    // Original serializer cases in fixtures/earth-aoe-animation-2022.txt, direction bit removed.
    check(success_body_effect(42,174)==Packet({158,0,21,0,0,0,42,0,0,0,57,1,0,0}),"original Aftershock body-resource wire fixture");
    check(success_body_effect(42,937)==Packet({158,0,21,0,0,0,42,0,0,0,158,4,0,0}),"original Shockwave body-resource wire fixture");
    check(success_body_effect(42,217)==Packet({158,0,21,0,0,0,42,0,0,0,130,1,0,0}),"original Crystal Wave body-resource wire fixture");
    check(!success_body_effect(42,2192),"target-body evidence does not invent a caster-body resource");
    check(!success_body_effect(0,174)&&!success_body_effect(42,3433),"invalid caster/resource lookup rejected");
    for(const auto& row:skill_catalog_2022){
        const auto effect=success_body_effect(42,row.id);
        check(bool(effect)==(row.caster_body_resource<2077),"caster-body sentinel policy");
        if(effect)check(*effect==generic(21,42,row.caster_body_resource),"native resource rather than skill ID is published");
    }
    check(!success_body_effect(42,345)&&skill_success_body_effect(42,345)==generic(21,42,618),"Defensive Stance explicit captured self target-body is separate from caster sentinel");
    check(!skill_success_body_effect(0,345)&&!skill_success_body_effect(42,2192),"explicit body exception does not invent other target effects");
    check(generic(48,42,1043)==Packet({158,0,48,0,0,0,42,0,0,0,19,4,0,0}),"native instant stance action fixture");
    Policy p;p.primary=3;p.secondary=1;p.campaigns=0x1e;p.can_change=true;p.learned={{281,1},{282,1},{286,1},{313,1},{1,1}};
    Slots bar{};bar[0]={281,0};bar[1]={282,0};bar[2]={286,0};bar[3]={313,0};bar[4]={1,0};
    attributes::Ranks effective{};effective[13]=12;effective[16]=10;
    vitals::Resources self{100,480,20,20},ally{100,480,20,20};std::vector<Packet> packets;unsigned heals=0;bool valid=true;
    Hooks hooks;
    hooks.target_valid=[&](unsigned target,float,bool allow_self){return valid&&(target==42?allow_self:target==43);};
    hooks.consume_energy=[&](unsigned cost){if(self.energy<cost)return false;self.energy-=cost;return true;};
    hooks.resources=[&](unsigned target)->std::optional<vitals::Resources>{if(target==42)return self;if(target==43)return ally;return std::nullopt;};
    hooks.heal=[&](unsigned target,unsigned amount,float,bool){++heals;auto& r=target==42?self:ally;r.health=std::min(r.max_health,r.health+amount);return true;};
    hooks.events=[&](const std::vector<Packet>& values){packets.insert(packets.end(),values.begin(),values.end());};
    Runtime casts(42);
    check(casts.start(0,{281,0,42},bar,p,effective,true,hooks)==casting::Reason::accepted,"owned Orison cast accepted");
    check(packets.size()==2&&packets[0][0]==227&&packets[1]==activation(42,42,281),"accepted normal cast publishes pending and specified body action");
    check(self.energy==15&&casts.active()&&heals==0,"energy consumed atomically before activation, heal waits");
    casts.update(999,hooks);check(heals==0,"no early heal");casts.update(1000,hooks);
    check(heals==1&&self.health==192,"Orison rank12 heals60 plus DivineFavor32");
    check(packets[packets.size()-3]==Packet({158,0,58,0,0,0,42,0,0,0,0,0,0,0})&&packets[packets.size()-2]==cancellation_packet(42,{281,0})&&packets.back()[0]==228&&packets.back()[12]==2,"successful body finish, pending acknowledgment and INTEGER recharge are distinct");
    check(casts.start(1001,{281,0,42},bar,p,effective,true,hooks)==casting::Reason::busy,"aftercast blocks new activation");
    check(casts.start(1750,{281,0,42},bar,p,effective,true,hooks)==casting::Reason::recharge,"server rejects optimistic recharge bypass");
    casts.update(3000,hooks);check(packets.back()[0]==229,"ready sent after exact expiry");
    check(casts.start(3000,{282,0,43},bar,p,effective,true,hooks)==casting::Reason::accepted,"WordofHealing ally accepted");casts.update(3750,hooks);
    check(ally.health==311,"WordHealing below50 conditional bonus plus DivineFavor");
    check(casts.start(4500,{286,0,42},bar,p,effective,true,hooks)==casting::Reason::target,"HealOther disallows caster");
    const auto energy=self.energy;check(casts.start(4500,{1,0,42},bar,p,effective,true,hooks)==casting::Reason::unsupported&&self.energy==energy,"unsupported armor effect cannot fake success");
    check(casts.start(4500,{313,0,42},bar,p,effective,true,hooks)==casting::Reason::accepted,"HealingTouch begins");valid=false;casts.update(5250,hooks);
    check(heals==2&&!casts.active()&&packets.back()[0]==158,"target invalidation cancels pending without heal");
    check(attunement_refund(5)==2&&attunement_refund(10)==4&&attunement_refund(15)==6&&attunement_refund(25)==8,"attunement half-to-even rounding");
    Policy elemental;elemental.primary=6;elemental.secondary=7;elemental.campaigns=0x1e;elemental.can_change=true;elemental.learned={{169,1},{1043,1}};Slots elemental_bar{};elemental_bar[0]={169,0};elemental_bar[1]={1043,0};attributes::Ranks earth{};earth[9]=12;
    vitals::Resources elemental_self{480,480,60,60};unsigned refunded=0;double speed=1.;std::vector<Packet> elemental_packets;Hooks elemental_hooks;
    elemental_hooks.target_valid=[](unsigned id,float,bool){return id==50;};elemental_hooks.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==50?std::optional(elemental_self):std::nullopt;};elemental_hooks.consume_energy=[&](unsigned n){if(n>elemental_self.energy)return false;elemental_self.energy-=n;return true;};elemental_hooks.events=[&](const auto& b){elemental_packets.insert(elemental_packets.end(),b.begin(),b.end());};elemental_hooks.restore_energy=[&](unsigned n){refunded+=n;elemental_self.energy=std::min(elemental_self.max_energy,elemental_self.energy+n);return true;};elemental_hooks.speed_multiplier=[&](double n){speed=n;return true;};
    Runtime elemental_runtime(50);check(elemental_runtime.start(0,{169,0,999},elemental_bar,elemental,earth,true,elemental_hooks)==casting::Reason::accepted,"attunement uses self despite selected other target");elemental_runtime.update(1000,elemental_hooks);check(elemental_self.energy==54&&refunded==4,"Earth attunement gains actual Earth-use refund");check(elemental_packets[2][0]==65&&elemental_packets[2].size()==20,"native timed enchantment effect");
    const auto read=[](const Packet& b,unsigned at){unsigned n=0;for(unsigned i=0;i<4;++i)n|=unsigned(b.at(at+i))<<(8*i);return n;};check(std::bit_cast<float>(read(elemental_packets[2],16))==55.f,"pinned36..60 duration scaled rank12");
    check(elemental_runtime.start(1750,{1043,0,0},elemental_bar,elemental,earth,true,elemental_hooks)==casting::Reason::accepted,"instant Dash accepted");elemental_runtime.update(1750,elemental_hooks);check(speed==1.5&&elemental_self.energy==49,"Dash actual speed and energy effect");check(std::find(elemental_packets.begin(),elemental_packets.end(),generic(21,50,1176))!=elemental_packets.end(),"successful Dash publishes native body-effect resource, not skill ID");elemental_runtime.update(4749,elemental_hooks);check(speed==1.5,"Dash remains through duration");elemental_runtime.update(4750,elemental_hooks);check(speed==1.&&elemental_packets.back()[0]==67,"Dash expires exactly and clears icon/speed");
    elemental_runtime.update(56000,elemental_hooks);check(elemental_packets.back()[0]==229||elemental_packets.back()[0]==67,"attunement expiry and recharge handled");
    Hooks missing=elemental_hooks;missing.speed_multiplier={};Runtime no_speed(50);check(no_speed.start(0,{1043,0,0},elemental_bar,elemental,earth,true,missing)==casting::Reason::unsupported,"no speed hook cannot fake Dash success");
    elemental.learned[174]=1;elemental.learned[937]=1;elemental_bar[2]={174,0};elemental_bar[3]={937,0};
    unsigned aoes=0;std::string phase;unsigned last_damage=0;
    elemental_hooks.hostile_aoe=[&](const effects::AoEPlan& plan){++aoes;last_damage=static_cast<unsigned>(plan.strikes[0].earth_damage);return true;};
    elemental_hooks.diagnostic=[&](unsigned,const char* value){phase=value;};
    Runtime aoe_runtime(50);elemental_self.energy=60;
    check(aoe_runtime.start(0,{174,0,999},elemental_bar,elemental,earth,true,elemental_hooks)==casting::Reason::accepted&&phase=="began","self-centered AoE does not require selected foe");
    aoe_runtime.update(749,elemental_hooks);check(aoes==0,"AoE waits for actual activation");
    aoe_runtime.update(750,elemental_hooks);check(aoes==1&&last_damage==85&&elemental_self.energy==50&&phase=="applied"&&elemental_packets.back()[0]==228,"empty validated hostile AoE completes and recharges");
    const auto count_body=[&](unsigned resource){return std::count(elemental_packets.begin(),elemental_packets.end(),generic(21,50,resource));};
    check(count_body(313)==1&&count_body(1182)==0,"successful Aftershock publishes exact caster-body resource once");
    const auto captured_shake=particles::aftershock_screen_shake_2022(50);
    check(captured_shake==Packet({69,0,0,0,128,65,0,0,128,62,50,0,0,0}),"Aftershock wire must match captured16.0/0.25 actor-relative shake");
    const auto count_shake=[&](){return std::count(elemental_packets.begin(),elemental_packets.end(),captured_shake);};
    check(count_shake()==1,"Aftershock empty AoE publishes exactly one caster camera shake");
    aoe_runtime.update(751,elemental_hooks);check(count_shake()==1,"Aftershock repeated update does not duplicate shake");
    Runtime shockwave_visual(50);check(shockwave_visual.start(2000,{937,0,0},elemental_bar,elemental,earth,true,elemental_hooks)==casting::Reason::accepted,"Shockwave visual fixture begin");shockwave_visual.update(2999,elemental_hooks);check(count_body(1182)==0,"Shockwave body effect waits actual activation");shockwave_visual.update(3000,elemental_hooks);shockwave_visual.update(3001,elemental_hooks);check(count_body(1182)==1,"successful Shockwave exact body effect once");
    Runtime cancelled_visual(50);check(cancelled_visual.start(4000,{937,0,0},elemental_bar,elemental,earth,true,elemental_hooks)==casting::Reason::accepted,"cancelled Shockwave visual begins");cancelled_visual.cancel(elemental_hooks);cancelled_visual.update(5000,elemental_hooks);check(count_body(1182)==1,"cancelled Shockwave never publishes success body effect");
    Hooks no_aoe=elemental_hooks;no_aoe.hostile_aoe={};Runtime unavailable_aoe(50);
    check(unavailable_aoe.start(0,{937,0,0},elemental_bar,elemental,earth,true,no_aoe)==casting::Reason::unsupported,"missing AoE mechanics cannot fake success");
    elemental_hooks.hostile_aoe=[](const effects::AoEPlan&){return false;};Runtime failed_aoe(50);
    check(failed_aoe.start(0,{937,0,0},elemental_bar,elemental,earth,true,elemental_hooks)==casting::Reason::accepted,"reviewed AoE starts");failed_aoe.update(1000,elemental_hooks);
    check(phase=="effect_failed"&&elemental_packets.back()[0]==158&&elemental_packets[elemental_packets.size()-2][0]==226,"failed AoE cancels without recharge/falsefinish");
    check(count_body(1182)==1,"failed AoE including missing armor never publishes success body effect");
    check(count_shake()==1,"Shockwave success/cancellation/failure must not invent Aftershock camera shake");
    elemental_self.energy=60;Runtime cancelled_aftershock(50);
    check(cancelled_aftershock.start(6000,{174,0,0},elemental_bar,elemental,earth,true,elemental_hooks)==casting::Reason::accepted,"cancelled Aftershock starts");
    cancelled_aftershock.cancel(elemental_hooks);cancelled_aftershock.update(6750,elemental_hooks);
    check(count_shake()==1,"cancelled Aftershock never shakes camera");
    Runtime failed_aftershock(50);check(failed_aftershock.start(7000,{174,0,0},elemental_bar,elemental,earth,true,elemental_hooks)==casting::Reason::accepted,"failed Aftershock starts");
    failed_aftershock.update(7750,elemental_hooks);check(count_shake()==1,"failed world commit never shakes camera");
    elemental.learned[217]=1;elemental_bar[4]={217,0};elemental_self.energy=60;
    const auto crystal12=effects::crystal_wave(12);
    check(crystal12&&crystal12->base_damage==58&&crystal12->per_condition_damage==13&&effects::crystal_wave_damage(*crystal12,2)==84,"CrystalWave reviewed damage plus two removed conditions");
    check(effects::crystal_wave(0)->base_damage==10&&effects::crystal_wave(15)->base_damage==70&&effects::crystal_wave(20)->per_condition_damage==18&&!effects::crystal_wave(21),"CrystalWave reviewed rank endpoints and bounds");
    check(!effects::crystal_wave_damage(*crystal12,129),"CrystalWave condition-count bound");
    auto forged_crystal=*crystal12;forged_crystal.base_damage=999;check(!effects::crystal_wave_damage(forged_crystal,0),"CrystalWave refuses noncanonical plan");
    Hooks crystal_hooks=elemental_hooks;crystal_hooks.crystal_wave={};Runtime missing_crystal(50);
    check(missing_crystal.start(0,{217,0,0},elemental_bar,elemental,earth,true,crystal_hooks)==casting::Reason::unsupported&&elemental_self.energy==60,"CrystalWave no hook rejects before cost");
    unsigned crystal_calls=0;crystal_hooks.crystal_wave=[&](const effects::CrystalWavePlan& plan,uint64_t at){check(plan.skill==217&&plan.rank==12&&at==750,"CrystalWave self-centered completion uses captured rank/time");++crystal_calls;return true;};
    Runtime crystal_runtime(50);check(crystal_runtime.start(0,{217,0,999},elemental_bar,elemental,earth,true,crystal_hooks)==casting::Reason::accepted&&elemental_self.energy==45,"CrystalWave ignores selected enemy and consumes15energy");
    crystal_runtime.update(749,crystal_hooks);check(!crystal_calls,"CrystalWave waits750ms");crystal_runtime.update(750,crystal_hooks);check(crystal_calls==1&&read(elemental_packets.back(),12)==15,"CrystalWave successful empty foe set finishes with15secondrecharge");
    check(crystal_runtime.start(1500,{217,0,0},elemental_bar,elemental,earth,true,crystal_hooks)==casting::Reason::recharge,"CrystalWave recharge enforced");
    crystal_runtime.update(15750,crystal_hooks);check(elemental_packets.back()[0]==229,"CrystalWave exact recharge expiry");
    crystal_hooks.crystal_wave=[](const effects::CrystalWavePlan&,uint64_t){return false;};Runtime failed_crystal(50);elemental_self.energy=60;
    check(failed_crystal.start(0,{217,0,0},elemental_bar,elemental,earth,true,crystal_hooks)==casting::Reason::accepted,"CrystalWave failedworld starts");failed_crystal.update(750,crystal_hooks);check(elemental_packets.back()==generic(59,50,0),"CrystalWave failure cancels without falsefinish");
    p.learned[958]=1;bar[5]={958,0};self.energy=20;ally.health=100;
    float ally_distance=625.f,seen_range=0;bool seen_self=true;
    Hooks whisper_hooks=hooks;
    whisper_hooks.target_valid=[&](unsigned target,float range,bool allow_self){seen_range=range;seen_self=allow_self;return target==42?allow_self:(target==43&&ally_distance<=range);};
    Runtime whispers(42);
    check(whispers.start(0,{958,0,42},bar,p,effective,true,whisper_hooks)==casting::Reason::target&&self.energy==20,"Healing Whisper cannot heal self or consume rejected cost");
    check(whispers.start(0,{958,0,43},bar,p,effective,true,whisper_hooks)==casting::Reason::target&&seen_range==624.f&&!seen_self,"Healing Whisper rejects beyond half spell range");
    ally_distance=624.f;
    check(whispers.start(0,{958,0,43},bar,p,effective,true,whisper_hooks)==casting::Reason::accepted&&self.energy==15,"Healing Whisper boundary target and exact cost");
    whispers.update(999,whisper_hooks);check(ally.health==100,"Healing Whisper activation delay");
    whispers.update(1000,whisper_hooks);check(ally.health==220,"Healing Whisper rank12 base88 plus DivineFavor32");
    check(whispers.start(1750,{958,0,43},bar,p,effective,true,whisper_hooks)==casting::Reason::recharge,"Healing Whisper genuine recharge after aftercast");
    check(effects::direct_heal_amount(282,12,0,240,480)==81&&effects::direct_heal_amount(282,12,0,239,480)==179,"Word of Healing below-half condition excludes exact-half");
    check(!effects::direct_heal_amount(2,12,0,1,480)&&!effects::direct_heal_amount(958,21,0,1,480),"trusted engine heal resolver rejects unsupported effect and rank");
    // Trusted NPC execution uses explicit instance resources/ranks, never a
    // player learned whitelist, copied account entitlement or invented stats.
    ActorContext npc{42,91,3,effective};std::uint64_t generation=91;
    Hooks npc_hooks=whisper_hooks;npc_hooks.actor_generation=[&](){return std::optional(generation);};
    Runtime npc_casts(42);self.energy=20;ally.health=100;
    check(npc_casts.start_trusted(0,{958,0,43},npc,true,npc_hooks)==casting::Reason::accepted,"trusted actor direct heal independent of client learned/bar gates");
    npc_casts.update(1000,npc_hooks);check(ally.health==220&&self.energy==15,"trusted actor uses explicit ranks and actual resources");
    Runtime stale_npc(42);self.energy=20;ally.health=100;
    check(stale_npc.start_trusted(0,{958,0,43},npc,true,npc_hooks)==casting::Reason::accepted,"trusted pending cast begins");
    const auto old_packets=packets.size();generation=92;stale_npc.update(1000,npc_hooks);
    check(ally.health==100&&packets.size()==old_packets&&!stale_npc.active(),"recycled actor ID cancels without mutating or notifying replacement");
    check(stale_npc.start_trusted(2000,{958,0,43},npc,true,npc_hooks)==casting::Reason::policy,"old generation cannot execute replacement");
    ActorContext wrong=npc;wrong.actor=43;Runtime wrong_npc(42);const auto remaining=self.energy;
    check(wrong_npc.start_trusted(0,{958,0,43},wrong,true,npc_hooks)==casting::Reason::policy&&self.energy==remaining,"wrong actor identity cannot spend resources");
    generation=91;Runtime trusted_dash(50);ActorContext runner{50,91,7,earth};Hooks runner_hooks=elemental_hooks;runner_hooks.actor_generation=[&](){return std::optional(generation);};elemental_self.energy=20;
    check(trusted_dash.start_trusted(0,{1043,0,0},runner,true,runner_hooks)==casting::Reason::accepted,"trusted self buff has no fabricated player slot");trusted_dash.update(0,runner_hooks);check(speed==1.5,"trusted Dash applies reviewed multiplier");
    const auto old_effect_packets=elemental_packets.size();generation=92;trusted_dash.update(3000,runner_hooks);
    check(speed==1.5&&elemental_packets.size()==old_effect_packets,"expired stale buff never resets replacement actor speed");
    generation=91;runner.primary=6;unsigned trusted_aoes=0;runner_hooks.hostile_aoe=[&](const effects::AoEPlan& plan){++trusted_aoes;return plan.skill==174&&plan.rank==12;};
    Runtime trusted_earth(50);elemental_self.energy=20;
    check(trusted_earth.start_trusted(0,{174,0,0},runner,true,runner_hooks)==casting::Reason::accepted,"trusted caster-centered AoE bypasses player learned whitelist");trusted_earth.update(750,runner_hooks);
    check(trusted_aoes==1&&elemental_self.energy==10,"trusted reviewed AoE executes actual callback/cost");
    Hooks unknown_resources=runner_hooks;unknown_resources.resources=[](unsigned)->std::optional<vitals::Resources>{return std::nullopt;};Runtime unknown_actor(50);
    check(unknown_actor.start_trusted(0,{174,0,0},runner,true,unknown_resources)==casting::Reason::energy&&elemental_self.energy==10,"unknown NPC health cannot become fabricated caster state");
    Runtime denied_status(50);elemental_self.energy=20;speed=1.;
    unsigned registrations=0,removals=0;
    runner_hooks.status_mutation=[&](unsigned skill,unsigned id,unsigned rank,Milliseconds now,std::optional<Milliseconds> expires,bool active){++registrations;return false;};
    const auto before_denial=elemental_packets.size();
    check(denied_status.start_trusted(0,{1043,0,0},runner,true,runner_hooks)==casting::Reason::accepted,"status denial tested after ordinary activation");
    denied_status.update(0,runner_hooks);
    check(registrations==1&&speed==1.&&elemental_packets.size()==before_denial+4,"failed authoritative registration cannot publish buff or mutate speed");
    Runtime registered_status(50);elemental_self.energy=20;
    runner_hooks.status_mutation=[&](unsigned skill,unsigned id,unsigned rank,Milliseconds now,std::optional<Milliseconds> expires,bool active){if(active){check(skill==1043&&id==1&&expires==3000,"exact reviewed status record");++registrations;}else ++removals;return true;};
    check(registered_status.start_trusted(0,{1043,0,0},runner,true,runner_hooks)==casting::Reason::accepted,"authoritative buff starts");registered_status.update(0,runner_hooks);
    check(speed==1.5,"registered buff applies speed");const auto after_instant=elemental_packets.size();registered_status.cancel(runner_hooks);
    check(speed==1.5&&elemental_packets.size()==after_instant,"movement cancellation after committed instant Dash cannot cancel buff or emit stop59");registered_status.update(3000,runner_hooks);
    check(removals==1&&speed==1.,"expiry reconciles exact authoritative status before presentation reset");
    {
        Hooks stance_hooks=runner_hooks;ActorContext warrior=runner;warrior.primary=1;
        effects::Scope scope{1,50,91};effects::Registry registry(scope);unsigned applied_count=0;
        stance_hooks.status_mutation=[&](unsigned skill,unsigned id,unsigned rank,Milliseconds now,std::optional<Milliseconds> ends,bool active){
            if(!active){registry.remove(scope,id);return true;}
            check((skill==346||skill==1043)&&rank==0&&ends==now+(skill==346?8000:3000),"stance exact native duration and rank");++applied_count;
            return registry.apply(scope,{id,50,skill,rank,effects::Category::stance,now,ends},effects::Replace::category).accepted;
        };
        stance_hooks.effect_snapshot=[&](Milliseconds now)->std::optional<std::vector<effects::Entry>>{return registry.active(scope,now);};
        Runtime stance(50);elemental_self.energy=20;speed=1.;const auto before=elemental_packets.size();
        check(stance.start_trusted(0,{346,0,999},warrior,true,stance_hooks)==casting::Reason::accepted,"Frenzy instant self stance starts");
        stance.update(0,stance_hooks);check(elemental_self.energy==15&&applied_count==1&&speed==1.,"Frenzy spends5 without movement speed change");
        check(effects::frenzy_active(registry.active(scope,7999),7999)&&!effects::frenzy_active(registry.active(scope,8000),8000),"Frenzy8second boundary");
        check(std::find(elemental_packets.begin()+before,elemental_packets.end(),generic(48,50,346))!=elemental_packets.end(),"Frenzy native instant activation");
        check(std::find(elemental_packets.begin()+before,elemental_packets.end(),generic(21,50,601))!=elemental_packets.end(),"Frenzy pinned caster body601");
        check(stance.start_trusted(3999,{346,0,0},warrior,true,stance_hooks)==casting::Reason::recharge,"Frenzy4second recharge enforced");
        stance.update(4000,stance_hooks);check(stance.start_trusted(4000,{346,0,0},warrior,true,stance_hooks)==casting::Reason::accepted,"Frenzy recharges while effect remains");
        stance.update(4000,stance_hooks);check(applied_count==2&&elemental_self.energy==10&&registry.size()==1,"Frenzy renews without stacking");
        const auto before_expiry=elemental_packets.size();stance.update(12000,stance_hooks);
        check(registry.size()==0&&std::find(elemental_packets.begin()+before_expiry,elemental_packets.end(),effect_remove(50,2))!=elemental_packets.end(),"snapshot expiry removes native effect and authoritative record");
        Policy owner;owner.primary=1;owner.campaigns=0x1e;owner.learned={{346,1}};Slots bar{};bar[0]={346,0};attributes::Ranks ranks{};Runtime player_stance(50);
        check(player_stance.start(20000,{346,0,999},bar,owner,ranks,true,stance_hooks)==casting::Reason::accepted,"learned equipped Warrior Frenzy uses player entry point");
        player_stance.update(20000,stance_hooks);check(elemental_self.energy==5&&applied_count==3&&effects::frenzy_active(registry.active(scope,20000),20000),"player Frenzy applies authoritative status");
        registry.clear(scope);elemental_self.energy=20;Runtime swapping(50);
        check(swapping.start_trusted(30000,{1043,0,0},warrior,true,stance_hooks)==casting::Reason::accepted,"Dash replacement fixture starts");swapping.update(30000,stance_hooks);check(speed==1.5,"Dash movement boost active");
        check(swapping.start_trusted(30100,{346,0,0},warrior,true,stance_hooks)==casting::Reason::accepted,"Frenzy replaces Dash");swapping.update(30100,stance_hooks);
        check(speed==1.&&registry.size()==1&&effects::frenzy_active(registry.active(scope,30100),30100),"replacing Dash resets movement immediately");
        swapping.update(33000,stance_hooks);check(speed==1.&&effects::frenzy_active(registry.active(scope,33000),33000),"old Dash expiry cannot alter replacement stance");
        registry.clear(scope);elemental_self.energy=20;Runtime unavailable_snapshot(50);
        check(unavailable_snapshot.start_trusted(40000,{1043,0,0},warrior,true,stance_hooks)==casting::Reason::accepted,"snapshot failure fixture starts Dash");
        const auto live_snapshot=stance_hooks.effect_snapshot;
        stance_hooks.effect_snapshot=[](Milliseconds)->std::optional<std::vector<effects::Entry>>{return std::nullopt;};
        unavailable_snapshot.update(40100,stance_hooks);
        check(speed==1.5&&registry.size()==1,"unavailable snapshot must not retire a live stance or reset speed");
        stance_hooks.effect_snapshot=live_snapshot;registry.clear(scope);
        unavailable_snapshot.update(40200,stance_hooks);
        check(speed==1.,"confirmed absent stance still retires cached movement mechanics");
    }
    p.learned[867]=1;Slots light_bar{};light_bar[0]={867,0};Hooks light_hooks=hooks;unsigned light_refund=0;bool enchanted=false;Milliseconds queried=0;
    light_hooks.has_enchantment=[&](unsigned target,Milliseconds now){check(target==43,"Healing Light queries actual target");queried=now;return enchanted;};
    light_hooks.restore_energy=[&](unsigned amount){light_refund+=amount;self.energy=std::min(self.max_energy,self.energy+amount);return true;};
    Runtime light(42);self.energy=20;ally.health=100;valid=true;
    check(light.start(0,{867,0,43},light_bar,p,effective,true,light_hooks)==casting::Reason::accepted,"Healing Light accepts valid ally");
    enchanted=true;light.update(1000,light_hooks);
    check(ally.health==220&&self.energy==18&&light_refund==3&&queried==1000,"rank12 Light heals88 plus Divine32 and returns3 after completion enchantment check");
    Runtime unenchanted_light(42);self.energy=20;ally.health=100;enchanted=true;
    check(unenchanted_light.start(0,{867,0,43},light_bar,p,effective,true,light_hooks)==casting::Reason::accepted,"Light begins on enchanted target");enchanted=false;unenchanted_light.update(1000,light_hooks);
    check(ally.health==220&&self.energy==15&&light_refund==3,"enchantment expired before completion gives no return");
    Runtime missing_light(42);Hooks no_status=light_hooks;no_status.has_enchantment={};const auto before_missing=self.energy;
    check(missing_light.start(0,{867,0,43},light_bar,p,effective,true,no_status)==casting::Reason::unsupported&&self.energy==before_missing,"missing authoritative status denied before cost");
    Runtime lost_light(42);self.energy=20;ally.health=100;enchanted=true;valid=true;
    check(lost_light.start(0,{867,0,43},light_bar,p,effective,true,light_hooks)==casting::Reason::accepted,"Light pending target loss");valid=false;lost_light.update(1000,light_hooks);
    check(ally.health==100&&self.energy==15&&light_refund==3,"invalidated target neither healed nor refunded");
    p.learned[288]=1;Slots regen_bar{};regen_bar[0]={288,0};Hooks regen_hooks=hooks;valid=true;self.energy=20;
    unsigned regen_calls=0;bool action_began=false;
    regen_hooks.begin_action=[&](Slot used,Milliseconds at){check(used==Slot{288,0}&&at==0&&self.energy==10,"begin action after committed cost");action_began=true;};
    regen_hooks.events=[&](const std::vector<Packet>& values){if(values.front()[0]==227)check(action_began,"movement/emote preparation precedes activation publication");};
    regen_hooks.apply_health_regeneration=[&](unsigned target,const regeneration::Plan& plan,Milliseconds now,Milliseconds expiry,float range,bool self_allowed){++regen_calls;check(target==43&&plan.skill==288&&plan.pips==8&&now==1000&&expiry==16000&&range==1248.f&&self_allowed,"Breeze exact target plan");return true;};
    ally.health=100;const auto before_breeze_heals=heals;
    Runtime breeze_cast(42);check(breeze_cast.start(0,{288,0,43},regen_bar,p,effective,true,regen_hooks)==casting::Reason::accepted,"Breeze starts with genuine world hook");breeze_cast.update(999,regen_hooks);check(regen_calls==0,"Breeze does not grant early regeneration");breeze_cast.update(1000,regen_hooks);check(regen_calls==1&&ally.health==132&&heals==before_breeze_heals+1,"Breeze source success triggers DivineFavor32");
    Runtime failed_breeze(42);Hooks failed_hooks=regen_hooks;failed_hooks.begin_action={};failed_hooks.events=[](const auto&){};failed_hooks.apply_health_regeneration=[](auto,const auto&,auto,auto,auto,auto){return false;};self.energy=20;const auto before_failed_heals=heals;
    check(failed_breeze.start(0,{288,0,43},regen_bar,p,effective,true,failed_hooks)==casting::Reason::accepted,"Breeze failed source begins");failed_breeze.update(1000,failed_hooks);check(heals==before_failed_heals,"failed regeneration application cannot trigger DivineFavor");
    ActorContext secondary_monk{42,91,6,effective};Runtime secondary_breeze(42);Hooks secondary_hooks=failed_hooks;secondary_hooks.actor_generation=[](){return std::optional<std::uint64_t>(91);};secondary_hooks.apply_health_regeneration=[](auto,const auto&,auto,auto,auto,auto){return true;};self.energy=20;
    check(secondary_breeze.start_trusted(0,{288,0,43},secondary_monk,true,secondary_hooks)==casting::Reason::accepted,"secondary Monk Breeze starts");secondary_breeze.update(1000,secondary_hooks);check(heals==before_failed_heals,"non-Monk primary does not trigger DivineFavor from supplied rank");
    Runtime missing_regen(42);regen_hooks.apply_health_regeneration={};self.energy=20;
    check(missing_regen.start(0,{288,0,43},regen_bar,p,effective,true,regen_hooks)==casting::Reason::unsupported&&self.energy==20,"missing world regeneration rejects before cost");
    ActorContext ranger{42,91,2,{}};ranger.effective[24]=12;Runtime troll_cast(42);regen_hooks.actor_generation=[](){return std::optional<std::uint64_t>(91);};regen_hooks.begin_action={};regen_hooks.events=[](const auto&){};
    regen_hooks.apply_health_regeneration=[&](unsigned target,const regeneration::Plan& plan,Milliseconds now,Milliseconds expiry,float,bool){++regen_calls;check(target==42&&plan.skill==446&&plan.pips==9&&now==3000&&expiry==16000,"Troll forces self and uses actual Wilderness rank");return true;};
    check(troll_cast.start_trusted(0,{446,0,43},ranger,true,regen_hooks)==casting::Reason::accepted,"Troll ignores selected ally");troll_cast.update(2999,regen_hooks);check(regen_calls==1,"Troll pinned three-second activation");troll_cast.update(3000,regen_hooks);check(regen_calls==2,"Troll source duration13seconds");
    p.learned[287]=1;p.learned[1397]=1;Slots party_bar{};party_bar[0]={287,0};party_bar[1]={1397,0};Hooks party_hooks=hooks;valid=true;self.energy=20;unsigned party_calls=0;
    party_hooks.party_heal=[&](const effects::PartyHealPlan& plan,Milliseconds now){++party_calls;check(plan.skill==287&&plan.base_heal==66&&plan.caster_divine_heal==32&&now==2000,"party cast reviewed shared plan after actual activation");return true;};
    Runtime party_cast(42);check(party_cast.start(0,{287,0,999},party_bar,p,effective,true,party_hooks)==casting::Reason::accepted&&self.energy==5,"Heal Party ignores selection and pays encoded15energy");party_cast.update(1999,party_hooks);check(!party_calls,"party heal not early");party_cast.update(2000,party_hooks);check(party_calls==1,"party hook called once");
    Runtime lod_cast(42);self.energy=20;party_hooks.party_heal=[&](const effects::PartyHealPlan& plan,Milliseconds now){++party_calls;check(plan.skill==1397&&plan.base_heal==57&&now==1000,"LOD uses genuine bonus57 not101");return true;};
    check(lod_cast.start(0,{1397,0,999},party_bar,p,effective,true,party_hooks)==casting::Reason::accepted&&self.energy==15,"LOD realcost5");lod_cast.update(1000,party_hooks);
    Runtime absent_party(42);party_hooks.party_heal={};self.energy=20;check(absent_party.start(0,{287,0,0},party_bar,p,effective,true,party_hooks)==casting::Reason::unsupported&&self.energy==20,"missing authoritative party hook denies before cost");
    p.learned[301]=1;p.learned[2003]=1;Slots removal_bar{};removal_bar[0]={301,0};removal_bar[1]={2003,0};Hooks removal_hooks=hooks;self.energy=20;unsigned removals_called=0;
    removal_hooks.remove_effects=[&](unsigned target,const effects::RemovalPlan& plan,Milliseconds now){++removals_called;check(target==43&&plan.skill==2003&&plan.kind==effects::Category::hex&&plan.count==1&&plan.conditional_heal==102&&plan.divine_heal==32&&now==1000,"CureHex reviewed amount and targetDF plan");return true;};
    Runtime cure(42);check(cure.start(0,{2003,0,43},removal_bar,p,effective,true,removal_hooks)==casting::Reason::accepted&&self.energy==15,"CureHex pays5 before actual activation");cure.update(999,removal_hooks);check(!removals_called,"no early hex removal");cure.update(1000,removal_hooks);check(removals_called==1,"atomic world removal called once");
    Runtime no_removal(42);removal_hooks.remove_effects={};self.energy=20;check(no_removal.start(0,{301,0,43},removal_bar,p,effective,true,removal_hooks)==casting::Reason::unsupported&&self.energy==20,"absent genuine registry removal denies before cost");
    check(effects::removal(301,12,10)->conditional_heal==0&&!effects::removal(302,12,10),"RemoveHex no invented healing and SmiteHex unsupported damage");
    for(unsigned id=275;id<=278;++id)p.learned[id]=1;Slots cleanse_bar{};cleanse_bar[0]={275,0};cleanse_bar[1]={276,0};cleanse_bar[2]={277,0};cleanse_bar[3]={278,0};effective[15]=12;Hooks cleanse_hooks=hooks;self.energy=20;unsigned cleanses=0;
    cleanse_hooks.remove_conditions=[&](unsigned target,const effects::ConditionRemovalPlan& plan,Milliseconds now){++cleanses;check(target==43&&plan.skill==276&&plan.count==10&&plan.per_condition_heal==58&&plan.divine_heal==32&&now==750,"Restore Condition uses Protection12 not Healing rank");return true;};
    Runtime restore(42);check(restore.start(0,{276,0,42},cleanse_bar,p,effective,true,cleanse_hooks)==casting::Reason::target&&self.energy==20,"RestoreCondition others-only");check(restore.start(0,{276,0,43},cleanse_bar,p,effective,true,cleanse_hooks)==casting::Reason::accepted&&self.energy==15,"RestoreCondition actual cost");restore.update(749,cleanse_hooks);check(!cleanses,"no early cleanse");restore.update(750,cleanse_hooks);check(cleanses==1,"world owns atomic condition state");
    auto mend=effects::condition_removal(275,12,10),ailment=effects::condition_removal(277,12,10),purge=effects::condition_removal(278,0,10);
    check(mend&&!mend->self&&effects::condition_heal(*mend,1,2)==89,"MendCondition heals only one removed plusDF");
    check(ailment&&ailment->self&&effects::condition_heal(*ailment,1,2)==146,"MendAilment heals remaining count plusDF");
    check(purge&&purge->count==10&&effects::condition_heal(*purge,3,0)==32,"Purge no invented baseheal");
    check(effects::condition_heal(*mend,0,0)==32&&!effects::condition_heal(*mend,9,9),"emptysetDivine and impossible counts bounded");
    cleanse_hooks.remove_conditions={};Runtime unsupported_cleanse(42);self.energy=20;check(unsupported_cleanse.start(0,{278,0,42},cleanse_bar,p,effective,true,cleanse_hooks)==casting::Reason::unsupported&&self.energy==20,"absent real condition state denies beforecost");
    // Factions duplicate follows the same other-ally mechanics, not fabricated
    // generic heal support for every spell or NPC catalog row.
    p.learned={{1120,1}};Slots gaze_bar{};gaze_bar[0]={1120,0};Runtime gaze(42);self.energy=20;ally.health=100;valid=true;
    check(gaze.start(0,{1120,0,42},gaze_bar,p,effective,true,hooks)==casting::Reason::target&&self.energy==20,"Jamei forbids self without cost");
    check(gaze.start(0,{1120,0,43},gaze_bar,p,effective,true,hooks)==casting::Reason::accepted&&self.energy==10,"Jamei genuine10energy activation");
    gaze.update(749,hooks);check(ally.health==100,"Jamei no early heal");gaze.update(750,hooks);check(ally.health==283,"Jamei rank12 heals151 plus32DivineFavor");
    check(effects::direct_heal_amount(1120,0,0,100,480)==35&&effects::direct_heal_amount(1120,15,0,100,480)==180,"Jamei pinned endpoints");
    {
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned={{1686,1}};
        Slots slots{};slots[0]={1686,0};attributes::Ranks ranks{};ranks[13]=12;ranks[16]=10;
        vitals::Resources caster{480,480,20,20},recipient{100,480,20,20};
        Hooks h;std::vector<Packet> output;unsigned healed=0;bool eligible=true,commit=true;
        h.events=[&](const auto& batch){output.insert(output.end(),batch.begin(),batch.end());};
        h.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?caster:actor==43?std::optional(recipient):std::nullopt;};
        h.target_valid=[&](unsigned actor,float range,bool allow_self){return eligible&&range==1248.f&&(actor==43||(actor==42&&allow_self));};
        h.consume_energy=[&](unsigned n){if(n>caster.energy)return false;caster.energy-=n;return true;};
        h.heal=[&](unsigned actor,unsigned amount,float range,bool allow_self){check(range==1248&&allow_self,"Glimmer target rules");if(!commit)return false;healed=amount;auto& target=actor==42?caster:recipient;target.health=std::min(target.max_health,target.health+amount);return true;};
        Runtime glimmer(42);
        eligible=false;check(glimmer.start(0,{1686,0,43},slots,policy,ranks,true,h)==casting::Reason::target&&caster.energy==20,"Glimmer invalid target no cost");eligible=true;
        check(glimmer.start(0,{1686,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted&&caster.energy==15,"Glimmer exact5energy");
        const auto particle=*particles::target_body_2022(43,1686);
        glimmer.update(249,h);check(!healed&&std::find(output.begin(),output.end(),particle)==output.end(),"Glimmer no early heal or target particle");
        glimmer.update(250,h);check(healed==126&&recipient.health==226&&std::count(output.begin(),output.end(),particle)==1,"Glimmer rank12 heal94 plus32DivineFavor and target resource1653");
        check(glimmer.start(1000,{1686,0,43},slots,policy,ranks,true,h)==casting::Reason::recharge&&caster.energy==15,"Glimmer exact1second recharge after completion");
        check(glimmer.start(1250,{1686,0,42},slots,policy,ranks,true,h)==casting::Reason::accepted,"Glimmer self targeting allowed");glimmer.update(1500,h);
        check(effects::direct_heal_amount(1686,0,0,1,480)==10&&effects::direct_heal_amount(1686,15,0,1,480)==115,"Glimmer pinned endpoints");
        Runtime failed(42);output.clear();commit=false;caster.energy=20;
        check(failed.start(0,{1686,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"Glimmer failed commit setup");failed.update(250,h);
        check(std::find(output.begin(),output.end(),particle)==output.end()&&output.back()==generic(59,42,0),"Glimmer failed heal emits no target success particle");
    }
    p.learned={{283,1}};Slots kiss_bar{};kiss_bar[0]={283,0};Runtime kiss(42);self.energy=20;ally.health=100;
    check(kiss.start(0,{283,0,43},kiss_bar,p,effective,true,hooks)==casting::Reason::unsupported&&self.energy==20,"Kiss absent authoritative status hook denies before cost");
    unsigned status_heals=0;hooks.status_heal=[&](unsigned target,const effects::StatusHealPlan& plan,uint64_t now){check(target==43&&now==1000,"Kiss compound target/time");auto amount=effects::status_heal_amount(plan,2,1);check(amount==173,"Kiss actual3statuses adds90 plus51base32DF");++status_heals;return true;};
    check(kiss.start(0,{283,0,42},kiss_bar,p,effective,true,hooks)==casting::Reason::target,"Kiss cannot self target");
    check(kiss.start(0,{283,0,43},kiss_bar,p,effective,true,hooks)==casting::Reason::accepted&&self.energy==15,"Kiss proven cost activation");kiss.update(999,hooks);check(!status_heals,"Kiss no early status count");kiss.update(1000,hooks);check(status_heals==1,"Kiss compound callback executed once");check(kiss.start(1750,{283,0,43},kiss_bar,p,effective,true,hooks)==casting::Reason::recharge&&self.energy==15,"Kiss recharge rejects without extra cost");
    const auto status_plan=*effects::status_heal(283,12,10);check(effects::status_heal_amount(status_plan,0,0)==83,"Kiss empty set still base plusDF");check(!effects::status_heal_amount(status_plan,129,0),"Kiss boundedstatus count");
    {
        Policy policy;policy.primary=6;policy.secondary=7;policy.campaigns=0x1e;policy.learned={{171,1},{1374,1},{1661,1}};Slots slots{};slots[0]={171,0};slots[1]={1374,0};slots[2]={1661,0};attributes::Ranks ranks{};ranks[9]=12;ranks[12]=11;
        unsigned energy=60,launched=0;bool foe=true;effects::EarthProjectilePlan last;Hooks h;
        h.events=[](const auto&){};h.target_valid=[](auto,float,bool){return false;};
        h.projectile_target_valid=[&](unsigned target,float range){return foe&&target==43&&range==1248;};
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42||id==43?std::optional(vitals::Resources{480,480,energy,60}):std::nullopt;};
        h.consume_energy=[&](unsigned n){if(n>energy)return false;energy-=n;return true;};
        h.launch_projectile=[&](unsigned target,const auto& plan,uint64_t){check(target==43&&effects::valid_earth_projectile(plan),"projectile reviewed launch");last=plan;++launched;return true;};
        Runtime runtime(42);check(runtime.start(0,{171,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted&&energy==45,"Stoning uses foe gate and true15energy");runtime.update(999,h);check(!launched,"projectile waits activation");runtime.update(1000,h);check(launched==1&&last.earth_damage==93&&last.knockdown_if_weak,"Stoning queues hit conditional effects");
        check(runtime.start(1750,{1661,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"Glowstone foe accepted");runtime.update(2500,h);check(launched==2&&last.energy_storage_rank==11&&effects::glowstone_refund(last,true)==10&&!effects::glowstone_refund(last,false),"Glowstone snapshots primary ES and refunds only actual weak hit");
        foe=false;const auto before=energy;check(runtime.start(3250,{1374,0,43},slots,policy,ranks,true,h)==casting::Reason::target&&energy==before,"invalid foe no cost");foe=true;check(runtime.start(3250,{1374,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"Ebon Hawk accepted");foe=false;runtime.update(4250,h);check(launched==2,"lost foe cannot launch");
    }
    {
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{2,1},{305,1},{314,1}};
        Slots slots{};slots[0]={2,0};slots[1]={305,0};slots[2]={314,0};attributes::Ranks ranks{};ranks[13]=12;
        vitals::Resources caster{480,480,40,40},corpse{0,200,0,40};effects::Scope lifetime{1,43,1};
        std::vector<Packet> events;Hooks h;h.events=[&](const auto& packets){events.insert(events.end(),packets.begin(),packets.end());};
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{if(id==42)return caster;if(id==43)return corpse;return {};};
        h.consume_energy=[&](unsigned n){if(caster.energy<n)return false;caster.energy-=n;return true;};
        h.target_valid=[](auto,float,bool){return false;};
        h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?std::optional(lifetime):std::nullopt;};
        h.resurrection_target_valid=[&](unsigned id,float range){return id==43&&!corpse.health&&(range==1248||range==144);};
        h.resurrect=[&](effects::Scope scope,const effects::ResurrectionPlan& plan,uint64_t){if(scope!=lifetime||corpse.health)return false;corpse.health=corpse.max_health*plan.health_percent/100;corpse.energy=corpse.max_energy*plan.energy_percent/100;return true;};
        Runtime runtime(42);check(runtime.start(0,{2,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"Signet accepts deadparty target without energycost");
        runtime.update(2999,h);check(!corpse.health&&caster.energy==40,"Signet waits exact3seconds and costszero");
        runtime.update(3000,h);check(corpse.health==200&&corpse.energy==10&&events.back()==slot_event(230,42,{2,0}),"Signet100health25energy and native indefinite230");
        corpse.health=0;runtime.update(100000000,h);check(runtime.start(100000000,{2,0,43},slots,policy,ranks,true,h)==casting::Reason::recharge,"Signet never auto recharges from elapsedtime");
        runtime.morale_boost(h);check(events.back()==slot_event(229,42,{2,0}),"genuine morale clears spentsignet native229");
        check(runtime.start(100000000,{2,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"morale permits signet reuse");
        ++lifetime.generation;runtime.update(100003000,h);check(!corpse.health&&events.back()==generic(59,42,0),"recycledcorpse ID cancels old resurrectioncast");
        Runtime spell(42);check(spell.start(0,{305,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted&&caster.energy==30,"Resurrect charges10energy");
        spell.update(4999,h);check(!corpse.health,"Resurrect waits5secondactivation");spell.update(5000,h);
        check(corpse.health==50&&corpse.energy==0&&events.back()==slot_event(228,42,{305,0},8),"Resurrect25healthzeroenergy8secrecharge");
        corpse.health=0;check(spell.start(5750,{305,0,43},slots,policy,ranks,true,h)==casting::Reason::recharge,"spell normalrecharge enforced");
        spell.update(13000,h);check(spell.start(13000,{305,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"spell timedrecharge completes");
        corpse.health=1;spell.update(18000,h);check(corpse.health==1&&events.back()==generic(59,42,0),"anotherresurrection winningrace cancels safely");
        corpse.health=0;Runtime touch(42);check(touch.start(0,{314,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"RestoreLife deadparty touch accepted");
        touch.update(3999,h);check(!corpse.health,"RestoreLife waits4seconds");touch.update(4000,h);
        check(corpse.health==112&&corpse.energy==32&&events.back()==slot_event(228,42,{314,0},8),"RestoreLife rank12 native56percenthealth80energy");
        policy.campaigns=0x1e;policy.learned.emplace(1128,1);policy.learned.emplace(304,1);slots[3]={1128,0};slots[4]={304,0};caster.energy=40;corpse.health=0;
        float corpse_distance=625;h.resurrection_target_valid=[&](unsigned id,float range){return id==43&&!corpse.health&&corpse_distance<=range;};
        h.resurrect=[&](effects::Scope scope,const effects::ResurrectionPlan& plan,uint64_t){if(scope!=lifetime||corpse.health)return false;corpse.health=plan.caster_health?std::min(caster.health,corpse.max_health):corpse.max_health*plan.health_percent/100;corpse.energy=corpse.max_energy*plan.energy_percent/100;return true;};
        Runtime chant(42);check(chant.start(0,{1128,0,43},slots,policy,ranks,true,h)==casting::Reason::target&&caster.energy==40,"Chantrejects outsidehalfspellrange beforecost");
        corpse_distance=624;check(chant.start(0,{1128,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted&&caster.energy==30,"Chant10energy halfspellrange boundary");
        caster.health=130;chant.update(5999,h);check(!corpse.health,"Chantwaits6seconds");caster.health=100;chant.update(6000,h);
        check(corpse.health==100&&corpse.energy==11&&events.back()==slot_event(228,42,{1128,0},15),"Chantcompletion currenthealth not caststarthealth and29percentenergy");
        unsigned area_calls=0;h.target_valid=[](unsigned id,float,bool self){return id==42&&self;};
        h.resurrect_party=[&](const effects::PartyResurrectionPlan& plan,uint64_t){check(effects::valid_party_resurrection(plan)&&plan.radius==322,"massreviewedradius");++area_calls;return true;};
        caster.energy=40;Runtime area(42);check(area.start(0,{304,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted&&caster.energy==15,"Light25energy self-centered despite selectedtarget");
        area.update(3999,h);check(!area_calls,"Lightwaits4seconds");area.update(4000,h);check(area_calls==1&&events.back()==slot_event(228,42,{304,0},20),"Lightcommit and20secrecharge");
        policy.learned.emplace(1816,1);slots[5]={1816,0};corpse.health=0;corpse.energy=0;
        unsigned title_rank=7;h.sunspear_rank=[&](){return std::optional(title_rank);};
        Runtime sunspear(42);check(sunspear.start(0,{1816,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"Sunspear signet accepts dead party target");
        title_rank=10;sunspear.update(2999,h);check(!corpse.health,"Sunspear signet waits three-second cast");
        sunspear.update(3000,h);check(corpse.health==200&&corpse.energy==28&&events.back()==slot_event(230,42,{1816,0}),"Sunspear snapshots rank seven and spends signet until morale boost");
        corpse.health=0;check(sunspear.start(4000,{1816,0,43},slots,policy,ranks,true,h)==casting::Reason::recharge,"Sunspear signet cannot recast within mission");
        sunspear.morale_boost(h);check(sunspear.start(4000,{1816,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"morale boost recharges Sunspear signet");
    }
    {
        Policy policy;policy.primary=6;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{224,1},{229,1},{232,1},{865,1}};
        Slots bar{};bar[0]={224,0};bar[1]={229,0};bar[2]={232,0};bar[3]={865,0};attributes::Ranks air{};air[8]=12;
        vitals::Resources self{480,480,60,60},foe{480,480,20,20};effects::Scope target{1,43,5};
        unsigned applications=0;std::vector<Packet> packets;Hooks h;
        h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):id==43?std::optional(foe):std::nullopt;};
        h.consume_energy=[&](unsigned cost){if(cost>self.energy)return false;self.energy-=cost;return true;};
        h.target_valid=[](auto,float,bool){return false;};
        h.projectile_target_valid=[](unsigned id,float range){return id==43&&(range==1248||range==144);};
        h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?std::optional(target):std::nullopt;};
        h.lightning=[&](effects::Scope victim,const effects::LightningPlan& plan,uint64_t){check(victim==target&&effects::valid_lightning(plan),"lightning capturedscope reviewedplan");++applications;return true;};
        Runtime runtime(42);check(runtime.start(0,{224,0,43},bar,policy,air,true,h)==casting::Reason::accepted&&self.energy==50,"Charge costs10energy beforeactivation");
        runtime.update(999,h);check(!applications,"Charge waits1second");runtime.update(1000,h);check(applications==1&&packets.back()==slot_event(228,42,{224,0},8),"Charge complete and8secrecharge");
        check(runtime.start(1750,{229,0,43},bar,policy,air,true,h)==casting::Reason::accepted&&self.energy==35,"Orb encoded11 means15energy");
        runtime.update(3749,h);check(applications==1,"Orb waits2seconds");runtime.update(3750,h);check(applications==2&&packets.back()==slot_event(228,42,{229,0},5),"Orb launchedthroughactualworldhook and5secrecharge");
        check(runtime.start(4500,{232,0,43},bar,policy,air,true,h)==casting::Reason::accepted&&self.energy==30,"Touch costs5energy");
        runtime.update(5249,h);check(applications==2,"Touch waits750ms");runtime.update(5250,h);check(applications==3&&packets.back()==slot_event(228,42,{232,0},10),"Touch complete and10secrecharge");
        runtime.update(9000,h);check(runtime.start(9000,{224,0,43},bar,policy,air,true,h)==casting::Reason::accepted,"Charge timedrecharge releases");
        ++target.generation;runtime.update(10000,h);check(applications==3&&packets.back()==generic(59,42,0),"late replacementactor cancels lightning withnofalseeffect");
        self.energy=60;check(runtime.start(11000,{865,0,43},bar,policy,air,true,h)==casting::Reason::accepted&&self.energy==35,"Hammer native encoded12 means25 energy");
        runtime.update(12999,h);check(applications==3,"Hammer waits two-second activation");runtime.update(13000,h);check(applications==4&&packets.back()==slot_event(228,42,{865,0},4),"Hammer dispatches actual effect then four-second recharge");
        check(runtime.start(13750,{865,0,43},bar,policy,air,true,h)==casting::Reason::recharge,"Hammer recharge blocks early repeat");
        Profile saved;saved.primary=6;saved.profession_mask=1u<<6;saved.learned={{865,1},{2235,1}};saved.bar[0]={865,0};saved.bar[1]={2235,0};
        const auto match=match_profile(saved,true);const auto match_policy=skills::policy(match,0x1e,false,true);
        Runtime pvp_runtime(42);self.energy=60;unsigned pvp_hits=0;
        h.lightning=[&](effects::Scope victim,const effects::LightningPlan& plan,uint64_t){
            check(victim==target&&plan.skill==3396&&plan.damage==82&&plan.cracked_ms==0,"match Hammer variant deals damage without PvE condition");++pvp_hits;return true;};
        check(pvp_runtime.start(0,{865,0,43},match.bar,match_policy,air,true,h)!=casting::Reason::accepted&&self.energy==60,"base request cannot bypass effective PvP bar");
        check(pvp_runtime.start(0,{3396,0,43},match.bar,match_policy,air,true,h)==casting::Reason::accepted&&self.energy==35,"owned base skill permits its effective PvP variant");
        check(pvp_runtime.start(999,{2235,0,43},match.bar,match_policy,air,true,h)==casting::Reason::pve_only&&self.energy==35&&packets.back()==cancellation_packet(42,{2235,0}),"PvE denial during valid cast acknowledges only denied skill without spending energy");
        pvp_runtime.update(1999,h);check(!pvp_hits,"PvP Hammer does not apply before activation");
        pvp_runtime.update(2000,h);check(pvp_hits==1&&packets.back()==slot_event(228,42,{3396,0},4),"PvP effect and recharge acknowledge variant identity");
        check(saved.bar[0].skill==865&&!saved.learned.contains(3396),"match cast preserves saved skill profile");
    }
    {
        Policy policy;policy.primary=5;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{58,1}};
        Slots bar{};bar[0]={58,0};attributes::Ranks ranks{};
        vitals::Resources self{480,480,0,40},foe{480,480,20,20};effects::Scope target{1,43,5};unsigned applied=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):id==43?std::optional(foe):std::nullopt;};
        h.consume_energy=[](unsigned cost){return cost==0;};
        h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned id,float range){return id==43&&range==144;};
        h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?std::optional(target):std::nullopt;};
        h.blindness=[&](effects::Scope victim,const effects::BlindnessPlan& plan,uint64_t){check(victim==target&&plan.skill==58&&plan.blind_caster&&plan.duration==15000,"Midnight captured lifetime and both blind recipients");++applied;return true;};
        Runtime runtime(42);check(runtime.start(0,{58,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Midnight usable at zero Energy");
        runtime.update(1999,h);check(applied==0,"Midnight waits captured two second activation");runtime.update(2000,h);check(applied==1,"Midnight completes");
        check(runtime.start(2750,{58,0,43},bar,policy,ranks,true,h)!=casting::Reason::accepted,"Midnight cannot bypass recharge");
        check(runtime.start(13000,{58,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Midnight recharges");
        ++target.generation;runtime.update(15000,h);check(applied==1,"Midnight rejects replaced target at completion");
    }
    {
        Policy policy;policy.primary=2;policy.secondary=6;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{220,1},{424,1}};
        Slots bar{};bar[0]={424,0};bar[1]={220,0};attributes::Ranks ranks{};ranks[23]=12;ranks[8]=12;
        vitals::Resources self{480,480,60,60},foe{480,480,20,20};effects::Scope target{1,43,5};unsigned applied=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):id==43?std::optional(foe):std::nullopt;};
        h.consume_energy=[&](unsigned cost){if(cost>self.energy)return false;self.energy-=cost;return true;};
        h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned id,float range){return id==43&&(range==144||range==1248);};
        h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?std::optional(target):std::nullopt;};
        h.blindness=[&](effects::Scope victim,const effects::BlindnessPlan& plan,uint64_t){check(victim==target&&effects::valid_blindness(plan),"Control captured lifetime");++applied;return true;};
        Runtime runtime(42);check(runtime.start(0,{424,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==57,"Expertise reduces Dirt five to three");
        runtime.update(999,h);check(applied==0,"Dirt activation waits");runtime.update(1000,h);check(applied==1,"Dirt actual completion");
        check(runtime.start(1750,{220,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==47,"Non-touch Elementalist spell not reduced by Expertise");
        ++target.generation;runtime.update(2500,h);check(applied==1,"Replacement foe aborts control effect");
    }
    {
        Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{154,1}};Slots bar{};bar[0]={154,0};attributes::Ranks ranks{};ranks[7]=12;
        vitals::Resources self{480,480,40,40},foe{480,480,20,20};effects::Scope target{1,43,5};unsigned applied=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):id==43?std::optional(foe):std::nullopt;};h.consume_energy=[&](unsigned n){if(n>self.energy)return false;self.energy-=n;return true;};
        h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned id,float range){return id==43&&range==144;};h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?std::optional(target):std::nullopt;};
        h.condition_transfer=[&](effects::Scope scope,const effects::ConditionTransferPlan& plan,uint64_t,std::optional<double> roll){check(scope==target&&plan.count==3&&!roll,"Touch captured target and Curses rank");++applied;return true;};
        Runtime runtime(42);check(runtime.start(0,{154,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==35,"Touch actual five energy");runtime.update(749,h);check(applied==0,"Touch waits750ms");runtime.update(750,h);check(applied==1,"Touch actual completion");
        check(runtime.start(1500,{154,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Touch zero recharge after aftercast");++target.generation;runtime.update(2250,h);check(applied==1,"Touch recycled target cancels");
    }
    {
        Policy policy;policy.primary=4;policy.secondary=5;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{117,1},{78,1}};Slots bar{};bar[0]={117,0};bar[1]={78,0};attributes::Ranks ranks{};ranks[7]=12;
        vitals::Resources self{480,480,40,40},foe{480,480,20,20};effects::Scope target{1,43,5};unsigned applied=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):id==43?std::optional(foe):std::nullopt;};h.consume_energy=[&](unsigned n){if(n>self.energy)return false;self.energy-=n;return true;};h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned id,float range){return id==43&&range==1248;};h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?std::optional(target):std::nullopt;};
        h.condition_spell=[&](effects::Scope scope,const effects::ConditionSpellPlan& plan,uint64_t){check(scope==target&&effects::valid_condition_spell(plan),"Condition spell captured scope");++applied;return true;};
        Runtime runtime(42);check(runtime.start(0,{117,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==35,"Enfeeble costs five");runtime.update(249,h);check(applied==0,"Condition spell activation");runtime.update(250,h);check(applied==1,"Enfeeble real completion");
        check(runtime.start(1000,{78,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==30,"Epidemic costs five");++target.generation;runtime.update(1250,h);check(applied==1,"Epidemic stale foe aborts");
    }
    {
        Policy policy;policy.primary=2;policy.secondary=4;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{153,1},{156,1},{1077,1}};Slots bar{};bar[0]={153,0};bar[1]={156,0};bar[2]={1077,0};attributes::Ranks ranks{};ranks[4]=12;ranks[23]=12;
        vitals::Resources self{100,480,60,60},foe{480,480,20,20};effects::Scope target{1,43,5};unsigned applied=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):id==43?std::optional(foe):std::nullopt;};h.consume_energy=[&](unsigned n){if(n>self.energy)return false;self.energy-=n;return true;};h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned id,float range){return id==43&&(range==1248||range==144);};h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?std::optional(target):std::nullopt;};
        h.life_steal=[&](effects::Scope scope,const effects::LifeStealPlan& plan,uint64_t){check(scope==target&&effects::valid_life_steal(plan),"Life steal captured scope");++applied;return true;};
        Runtime runtime(42);check(runtime.start(0,{153,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==50,"Gaze non-touch costs10");runtime.update(999,h);check(applied==0,"Gaze one-second activation");runtime.update(1000,h);check(applied==1,"Gaze completed");
        check(runtime.start(1750,{156,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==42,"Touch15energy reduced8 by Expertise12");runtime.update(2500,h);check(applied==2,"Touch actual completion");
        check(runtime.start(3250,{1077,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==34,"Bite independent touch cost");++target.generation;runtime.update(4000,h);check(applied==2,"Bite late replaced foe cancels");
    }
    {
        Policy policy;policy.primary=6;policy.secondary=4;policy.campaigns=0x1e;policy.can_change=true;Slots bar{};unsigned slot=0;for(unsigned id:{183u,186u,189u,191u,1379u,1095u,846u,89u}){policy.learned[id]=1;bar[slot++]={id,0};}
        attributes::Ranks ranks{};ranks[10]=12;ranks[11]=12;ranks[5]=12;ranks[12]=12;vitals::Resources self{480,480,200,200},foe{480,480,20,20};effects::Scope target{1,43,5},own{1,42,6};unsigned applied=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):id==43?std::optional(foe):std::nullopt;};h.consume_energy=[&](unsigned n){if(n>self.energy)return false;self.energy-=n;return true;};h.target_valid=[](unsigned id,float,bool allow){return id==42&&allow;};h.projectile_target_valid=[](unsigned id,float range){return id==43&&(range==1248||range==144);};h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?std::optional(target):id==42?std::optional(own):std::nullopt;};
        h.elemental=[&](effects::Scope scope,const effects::ElementalPlan& plan,uint64_t){check(scope==(plan.caster_center?own:target)&&effects::valid_elemental(plan)&&plan.storage==12,"Elemental full plan and captured scope");++applied;return true;};
        Runtime runtime(42);uint64_t now=0;unsigned expected_energy=200;for(const auto& equipped:bar){const auto* row=find_skill(equipped.skill);expected_energy-=row->energy_cost();const auto count=applied;
            check(runtime.start(now,{equipped.skill,0,equipped.skill==183?0u:43u},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==expected_energy,"Eight elemental skill costs and correct targeting");const auto activation_ms=static_cast<uint64_t>(std::llround(row->activation*1000));runtime.update(now+activation_ms-1,h);check(applied==count,"Native elemental activation waits");runtime.update(now+activation_ms,h);check(applied==count+1,"Elemental effect actually dispatches");now+=activation_ms+static_cast<uint64_t>(std::llround(row->aftercast*1000));
        }
        check(applied==8&&self.energy==120,"Eight full elemental bindings");check(runtime.start(now,{191,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Immolate elapsed recharge ready");++target.generation;runtime.update(now+1000,h);check(applied==8,"Elemental replaced actor aborts");
        bar={};bar[0]={197,0};policy.learned[197]=1;unsigned storms=0;
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):std::nullopt;};
        h.complete_fire_storm=[&](effects::Scope scope,const effects::FireStormPlan& plan,uint64_t)->std::optional<std::vector<Packet>>{
            check(scope==target&&plan==*effects::fire_storm(12),"Fire Storm captured rank/target");++storms;return std::vector<Packet>{};};
        const auto* storm_row=find_skill(197);const auto activation=uint64_t(std::llround(storm_row->activation*1000));
        Runtime storm_cast(42);const auto energy_before=self.energy;
        check(storm_cast.start(50000,{197,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&
            self.energy==energy_before-storm_row->energy_cost(),"Fire Storm NPC target/cost rejected");
        storm_cast.update(50000+activation-1,h);check(storms==0,"Fire Storm committed before activation");
        storm_cast.update(50000+activation,h);check(storms==1,"Fire Storm hook not dispatched");
        storm_cast.update(60000,h);check(storms==1,"Fire Storm duplicate dispatch");
        Runtime stale_storm(42);check(stale_storm.start(70000,{197,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Fire Storm stale test start");
        ++target.generation;stale_storm.update(70000+activation,h);check(storms==1,"Fire Storm hit replaced NPC");
    }
    {
        Policy policy;policy.primary=5;policy.campaigns=0x1e;policy.can_change=true;Slots bar{};unsigned index=0;for(unsigned id:{39u,42u,59u,79u,80u}){policy.learned[id]=1;bar[index++]={id,0};} // Only one elite per tested bar.
        attributes::Ranks ranks{};ranks[2]=12;ranks[3]=12;vitals::Resources self{480,480,100,100},foe{480,480,40,40};effects::Scope target{1,43,5};unsigned applied=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):id==43?std::optional(foe):std::nullopt;};h.consume_energy=[&](unsigned n){if(n>self.energy)return false;self.energy-=n;return true;};h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned id,float range){return id==43&&range==1248;};h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?std::optional(target):std::nullopt;};h.energy_drain=[&](effects::Scope scope,const effects::EnergyDrainPlan& plan,uint64_t){check(scope==target&&effects::valid_energy_drain(plan),"Energy drain captured lifetime");++applied;return true;};
        Runtime runtime(42);uint64_t now=0;unsigned expected=100;for(unsigned id:{39u,42u,59u,79u,80u}){Slots valid_bar{};valid_bar[0]={id,0};const auto* row=find_skill(id);expected-=row->energy_cost();auto before=applied;check(runtime.start(now,{id,0,43},valid_bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==expected,"Energy family actualcost");auto ms=static_cast<uint64_t>(std::llround(row->activation*1000));runtime.update(now+ms-1,h);check(applied==before,"Energy drain nativeactivationwait");runtime.update(now+ms,h);check(applied==before+1,"Energy drain actualeffectcompletion");now+=ms+750;}
        check(applied==5&&self.energy==75,"Five energy denial bindings");Slots one{};one[0]={42,0};runtime.update(30000,h);check(runtime.start(30000,{42,0,43},one,policy,ranks,true,h)==casting::Reason::accepted,"Burn recharge ready");++target.generation;runtime.update(32000,h);check(applied==5,"Energy denial replaced target abort");
    }
    {
        Policy policy;policy.primary=3;policy.secondary=4;policy.campaigns=0x1e;
        policy.learned={{2,1},{281,1},{133,1}};Slots bar{};bar[0]={2,0};bar[1]={281,0};bar[2]={133,0};attributes::Ranks ranks{};
        vitals::Resources body{100,200,40,40},other_body{0,200,40,40};effects::Scope other{1,43,1};Hooks h;
        std::vector<Packet> events;
        h.events=[&](const auto& packets){events.insert(events.end(),packets.begin(),packets.end());};
        h.resources=[&](auto id)->std::optional<vitals::Resources>{return id==42?std::optional(body):id==43?std::optional(other_body):std::nullopt;};
        h.consume_energy=[](auto){return true;};h.target_valid=[](auto id,float,bool){return id==42;};
        h.target_scope=[&](auto id)->std::optional<effects::Scope>{return id==43?std::optional(other):std::nullopt;};
        h.resurrection_target_valid=[](auto id,float){return id==43;};h.resurrect=[&](auto,const auto&,auto){other_body.health=100;return true;};
        h.heal=[](auto,auto,float,bool){return true;};h.projectile_target_valid=[](auto id,float){return id==43;};
        h.shadow_damage=[](auto,const auto&,auto){return true;};
        h.health_sacrifice=[&](unsigned skill,auto){auto loss=sacrifice::resolve(skill,body.health,body.max_health);body.health=unsigned(loss->remaining);return loss;};
        Runtime cast(42);check(cast.start(100,{2,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"sacrifice signet fixture");cast.update(3100,h);
        check(cast.start(3200,{281,0,42},bar,policy,ranks,true,h)==casting::Reason::accepted,"sacrifice cooldown fixture");cast.update(4200,h);
        body.health=1;check(cast.start(5000,{133,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"lethal sacrifice start");cast.update(6000,h);
        check(body.health==0,"sacrifice was not lethal");cast.update(6001,h);body.health=100;
        check(cast.start(6100,{281,0,42},bar,policy,ranks,true,h)==casting::Reason::recharge,"sacrifice reset prior cooldown");
        check(cast.start(6100,{2,0,43},bar,policy,ranks,true,h)==casting::Reason::recharge,"sacrifice reset used signet");
        check(cast.start(6100,{133,0,43},bar,policy,ranks,true,h)==casting::Reason::recharge,"lethal successful skill omitted recharge");
        const auto recharge=slot_event(228,42,{133,0},find_skill(133)->recharge);
        check(std::count(events.begin(),events.end(),recharge)==1,"lethal successful skill did not publish exactly one recharge");
    }
    for(unsigned id:{118u,2885u}){
        Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{id,1}};Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};ranks[7]=12;
        vitals::Resources self{40,480,60,60},foe{480,480,20,20};effects::Scope target{1,43,5};unsigned applied=0,sacrificed=0;bool succeed=true;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(self):actor==43?std::optional(foe):std::nullopt;};h.consume_energy=[&](unsigned n){self.energy-=n;return true;};h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned actor,float range){return actor==43&&range==1248;};h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
        h.condition_spell=[&](effects::Scope scope,const effects::ConditionSpellPlan& plan,uint64_t){check(scope==target&&plan.radius==252&&plan.duration==(id==118?17000u:13000u),"Blood area reviewed rank");++applied;return succeed;};
        h.health_sacrifice=[&](unsigned skill,uint64_t)->std::optional<sacrifice::Loss>{check(applied==1&&skill==id,"Sacrifice follows effect commit");++sacrificed;auto loss=sacrifice::resolve(skill,self.health,self.max_health);self.health=static_cast<unsigned>(loss->remaining);return loss;};
        Runtime runtime(42);check(runtime.start(0,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Blood actual support");runtime.update(2000,h);check(applied==1&&sacrificed==1&&self.health==0,"Successful Blood can sacrifice caster to death");
        self.health=480;applied=0;sacrificed=0;succeed=false;Runtime failed(42);check(failed.start(0,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Blood failure test start");failed.update(2000,h);check(applied==1&&sacrificed==0&&self.health==480,"Failed effect never sacrifices");
    }
    for(unsigned id:{133u,102u,1067u}){
        Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{id,1}};Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};ranks[4]=12;
        vitals::Resources self{480,480,60,60},foe{480,480,40,40};effects::Scope target{1,43,5};unsigned applied=0,sacrificed=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(self):actor==43?std::optional(foe):std::nullopt;};h.consume_energy=[&](unsigned n){self.energy-=n;return true;};h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned actor,float range){return actor==43&&range==1248;};h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
        h.shadow_damage=[&](effects::Scope scope,const effects::ShadowPlan& plan,uint64_t){check(scope==target&&plan.damage==(id==133?40u:41u),"Shadow actual reviewed effect");++applied;return true;};
        h.health_sacrifice=[&](unsigned skill,uint64_t)->std::optional<sacrifice::Loss>{check(applied==1&&skill==133,"Dark Pact success precedes sacrifice");++sacrificed;return sacrifice::resolve(skill,self.health,self.max_health);};
        Runtime runtime(42);check(runtime.start(0,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Shadow spell actual support");runtime.update(2000,h);check(applied==1&&sacrificed==(id==133?1u:0u),"Shadow real effect and correct sacrifice eligibility");
    }
    for(unsigned id:{158u,1078u,131u}){
        Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{id,1}};Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};ranks[4]=12;
        vitals::Resources self{480,480,60,60},foe{480,480,40,40};effects::Scope target{1,43,5};unsigned applied=0,sacrificed=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(self):actor==43?std::optional(foe):std::nullopt;};h.consume_energy=[&](unsigned n){self.energy-=n;return true;};h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[&](unsigned actor,float range){return actor==43&&range==(id==131?1248:144);};h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
        h.shadow_damage=[&](effects::Scope scope,const effects::ShadowPlan& plan,uint64_t){check(scope==target&&plan.damage==50&&plan.range==144,"Shadow touch actual range and damage");++applied;return true;};
        h.condition_spell=[&](effects::Scope scope,const effects::ConditionSpellPlan& plan,uint64_t){check(scope==target&&plan.radius==166&&plan.duration==13000&&plan.kind==fw::gw::combat::conditions::Kind::bleeding,"Barbed actual area Bleeding");++applied;return true;};
        h.health_sacrifice=[&](unsigned skill,uint64_t)->std::optional<sacrifice::Loss>{check(applied==1&&skill==id,"Touch/signet effect before sacrifice");++sacrificed;return sacrifice::resolve(skill,self.health,self.max_health);};
        Runtime runtime(42);check(runtime.start(0,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==(id==131?60u:59u),"Touch/signet actual energy");runtime.update(1000,h);check(applied==1&&sacrificed==1,"Touch/signet real completion and cost");
    }
    for(unsigned id:{279u,1117u,280u,1119u,293u}){
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{id,1}};Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};ranks[13]=7;ranks[16]=12;
        vitals::Resources self{100,480,60,60};unsigned applied=0;Hooks h;h.events=[](const auto&){};h.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(self):std::nullopt;};h.target_valid=[](unsigned actor,float,bool){return actor==42;};h.consume_energy=[&](unsigned n){self.energy-=n;return true;};
        h.party_heal=[&](const effects::PartyHealPlan& plan,uint64_t){check(plan.base_heal==(id==279||id==1117?51u:100u)&&plan.caster_divine_heal==38,"Area heals use correct attribute distinct from Healing Prayers");++applied;return true;};h.heal=[&](unsigned actor,unsigned amount,float,bool){check(actor==42&&amount==83,"Signet uses Divine Favor rank12 without passive spell bonus");++applied;return true;};
        Runtime runtime(42);check(runtime.start(0,{id,0,id==293?42u:999u},bar,policy,ranks,true,h)==casting::Reason::accepted,"Area/self/signet actual support");runtime.update(2000,h);check(applied==1,"Actual area/signet completion");
    }
    for(unsigned id:{1691u,311u,295u}){
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{id,1}};Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};ranks[15]=12;ranks[16]=12;
        vitals::Resources self{100,480,60,60},ally{100,480,40,40};effects::Scope target{1,43,5};unsigned applied=0;Hooks h;h.events=[](const auto&){};h.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(self):actor==43?std::optional(ally):std::nullopt;};h.consume_energy=[&](unsigned n){self.energy-=n;return true;};h.target_valid=[&](unsigned actor,float range,bool allow){return actor==43&&range==1248&&allow==(id!=311);};h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
        h.remove_conditions=[&](unsigned actor,const effects::ConditionRemovalPlan& plan,uint64_t){check(actor==43&&plan.per_condition_heal==63&&plan.healing==effects::HealBasis::enchanted,"Dismiss actual conditional plan");++applied;return true;};
        h.ally_cleansing=[&](effects::Scope scope,const effects::AllyCleansingPlan& plan,uint64_t){check(scope==target&&effects::valid_ally_cleansing(plan),"Cleansing captured ally scope");check(id==311?plan.health_gain==22&&plan.divine_heal==38:plan.divine_heal==0,"Draw gain versus signet no DF");++applied;return true;};
        const auto complete=static_cast<uint64_t>(std::llround(find_skill(id)->activation*1000));Runtime runtime(42);check(runtime.start(0,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Cleansing actual binding");runtime.update(complete-1,h);check(applied==0,"Cleansing waits actual native activation");runtime.update(complete,h);check(applied==1,"Cleansing actual completion");
        applied=0;Runtime stale(42);check(stale.start(0,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Cleansing stale fixture start");++target.generation;stale.update(complete,h);check(applied==0,"Cleansing replaced ally rejected");
    }
    for(unsigned id:{44u,1052u,3184u}){
        Policy policy;policy.primary=5;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{id,1}};Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};ranks[1]=12;
        vitals::Resources self{480,480,60,60},foe{480,480,40,40};effects::Scope target{1,43,5};unsigned applied=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(self):actor==43?std::optional(foe):std::nullopt;};h.consume_energy=[&](unsigned n){self.energy-=n;return true;};h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned actor,float range){return actor==43&&range==1248;};h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
        h.wound_spell=[&](effects::Scope scope,const effects::WoundSpellPlan& plan,uint64_t){check(scope==target&&effects::valid_wound_spell(plan)&&plan.rank==12,"Wound spell captured target and rank");++applied;return true;};
        Runtime runtime(42);check(runtime.start(0,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==55,"Wound spells cost five energy");runtime.update(1999,h);check(applied==0,"Wound spell two-second activation");runtime.update(2000,h);check(applied==1,"Wound actual effect dispatch");
        check(runtime.start(3000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::recharge,"Native wound recharge");runtime.update(20000,h);check(runtime.start(20000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Wound actual recharge ready");++target.generation;runtime.update(22000,h);check(applied==1,"Wound stale foe aborts");
    }
    for(unsigned id:{884u,2192u}){
        Policy policy;policy.primary=6;policy.campaigns=0x1e;policy.learned={{id,1}};Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};ranks[find_skill(id)->attribute]=12;ranks[12]=13;
        vitals::Resources self{480,480,60,60},foe{480,480,40,40};effects::Scope target{1,43,5};unsigned applied=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(self):actor==43?std::optional(foe):std::nullopt;};h.consume_energy=[&](unsigned n){self.energy-=n;return true;};h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned actor,float range){return actor==43&&range==1248;};h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
        h.elemental=[&](effects::Scope scope,const effects::ElementalPlan& plan,uint64_t){check(scope==target&&effects::valid_elemental(plan)&&plan.rank==12&&plan.storage==13,"conditional elemental captured rank/Storage/target");++applied;return true;};
        Runtime runtime(42);check(runtime.start(0,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==60-find_skill(id)->energy_cost(),"conditional elemental native energy");runtime.update(999,h);check(applied==0,"conditional elemental native activation");runtime.update(1000,h);check(applied==1,"conditional elemental actual hook once");
        check(runtime.start(2000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::recharge,"conditional elemental native recharge");Runtime cancelled(42);check(cancelled.start(20000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"conditional elemental cancellation start");cancelled.cancel(h);cancelled.update(21000,h);check(applied==1,"cancelled elemental never commits or emits particles");Runtime stale(42);check(stale.start(30000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"conditional elemental stale start");++target.generation;stale.update(31000,h);check(applied==1,"conditional elemental recycled target rejects");
    }
    for(unsigned id:{828u,864u}){
        Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{id,1}};Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};ranks[find_skill(id)->attribute]=12;
        vitals::Resources self{480,480,60,60},foe{480,480,40,40};effects::Scope target{1,43,5};unsigned applied=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(self):actor==43?std::optional(foe):std::nullopt;};h.consume_energy=[&](unsigned n){self.energy-=n;return true;};h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned actor,float range){return actor==43&&range==1248;};h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
        h.condition_damage=[&](effects::Scope scope,const effects::ConditionDamagePlan& plan,uint64_t){check(scope==target&&effects::valid_condition_damage(plan)&&plan.rank==12,"condition damage captured rank and target lifetime");++applied;return true;};
        Runtime runtime(42);check(runtime.start(0,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==50,"condition damage native10 energy");runtime.update(999,h);check(applied==0,"condition damage one-second activation");runtime.update(1000,h);check(applied==1,"condition damage actual world hook once");
        check(runtime.start(2000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::recharge,"condition damage native recharge");Runtime cancelled(42);check(cancelled.start(20000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"condition damage cancellation start");cancelled.cancel(h);cancelled.update(21000,h);check(applied==1,"condition damage cancellation prevents effect");Runtime stale(42);check(stale.start(30000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"condition damage stale start");++target.generation;stale.update(31000,h);check(applied==1,"condition damage rejects recycled target");
    }
    for(unsigned id:{31u,859u,108u,2054u,109u,126u}){
        Policy policy;policy.primary=(id==108||id==109||id==126)?4:5;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{id,1}};Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};ranks[1]=12;ranks[7]=12;ranks[4]=12;
        vitals::Resources self{480,480,60,60},foe{480,480,40,40};effects::Scope target{1,43,5};unsigned applied=0;Hooks h;h.events=[](const auto&){};
        h.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(self):actor==43?std::optional(foe):std::nullopt;};h.consume_energy=[&](unsigned n){self.energy-=n;return true;};h.target_valid=[](auto,float,bool){return false;};h.projectile_target_valid=[](unsigned actor,float range){return actor==43&&range==1248;};h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
        h.degeneration_hex=[&](effects::Scope scope,const effects::DegenerationHexPlan& plan,uint64_t){check(scope==target&&effects::valid_degeneration_hex(plan)&&plan.rank==12,"Degeneration captured lifetime and rank");++applied;return true;};
        Runtime runtime(42);check(runtime.start(0,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==60-find_skill(id)->energy_cost(),"Degeneration actual native energy cost");runtime.update(999,h);check(applied==0,"Degeneration one-second activation");runtime.update(1000,h);check(applied==1,"Degeneration actual world hook");
        check(runtime.start(2000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::recharge,"Degeneration native recharge enforced");Runtime cancelled(42);check(cancelled.start(20000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Degeneration cancel fixture");cancelled.cancel(h);cancelled.update(21000,h);check(applied==1,"cancelled hex never commits");Runtime stale(42);check(stale.start(30000,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Degeneration stale fixture");++target.generation;stale.update(31000,h);check(applied==1,"recycled target hex rejected");
    }
    for(const unsigned melee_id:{322u,326u,1693u}){Policy melee_policy;melee_policy.primary=1;melee_policy.campaigns=0x1e;melee_policy.learned={{melee_id,1}};Slots melee_bar{};melee_bar[0]={melee_id,0};attributes::Ranks ranks{};ranks[17]=12;
     Runtime attack(42);Hooks mh;unsigned energy_cost=0,hits=0;bool target_present=true,caster_alive=true;const effects::Scope target_life{1,43,1};
     std::optional<Milliseconds> activity;mh.attack_activity=[&](auto until){activity=until;};mh.events=[](const auto&){};mh.consume_energy=[&](unsigned n){energy_cost+=n;return true;};mh.resources=[&](unsigned id){return std::optional<vitals::Resources>{{id==42&&!caster_alive?0u:480u,480,20,20}};};
     mh.weapon_snapshot=[](){return std::optional<combat::weapon::Owned>{{1,4,{15,22,2},12,false,false,27}};};mh.melee_target=[&](unsigned t,Milliseconds){return t==43&&target_present;};mh.target_scope=[&](unsigned t){return t==43?std::optional{target_life}:std::nullopt;};
    mh.weapon_attack_timing=[](auto){return std::optional<combat::weapon::Timing>{{500,1330,1200}};};
     mh.melee_attack=[&](auto scope,const auto& weapon,const auto& plan,Milliseconds){check(scope==target_life&&weapon.revision==4&&plan.bonus==(melee_id==1693?29u:34u),"captured melee weapon/target and effective strength");++hits;return true;};
     const auto melee_weapon=mh.weapon_snapshot;
     mh.weapon_snapshot=[](){return std::optional<combat::weapon::Owned>{{1,4,{15,22,2},12,false,true,26}};};
     Runtime wrong_weapon(42);check(wrong_weapon.start(0,{melee_id,0,43},melee_bar,melee_policy,ranks,true,mh)==casting::Reason::weapon&&!energy_cost&&!activity&&!hits,"caster weapon rejects melee without spending energy or beginning attack");
     mh.weapon_snapshot=melee_weapon;
     check(attack.start(100,{melee_id,0,43},melee_bar,melee_policy,ranks,true,mh)==casting::Reason::accepted,"PowerAttack owned melee begin");check(energy_cost==5&&activity==600,"melee actual energy and authoritative windup");attack.update(599,mh);check(!hits,"PowerAttack waits reviewed private windup");attack.update(600,mh);check(hits==1&&!activity,"melee hits once and clears windup");check(attack.start(700,{melee_id,0,43},melee_bar,melee_policy,ranks,true,mh)==casting::Reason::recharge,"PowerAttack genuine recharge");
     std::uint64_t interrupt_epoch=0;mh.attack_interrupt_epoch=[&]()->std::optional<std::uint64_t>{return interrupt_epoch;};
     Runtime interrupted_attack(42);check(interrupted_attack.start(5000,{melee_id,0,43},melee_bar,melee_policy,ranks,true,mh)==casting::Reason::accepted,"attack interruption fixture starts");
     ++interrupt_epoch;interrupted_attack.update(5500,mh);check(!interrupted_attack.active()&&!activity&&hits==1,"interrupt epoch cancels attack before completion and prevents hit");
     auto ordinary_activity=mh.attack_activity;
     mh.attack_activity=[&](auto until){activity=until;if(until)++interrupt_epoch;};
     Runtime reactive_attack(42);check(reactive_attack.start(6000,{melee_id,0,43},melee_bar,melee_policy,ranks,true,mh)==casting::Reason::accepted,"synchronous attack reaction admission failed");
     reactive_attack.update(6001,mh);check(!reactive_attack.active()&&!activity&&hits==1,"synchronous attack interruption was sampled after its own reaction");
     mh.attack_activity=ordinary_activity;
     Runtime cancelled_attack(42);check(cancelled_attack.start(10000,{melee_id,0,43},melee_bar,melee_policy,ranks,true,mh)==casting::Reason::accepted,"melee cancellation begin");cancelled_attack.cancel(mh);check(!activity,"cancel clears attacking state");cancelled_attack.update(10500,mh);check(hits==1,"cancelled melee never hits");
     Runtime lost_target(42);check(lost_target.start(20000,{melee_id,0,43},melee_bar,melee_policy,ranks,true,mh)==casting::Reason::accepted,"melee target departure begin");target_present=false;lost_target.update(20500,mh);check(hits==1,"departed melee target never hits");target_present=true;Runtime dead_attack(42);check(dead_attack.start(30000,{melee_id,0,43},melee_bar,melee_policy,ranks,true,mh)==casting::Reason::accepted,"melee death fixture");caster_alive=false;dead_attack.update(30100,mh);check(!activity&&!dead_attack.active(),"death clears pending attack and world windup");caster_alive=true;dead_attack.update(30500,mh);check(hits==1,"resurrection cannot revive cancelled pending strike");
    }
    {
        Policy policy;policy.primary=6;policy.campaigns=0x1e;policy.learned={{189,1}};
        Slots bar{};bar[0]={189,0};attributes::Ranks ranks{};ranks[10]=12;
        vitals::Resources self{480,480,60,60},foe{480,480,20,20};effects::Scope target{1,43,5};
        Hooks h;unsigned applied=0;h.events=[](const auto&){};
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):id==43?std::optional(foe):std::nullopt;};
        h.consume_energy=[&](unsigned amount){self.energy-=amount;return true;};
        h.projectile_target_valid=[&](unsigned id,float range){return id==43&&range==1248&&foe.health>0;};
        h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?std::optional(target):std::nullopt;};
        h.elemental=[&](effects::Scope scope,const effects::ElementalPlan& plan,uint64_t){check(scope==target&&plan.skill==189,"Rodgort retains original target lifetime");++applied;return true;};
        Runtime cast(42);check(cast.start(0,{189,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&self.energy==35,"Rodgort starts on live foe for25 energy");
        foe.health=0;cast.update(1999,h);check(applied==0,"Rodgort still waits two-second activation");
        cast.update(2000,h);check(applied==1,"Rodgort completes after target death");
        Runtime recycled(42);foe.health=480;check(recycled.start(10000,{189,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"Rodgort replacement fixture begins");
        foe.health=0;++target.generation;recycled.update(12000,h);check(applied==1,"Rodgort cannot target a recycled corpse identity");
    }
    {
        Policy policy;policy.primary=6;policy.secondary=3;policy.campaigns=0x1e;policy.learned={{200,1},{281,1}};
        Slots slots{};slots[0]={200,0};slots[1]={281,0};attributes::Ranks ranks{};
        vitals::Resources resource{100,480,20,20};unsigned cost=999;Hooks h;
        h.events=[](const auto&){};h.target_valid=[](unsigned,float,bool){return true;};
        h.resources=[&](unsigned)->std::optional<vitals::Resources>{return resource;};
        h.consume_energy=[&](unsigned value){cost=value;if(value>resource.energy)return false;resource.energy-=value;return true;};
        h.heal=[](unsigned,unsigned,float,bool){return true;};
        Runtime cast(42);
        check(cast.start(0,{200,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted&&cost==5,"glyph pays its own cost");
        cast.update(1000,h);
        check(cast.start(2000,{281,0,42},slots,policy,ranks,true,h)==casting::Reason::accepted&&cost==0,"glyph first spell discounted");
        cast.cancel(h);
        check(cast.start(2001,{281,0,42},slots,policy,ranks,true,h)==casting::Reason::accepted&&cost==0,"cancelled spell consumes first glyph charge");
        cast.cancel(h);
        check(cast.start(2002,{281,0,42},slots,policy,ranks,true,h)==casting::Reason::accepted&&cost==5,"two cancelled spells exhaust glyph");
        cast.cancel(h);
        check(cast.start(32000,{200,0,42},slots,policy,ranks,true,h)==casting::Reason::accepted,"glyph renews after recharge");cast.update(33000,h);
        check(cast.start(48000,{281,0,42},slots,policy,ranks,true,h)==casting::Reason::accepted&&cost==5,"glyph expires at fifteen seconds even before update");
    }
    {
        Policy policy;policy.primary=6;policy.secondary=3;policy.campaigns=0x1e;policy.learned={{180,1},{200,1},{281,1}};
        Slots slots{};slots[0]={180,0};slots[1]={200,0};slots[2]={281,0};attributes::Ranks ranks{};ranks[12]=8;
        vitals::Resources resource{100,480,50,50};unsigned aura_heal=0,energy_return=0,cost=999;
        effects::Scope scope{1,42,1};effects::Registry registry(scope);Hooks h;
        h.events=[](const auto&){};h.target_valid=[](unsigned,float,bool){return true;};
        h.resources=[&](unsigned)->std::optional<vitals::Resources>{return resource;};
        h.consume_energy=[&](unsigned value){cost=value;if(value>resource.energy)return false;resource.energy-=value;return true;};
        h.heal=[&](unsigned actor,unsigned amount,float,bool){if(actor==42)aura_heal+=amount;return true;};
        h.restore_energy=[&](unsigned value){energy_return+=value;return true;};
        h.status_mutation=[&](unsigned skill,unsigned id,unsigned rank,auto now,auto expires,bool active){
            if(active)return registry.apply(scope,{id,42,skill,rank,effects::category(*find_skill(skill)),now,expires},effects::Replace::same_skill).accepted;
            registry.remove(scope,id);return true;
        };
        h.effect_snapshot=[&](auto now)->std::optional<std::vector<effects::Entry>>{return registry.active(scope,now);};
        Runtime cast(42);
        check(cast.start(0,{180,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"Aura self-targets and starts");cast.update(250,h);
        check(aura_heal==0&&energy_return==0&&registry.has(scope,effects::Category::enchantment,250),"first Aura registers without triggering itself");
        check(cast.start(1000,{200,0,42},slots,policy,ranks,true,h)==casting::Reason::accepted,"glyph under Aura starts");cast.update(2000,h);
        check(aura_heal==0&&energy_return==0,"glyph is not a spell and does not trigger Aura");
        check(cast.start(3000,{281,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted&&cost==0,"glyph discounts spell under Aura");cast.update(4000,h);
        check(aura_heal==18&&energy_return==1,"Aura uses original five energy cost at rank8 despite glyph discount");
        check(cast.start(6000,{281,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"second discounted spell starts");cast.cancel(h);cast.update(7000,h);
        check(aura_heal==18&&energy_return==1,"cancelled spell does not trigger Aura");
        check(cast.start(22000,{180,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"Aura renewal starts");cast.update(22250,h);
        check(aura_heal==36&&energy_return==2&&registry.count(scope,effects::Category::enchantment,22250)==1,"renewal triggers existing Aura once and replaces its effect");
        auto active=registry.active(scope,23000);for(const auto& entry:active)if(entry.skill==180)registry.remove(scope,entry.instance);
        cast.update(23000,h);
        check(cast.start(24000,{281,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"spell after Aura strip starts");cast.update(25000,h);
        check(aura_heal==36&&energy_return==2,"removed Aura cannot heal or return energy");
        check(cast.start(43000,{180,0,42},slots,policy,ranks,true,h)==casting::Reason::accepted,"Aura starts after prior strip");cast.update(43250,h);
        check(cast.start(103250,{281,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted,"spell starts at Aura expiry");cast.update(104250,h);
        check(aura_heal==36&&energy_return==2,"expired Aura cannot trigger on completed spell");
        Runtime denied(42);auto missing=h;missing.restore_energy={};
        check(denied.start(0,{180,0,42},slots,policy,ranks,true,missing)==casting::Reason::unsupported,"Aura needs authoritative energy return hook");
        for(unsigned rank=0;rank<=20;++rank){auto plan=effects::aura_restoration(*find_skill(281),rank);check(plan&&plan->healing==10+rank&&plan->energy==(rank>=8?1u:0u),"Aura wiki progression across all supported ranks");}
        for(unsigned rank=0;rank<=20;++rank){
            const auto plan=effects::aura_restoration(*find_skill(281),rank,true);
            const unsigned expected[]={10,11,11,12,13,13,14,15,15,16,17,17,18,19,19,20,21,21,22,23,23};
            check(plan&&plan->healing==expected[rank]&&plan->energy==(rank>=8?1u:0u),"Aura PvP final-heal rounding across all ranks");
        }
        auto cheap=*find_skill(281);cheap.encoded_energy=1;
        check(effects::aura_restoration(cheap,8)->healing==4,"Aura rounds fractional one-energy heal per wiki table");
        for(unsigned type:{4u,5u,6u,9u,11u,24u,25u}){cheap.type=type;check(effects::is_spell(cheap),"all wiki spell subtypes recognized");}
        check(!effects::aura_restoration(*find_skill(200),8)&&!effects::aura_restoration(*find_skill(281),21),"Aura excludes nonspells and invalid rank");
    }
    {
        Policy policy;policy.primary=1;policy.campaigns=0x1e;policy.learned={{1,1}};Slots slots{};slots[0]={1,0};attributes::Ranks ranks{};ranks[21]=12;
        vitals::Resources resource{100,480,0,20};combat::conditions::State state;unsigned heals=0,amount=0,cost=99;
        Hooks h;h.events=[](const auto&){};h.resources=[&](unsigned)->std::optional<vitals::Resources>{return resource;};
        h.target_valid=[](unsigned target,float range,bool self){return target==42&&range==0&&self;};
        h.consume_energy=[&](unsigned value){cost=value;return value==0;};
        h.heal=[&](unsigned target,unsigned value,float range,bool self){check(target==42&&range==0&&self,"signet healing is self only");++heals;amount=value;return true;};
        h.healing_signet_activity=[&](auto now,auto until){return state.healing_signet(now,until);};
        Runtime cast(42);check(cast.start(100,{1,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted&&cost==0,"Healing Signet starts at zero energy and ignores selected foe");
        check(state.armor(80,100)==40&&heals==0,"armor penalty begins before heal");cast.update(2099,h);check(heals==0,"signet respects two-second activation");cast.update(2100,h);
        check(heals==1&&amount==154&&state.armor(80,2100)==80,"rank12 healing154 and casting penalty ends at completion");
        check(cast.start(3000,{1,0,42},slots,policy,ranks,true,h)==casting::Reason::recharge,"signet recharge respected");
        check(cast.start(6100,{1,0,42},slots,policy,ranks,true,h)==casting::Reason::accepted,"signet after recharge");cast.cancel(h);check(state.armor(80,6100)==80&&heals==1,"cancel restores armor without healing");
        check(cast.start(6200,{1,0,42},slots,policy,ranks,true,h)==casting::Reason::accepted,"signet restart after cancellation");resource.health=0;cast.update(6300,h);check(!cast.active()&&state.armor(80,6300)==80&&heals==1,"death cancels signet and clears penalty");
        resource.health=100;auto unavailable=h;unavailable.healing_signet_activity={};Runtime denied(42);
        check(denied.start(0,{1,0,42},slots,policy,ranks,true,unavailable)==casting::Reason::unsupported,"no healing-only success without armor hook");
    }
    for(unsigned id:{153u,156u}){
        Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.learned={{id,1}};
        Slots slots{};slots[0]={id,0};attributes::Ranks ranks{};unsigned applied=0;
        effects::Scope npc{1,43,7};Hooks hooks;hooks.events=[](const auto&){};
        hooks.resources=[](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional<vitals::Resources>{{100,100,100,100}}:std::nullopt;};
        hooks.consume_energy=[](unsigned){return true;};
        hooks.projectile_target_valid=[](unsigned actor,float range){return actor==43&&(range==144||range==1248);};
        hooks.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(npc):std::nullopt;};
        hooks.life_steal=[&](effects::Scope target,const effects::LifeStealPlan& plan,uint64_t){check(target==npc&&plan.skill==id,"NPC life steal application target");++applied;return true;};
        Runtime runtime(42);check(runtime.start(0,{id,0,43},slots,policy,ranks,true,hooks)==casting::Reason::accepted,"NPC life steal start");
        runtime.update(10000,hooks);check(applied==1,"NPC without player Resources cancelled before life steal application");
        Runtime stale(42);check(stale.start(20000,{id,0,43},slots,policy,ranks,true,hooks)==casting::Reason::accepted,"NPC lifetime test start");
        ++npc.generation;stale.update(30000,hooks);check(applied==1,"recycled NPC received old life steal cast");
    }
    std::cout<<"Direct heals, native timed Earth attunement refund, Dash lifecycle/resource/recharge tests passed.\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<"\n";return 1;}





#include "gw_npc_owned_ordinary_descriptor.h"
#include <iostream>
#include <source_location>
using namespace fw::gw::combat::ai::owned_ordinary;
void check(bool b,std::source_location at=std::source_location::current()){
    if(!b)throw std::runtime_error("Owned ordinary descriptor line "+std::to_string(at.line()));
}
struct Fixture{
    Descriptor d;Objective objective;std::vector<Member> members;
    Fixture(unsigned quest):d(*captured_tads(quest)){
        objective.owner={1,1,1};objective.quest=quest;objective.cycle=7;objective.map=146;objective.kind=d.kind;
        for(unsigned i=0;i<d.members;++i){Member m;m.source={1,60000+i,1};m.assigned_owner=objective.owner;
            m.assigned_quest=quest;m.assigned_cycle=7;m.map=146;m.actor=m.definition=m.source.actor;
            m.model=82023;m.flags=8;m.level=0;m.maximum_health=10;m.current_health=10;m.hostile=true;
            for(auto& a:m.armor)a=0;
            objective.assigned.insert(m.source);objective.remaining.insert(m.source);members.push_back(m);}
    }
    Admission admit(){return validate(d,objective,objective.owner,7,members,1000);}
};
int main()try{
    check(!captured_tads(83));check(!captured_tads(75));
    for(unsigned quest:{81u,85u}){
        Fixture f(quest);check(bool(f.admit()));check(f.admit().members==f.d.members);
        check(f.d.level==0&&f.d.maximum_health==10&&f.d.profile.damage==5&&f.d.profile.windup==750&&f.d.profile.period==2000&&f.d.profile.reach==128);
        auto stale=f.objective.owner;++stale.generation;
        check(validate(f.d,f.objective,stale,7,f.members,1000).refusal==Refusal::owner);
        check(validate(f.d,f.objective,f.objective.owner,8,f.members,1000).refusal==Refusal::objective);
        f.members[0].current_health=9;check(bool(f.admit())); // Existing damage is never reset.
        f.members[0].current_health=0;check(f.admit().refusal==Refusal::dead);f.members[0].current_health=9;
        f.members[0].health_clock=1001;check(f.admit().refusal==Refusal::future);f.members[0].health_clock=1000;
        f.members[0].maximum_health=35;check(f.admit().refusal==Refusal::unknown_combat);f.members[0].maximum_health=10;
        for(unsigned i=0;i<6;++i){f.members[0].armor[i].reset();check(f.admit().refusal==Refusal::unknown_combat);f.members[0].armor[i]=0;}
        auto saved=f.members[0];++f.members[0].source.generation;check(f.admit().refusal==Refusal::roster);f.members[0]=saved;
        ++f.members[0].assigned_cycle;check(f.admit().refusal==Refusal::identity);f.members[0]=saved;
        ++f.members[0].model;check(f.admit().refusal==Refusal::identity);f.members[0]=saved;
        f.members[1]=f.members[0];check(f.admit().refusal==Refusal::roster);f=Fixture(quest);
        f.objective.kind=ObjectiveKind::loot;check(f.admit().refusal==Refusal::objective);f=Fixture(quest);
        auto& m=f.members[0];m.controller_present=m.controller_owned=true;m.controller_source=m.source;m.controller_owner=f.objective.owner;
        m.controller_quest=quest;m.controller_cycle=7;m.installed_profile=f.d.profile;check(bool(f.admit()));
        for(unsigned variant=0;variant<15;++variant){auto& p=*m.installed_profile;p=f.d.profile;
            switch(variant){case 0:++p.damage;break;case 1:++p.mastery;break;case 2:++p.damage_type;break;
                case 3:++p.windup;break;case 4:++p.period;break;case 5:++p.reach;break;case 6:++p.speed;break;
                case 7:++p.acquire;break;case 8:++p.follow;break;case 9:++p.leash;break;case 10:++p.patrol_radius;break;
                case 11:++p.pause;break;case 12:++p.leg_timeout;break;case 13:p.evidence+=" changed";break;
                case 14:m.controller_source.generation++;break;}
            check(f.admit().refusal==Refusal::controller);
        }
        f=Fixture(quest);f.objective.remaining.erase(f.members.back().source);f.members.pop_back();
        check(bool(f.admit())==(quest==85)); // Kill-any may not silently become kill-all.
        f.objective.remaining.clear();f.members.clear();check(f.admit().refusal==Refusal::roster);
    }
    std::cout<<"Owned ordinary descriptor exact quest/scopes/profile, Health10, objective-kind and refusal guards passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

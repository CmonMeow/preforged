#include "gw_npc_skill_ai.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace ai=combat::ai;
namespace control=combat::npcs::control;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("NPC spellbook authority line "+std::to_string(at.line()));
}
ai::Spellbook book(){return ai::Spellbook({{{186,0},1248,"Synthetic Fireball lifecycle fixture"},
    {{194,0},1248,"Synthetic Flare alternative fixture"}});}
control::Token token(std::uint64_t sequence=1,std::uint64_t begins=1000,unsigned skill=186){
    return {{1,100,1},0,sequence,control::Action::spell,begins,begins+2000,skills::Slot{skill,0}};
}
int main()try{
    for(unsigned skill:{181u,2860u}){
        const skills::effects::Scope owner{1,100,1};const skills::Slot slot{static_cast<std::uint16_t>(skill),0};
        ai::Spellbook b({{slot,0,"authored Renewal self authority",ai::SpellTargetClass::self}},owner);
        ai::SpellTarget self{owner,0,true,false,true,true};
        check(b.eligible(slot,self,10,1000)&&!b.eligible(slot,self,9,1000));
        for(unsigned fault=0;fault<4;++fault){auto other=self;
            if(fault==0)other.scope.actor=101;
            if(fault==1)other.scope.generation++;
            if(fault==2)other.hostile=true;
            if(fault==3)other.distance=1;
            check(!b.eligible(slot,other,10,1000));
        }
        for(unsigned fault=0;fault<3;++fault){bool rejected=false;
            try{ai::Spellbook invalid({{slot,fault==1?1.f:0.f,"authored invalid Renewal",
                fault==0?ai::SpellTargetClass::reviewed:ai::SpellTargetClass::self}},
                fault==2?std::nullopt:std::optional(owner));}
            catch(const std::invalid_argument&){rejected=true;}
            check(rejected);
        }
    }
    {const skills::effects::Scope owner{1,100,1};
        ai::Spellbook b({{{186,0},1248,"Synthetic owner authority fixture"}},owner);
        ai::SpellTarget target{{1,101,1},100,true,true,true,true};
        check(b.choose(target,100,1000)==skills::Slot{186,0});
        target.scope.instance=2;
        check(!b.eligible({186,0},target,100,1000)&&!b.choose(target,100,1000));
        target.scope=owner;target.distance=0;
        check(!b.eligible({186,0},target,100,1000)&&!b.choose(target,100,1000));
        target.scope={1,101,1};
        check(b.choose(target,100,1000)==skills::Slot{186,0});
        // Owner inference after the first admitted token has the same boundary.
        auto inferred=book();check(inferred.begin(token()));inferred.finish(1000,false,false);
        target.scope.instance=2;check(!inferred.choose(target,100,1000));
    }
    // Invalid inputs must leave the book available for the original valid token.
    for(unsigned fault=0;fault<13;++fault){auto b=book();auto invalid=token();
        switch(fault){
        case 0:invalid.actor.instance=0;break;
        case 1:invalid.actor.actor=0;break;
        case 2:invalid.actor.actor=65536;break;
        case 3:invalid.actor.generation=0;break;
        case 4:invalid.sequence=0;break;
        case 5:invalid.epoch=UINT64_MAX;break;
        case 6:invalid.completes=invalid.begins;break;
        case 7:invalid.completes=invalid.begins-1;break;
        case 8:invalid.completes=invalid.begins+60001;break;
        case 9:invalid.kind=control::Action::attack;break;
        case 10:invalid.skill.reset();break;
        case 11:invalid.skill->copy=1;break;
        case 12:invalid.skill->skill=28;break;
        }
        check(!b.begin(invalid)&&!b.pending()&&b.action_ready(1000));check(b.begin(token()));
    }
    {auto b=book();const auto first=token();check(b.begin(first));
        check(!b.begin(token(2))&&b.tracks(first));
        b.finish(1000,false,false); // voluntary cancellation: no base recharge
        check(!b.begin(first)&&b.action_ready(1000));
        auto replacement=token(2);replacement.actor.generation++;
        check(!b.begin(replacement));replacement=token(2);replacement.actor.instance++;
        check(!b.begin(replacement));replacement=token(2);replacement.actor.actor++;
        check(!b.begin(replacement)&&b.begin(token(2)));
    }
    {auto b=book();check(b.begin(token()));b.finish(3000,true,true);
        const auto deadline=*b.recharge_until({186,0},3000);
        check(deadline>3750);
        // A different ready spell is still blocked by shared aftercast.
        check(!b.begin(token(2,3749,194))&&b.action_ready(3750));
        check(!b.begin(token(2,deadline-1))&&b.begin(token(2,3750,194)));
        b.finish(3750,false,false); // voluntarily cancel the alternate spell
        check(!b.begin(token(3,deadline-1))&&b.begin(token(3,deadline)));
    }
    {auto b=book();check(b.begin(token()));b.finish(1500,true,false); // prevention
        const auto deadline=*b.recharge_until({186,0},1500);
        check(b.action_ready(1500)&&!b.begin(token(2,deadline-1)));
        check(b.begin(token(2,1500,194)));b.finish(1500,false,false);
        check(b.begin(token(3,deadline)));
    }
    std::cout<<"NPC spellbook admission authority passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

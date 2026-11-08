#include "gw_party_state.h"
#include "gw_party_limits_generated.h"
#include "gw_world_simulation.h"
#include <iostream>
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main()try{
    using fw::gw::party::State;
    check(fw::gw::map_party_limit_2022(148)==2&&fw::gw::map_party_limit_2022(55)==6&&
          fw::gw::map_party_limit_2022(248)==8&&fw::gw::map_party_limit_2022(65535)==0,"2022 map party limits");
    for(auto map:{148u,55u,248u}){
        const auto limit=fw::gw::map_party_limit_2022(map);State town(limit);
        for(unsigned id=1;id<=limit+1;++id)check(town.add(id),"town party setup");
        for(unsigned id=2;id<=limit;++id)
            check(town.invite(1,id,0)&&town.accept(id,town.group(1),1),"town party fills to map limit");
        check(!town.invite(1,limit+1,2)&&town.members(town.group(1)).size()==limit,"town rejects oversized party");
    }
    State state(4);for(unsigned id=1;id<=5;++id)check(state.add(id),"join");
    check(!state.add(0)&&!state.add(1),"duplicate player");
    const auto first=state.group(1);
    check(!state.accept(2,first,0),"unsolicited accept");
    check(state.invite(1,2,0)&&!state.invite(2,1,0),"reciprocal invitation cannot double-link client party");
    check(state.invite(1,2,0)&&state.accept(2,first,1),"invite and accept");
    check(state.members(first)==std::vector<State::Id>{1,2},"leader order");
    check(!state.invite(2,3,2)&&!state.kick(2,1),"nonleader authority");
    check(state.invite(1,3,2)&&state.reject(3,first)&&!state.accept(3,first,3),"reject revokes consent");
    check(state.invite(1,3,4)&&state.cancel(1,state.group(3))&&!state.accept(3,first,5),"cancel revokes consent");
    check(state.invite(1,3,10)&&state.invite(1,3,60009)&&!state.accept(3,first,60010),"expiry and duplicate invite");
    check(state.invite(3,4,70000)&&state.accept(4,state.group(3),70001),"second party");
    check(state.invite(1,3,70002)&&state.accept(3,first,70003),"party merge");
    check(state.members(first).size()==4&&!state.invite(1,5,70004),"map capacity");
    check(state.kick(1,4)&&state.leader(4)&&state.members(first).size()==3,"kick creates solo");
    check(state.invite(1,5,70005)&&state.remove(1)&&state.leader(2),"leader departure promotes next");
    check(!state.accept(5,first,70006),"leader departure invalidates invitations");
    check(state.add(1)&&state.group(1)!=first,"reused player cannot steal occupied party ID");
    check(state.leave(2)&&state.leader(3)&&state.leader(2),"leader leaves into own party");
    check(!state.invite(99,1,0)&&!state.invite(1,99,0)&&!state.remove(99),"unknown players");
    State other(4);check(!other.accept(1,first,0),"instance isolation");
    fw::gw::world::Instance world;world.configure_party_capacity(4);
    auto a=std::make_shared<fw::gw::world::Outbox>(),b=std::make_shared<fw::gw::world::Outbox>(),c=std::make_shared<fw::gw::world::Outbox>();
    world.join(1,"Leader",0,0,a);world.join(2,"Member",0,0,b);world.join(3,"Outsider",0,0,c);
    a->drain();b->drain();c->drain();
    check(world.party_action(1,158,2,0),"world invitation");
    check(a->drain().back()==fw::gw::world::Packet{195,1,2,0}&&
          b->drain().back()==fw::gw::world::Packet{196,1,1,0}&&c->drain().empty(),"invite notification scope");
    check(!world.party_action(2,158,1,0)&&a->drain().empty()&&b->drain().empty(),
          "reciprocal request emits no second client list insertion");
    check(world.party_action(2,154,1,1),"world acceptance");
    const auto senderAccepted=a->drain(),recipientAccepted=b->drain();c->drain();
    check(senderAccepted.front()==fw::gw::world::Packet{197,1,2,0}&&
          recipientAccepted.front()==fw::gw::world::Packet{198,1,1,0},"merge clears original invitation audiences");
    check(std::count(recipientAccepted.begin(),recipientAccepted.end(),fw::gw::world::Packet{197,1,2,0})==0,
          "recipient must not clear nonexistent outgoing invite");
    check(world.party_roster(2).size()==5&&world.party_roster(2).back().back()==0,"member roster selection");
    check(world.chat(1,u"#Together"),"party chat");
    const auto message=a->drain();check(!message.empty()&&b->drain()==message&&c->drain().empty(),"chat follows membership");
    check(world.set_ready(2,true),"member ready");
    const auto ready=a->drain();check(ready==b->drain()&&ready.size()==1&&ready[0]==fw::gw::party::ready(1,2,true)&&c->drain().empty(),"ready reaches party with correct group ID");
    check(!world.party_action(2,167,1,2),"world rejects nonleader kick");
    check(world.party_action(2,160,0,3),"world leave");a->drain();b->drain();c->drain();
    world.chat(1,u"#Alone");check(!a->drain().empty()&&b->drain().empty()&&c->drain().empty(),"departed member loses party delivery");
    check(world.party_action(1,158,2,100),"expiry invite");a->drain();b->drain();
    world.tick(60100);
    check(a->drain()==std::vector<fw::gw::world::Packet>{{197,1,2,0}}&&b->drain()==std::vector<fw::gw::world::Packet>{{198,1,1,0}},"expiry clears both invitation UIs");
    check(!world.party_invite_name(1,u"Leader",70000)&&!world.party_invite_name(1,u"Missing",70000),"name invite rejects self and missing target");
    check(!world.party_invite_name(99,u"Member",70000),"name invite rejects unknown caller");
    check(world.party_invite_name(1,u"mEmBeR",70000),"case insensitive name invite");
    check(world.party_action(2,156,1,70001),"reject named invitation");
    a->drain();b->drain();c->drain();
    check(world.chat(1,u"/invite Member",70001)&&
          a->drain()==fw::gw::chat::local(1,0,u"Party membership can only be changed in a supported town or outpost.")&&
          b->drain().empty()&&c->drain().empty()&&!world.party_action(2,154,1,70002),
          "chat invitation defaults to private denial without authoritative staging permission");
    fw::gw::chat::commands::Context staging;staging.party_changes_allowed=true;
    check(world.chat(1,u"/invite Member",70002,staging),"staging chat invitation command");
    check(world.party_action(2,154,1,70003),"staging chat invitation can be accepted");
    auto duplicate=std::make_shared<fw::gw::world::Outbox>();
    world.join(4,"Outsider",0,0,duplicate);
    check(!world.party_invite_name(1,u"Outsider",70003),"ambiguous name cannot invite arbitrary player");
    State pre(2);pre.add(1);pre.add(2);pre.add(3);
    check(pre.invite(1,2,0)&&pre.accept(2,pre.group(1),1)&&!pre.invite(1,3,2),"2022 pre searing capacity");
    std::cout<<"PASS party consent, expiry, capacity, leader authority, names, departure and ID reuse\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

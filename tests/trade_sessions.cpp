#include "gw_trade_sessions.h"
#include <iostream>
using namespace fw::gw::trade;
void check(bool condition,const char*reason){if(!condition)throw std::runtime_error(reason);}
int main()try{
    Sessions sessions;
    auto a=sessions.attach({1,10,1,false,0,0}),b=sessions.attach({2,10,2,false,3,4});
    auto foreign=sessions.attach({3,11,3,false,0,0}),same=sessions.attach({1,10,4,false,0,0});
    auto pvp=sessions.attach({4,10,5,true,0,0});
    check(a&&b&&foreign&&same&&pvp,"attach");
    check(!sessions.attach({9,10,1,false,0,0}),"duplicate player slot");
    check(sessions.open(a,2,4)==Sessions::Result::Rejected,"out of range");
    for(auto target:{1u,3u,4u,5u,99u})check(sessions.open(a,target,10)==Sessions::Result::Rejected,"invalid partner");
    check(sessions.open(a,2,5)==Sessions::Result::Requested,"request");
    check(sessions.open(a,2,5)==Sessions::Result::Requested,"duplicate request");
    check(sessions.drain(b)==std::vector<std::vector<std::uint8_t>>{initiated_2022(1)},"one incoming prompt");
    check(sessions.drain(a)==std::vector<std::vector<std::uint8_t>>{requested_2022(2)},"initiator full window");
    check(sessions.open(same,2,5)==Sessions::Result::Rejected,"busy partner");
    auto extra=sessions.attach({5,10,6,false,0,0});
    check(sessions.open(same,6,5)==Sessions::Result::Rejected,"same owner opened parallel trade");
    check(sessions.open(b,1,5)==Sessions::Result::Initiated,"accept request");
    check(sessions.active(a)&&sessions.active(b),"active pair");
    check(sessions.drain(a).empty()&&sessions.drain(b).empty(),"automatic ack does not reopen prompt");
    for(auto id:{a,b})check(sessions.open(id,id==a?2:1,5)==Sessions::Result::Acknowledged,"automatic acknowledgement");
    check(sessions.drain(a).empty()&&sessions.drain(b).empty(),"ack loop");
    check(sessions.cancel(a)&&!sessions.cancel(a),"cancel idempotent");
    check(!sessions.active(a)&&!sessions.active(b),"cancel clears pair");
    for(auto id:{a,b})check(sessions.drain(id)==std::vector<std::vector<std::uint8_t>>{cancelled_2022()},"both close");
    sessions.open(a,2,5);sessions.detach(a);
    check(sessions.drain(b)==std::vector<std::vector<std::uint8_t>>{cancelled_2022()},"disconnect replaces stale request");
    auto replacement=sessions.attach({1,10,1,false,0,0});check(replacement!=a,"connection lease reused");
    sessions.open(replacement,2,5);sessions.detach(a);
    check(sessions.open(b,1,5)==Sessions::Result::Initiated,"stale cleanup affected replacement");
    check(!sessions.update(b,{2,12,2,false,0,0}),"travel identity update accepted");
    check(sessions.update(b,{2,10,2,true,0,0}),"pvp update");
    check(!sessions.active(replacement)&&!sessions.active(b),"pvp transition cancellation");
    Sessions bounded(1);check(bounded.attach({1,1,1,false,0,0})&&!bounded.attach({2,1,2,false,0,0}),"capacity");
    check(extra!=0,"extra connection");
    {
        Sessions leases;
        const auto trader=leases.attach({"trader",7,101,false,0,0});
        const auto partner=leases.attach({"partner",7,202,false,0,0});
        // Actor 1 aliases trader's connection handle, but belongs to another player.
        const auto unrelated=leases.attach({"unrelated",7,1,false,0,0});
        check(trader==1&&partner!=202&&unrelated!=1,"fixture separates actor IDs from connection leases");
        const auto allowed=[&](Sessions::Handle connection,bool in_scene=true){
            return fw::gw::inventory::mutation_allowed(in_scene,leases.active(connection));
        };
        check(leases.open(trader,202,5)==Sessions::Result::Requested&&
              leases.open(partner,101,5)==Sessions::Result::Initiated,"mismatched actor pair opens trade");
        check(!leases.active(101)&&!leases.active(202),"actor IDs cannot identify active connection leases");
        check(!allowed(trader)&&!allowed(partner),"active trade blocks both owners' inventory mutations");
        check(allowed(unrelated),"actor alias cannot block unrelated inventory mutations");
        check(leases.cancel(trader)&&!leases.cancel(trader),"mutation gate cancellation is idempotent");
        check(allowed(trader)&&allowed(partner)&&!allowed(trader,false),"cancel restores inventory only in a valid scene");
        leases.drain(trader);leases.drain(partner);
        check(leases.open(trader,202,5)==Sessions::Result::Requested&&
              leases.open(partner,101,5)==Sessions::Result::Initiated,"reopen before disconnect");
        leases.detach(trader);
        check(allowed(partner),"disconnect releases partner inventory gate");
        const auto reconnected=leases.attach({"trader",7,101,false,0,0});
        check(reconnected&&reconnected!=trader&&reconnected!=101,"reconnect creates a distinct connection lease");
        check(allowed(reconnected),"reconnect does not inherit departed trade");
        leases.drain(partner);
        check(leases.open(reconnected,202,5)==Sessions::Result::Requested&&
              leases.open(partner,101,5)==Sessions::Result::Initiated,"reconnected actor can trade again");
        leases.detach(trader);
        check(!allowed(reconnected)&&!allowed(partner)&&allowed(unrelated),"stale cleanup cannot unlock replacement or block unrelated player");
        check(leases.cancel(partner)&&allowed(reconnected)&&allowed(partner),"partner cancel releases replacement inventory gate");
    }
    Sessions exchange;
    auto left=exchange.attach({"left",1,1,false,0,0}),right=exchange.attach({"right",1,2,false,0,0});
    exchange.open(left,2,10);exchange.open(right,1,10);
    check(exchange.offer(left,{"char-left",1,{10,{{7,2}}}})&&exchange.offer(right,{"char-right",2,{20,{{8,3}}}}),"offer valid owned snapshots");
    unsigned commits=0;
    auto commit=[&](const Sessions::Commit& state){++commits;check(state.left.owner=="right"&&state.right.owner=="left"&&state.left_offer.revision==2&&state.right_offer.offered.items.at(0).item==7,"durable owner offer snapshot");return true;};
    check(exchange.accept(left,commit)&&commits==0,"first acceptance cannot exchange");
    check(exchange.offer(right,{"char-right",2,{25,{{8,3}}}}),"offer edit accepted");
    check(exchange.accept(right,commit)&&commits==0,"offer edit revokes prior acceptance");
    check(!exchange.accept(left,[](const auto&){return false;})&&exchange.active(left)&&exchange.active(right),"failed atomic commit keeps session and resets acceptance");
    check(exchange.accept(left,commit)&&commits==0,"failed commit reset both acceptances");
    check(exchange.accept(right,commit)&&commits==1&&!exchange.active(left)&&!exchange.active(right),"only mutual acceptance commits and closes");
    exchange.open(left,2,10);exchange.open(right,1,10);
    check(exchange.offer(left,{"char-left",3,{}})&&exchange.offer(right,{"char-right",3,{}}),"new offers after committed exchange");
    unsigned retries=0;
    auto retry=[&](const Sessions::Commit&){++retries;return true;};
    check(exchange.accept(left,retry)&&retries==0,"first acceptance before exceptional commit");
    bool threw=false;
    try{exchange.accept(right,[](const Sessions::Commit&)->bool{throw std::runtime_error("storage failure");});}
    catch(const std::runtime_error&){threw=true;}
    check(threw&&exchange.active(left)&&exchange.active(right),"exception propagates while retaining trade session");
    check(exchange.accept(right,retry)&&retries==0,"exceptional commit revokes both acceptances");
    check(exchange.accept(left,retry)&&retries==1&&!exchange.active(left)&&!exchange.active(right),"retry requires renewed mutual acceptance");
    std::cout<<"PASS owned same-world trade request/accept/ack/cancel/disconnect/PvP lifecycle\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

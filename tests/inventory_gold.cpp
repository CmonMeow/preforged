#include "gw_inventory_gold.h"
#include "gw_inventory_identity.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace fw::gw;
namespace currency=inventory::gold;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class Function>static void rejects(Function&& fn,const char* message){bool rejected=false;try{fn();}catch(const std::exception&){rejected=true;}check(rejected,message);}
static unsigned read(const items::Packet& packet,unsigned offset,unsigned width){unsigned result=0;for(unsigned n=0;n<width;++n)result|=unsigned(packet.at(offset+n))<<(8*n);return result;}
static items::Packet unhex(const std::string& text){check(!(text.size()%2),"fixture hex framing");items::Packet packet;for(unsigned n=0;n<text.size();n+=2){unsigned value=0;std::istringstream stream(text.substr(n,2));stream>>std::hex>>value;check(!stream.fail(),"fixture hex");packet.push_back(static_cast<std::uint8_t>(value));}return packet;}
static currency::Balances replay_deltas(currency::Balances state,const currency::Batch& packets){
    for(const auto& packet:packets){check(packet.size()==8&&read(packet,2,2)==1,"gold delta native layout");const auto op=read(packet,0,2),amount=read(packet,4,4);
        switch(op){case 319:check(state.carried<=currency::carried_cap-amount,"client carried overflow prevented");state.carried+=amount;break;
        case 320:check(state.stored<=currency::stored_cap-amount,"client storage overflow prevented");state.stored+=amount;break;
        case 334:check(state.carried>=amount,"client carried underflow prevented");state.carried-=amount;break;
        case 335:check(state.stored>=amount,"client storage underflow prevented");state.stored-=amount;break;default:throw std::runtime_error("unexpected currency opcode");}
    }return state;
}
int main(int argc,char** argv){try{
    check(argc==2,"gold native fixture path required");std::ifstream input(argv[1]);check(bool(input),"gold fixture unavailable");std::map<std::string,items::Packet> fixtures;
    for(std::string line;std::getline(input,line);){std::istringstream stream(line);std::string label,hex;stream>>label>>hex;if(!label.empty())fixtures.emplace(label,unhex(hex));}
    for(const auto& [op,amount]:std::initializer_list<std::pair<unsigned,unsigned>>{{319,25},{320,50},{334,100},{335,10}}){auto native=fixtures.at("delta-"+std::to_string(op));native.at(1)&=0x7f;
        check(currency::delta_2022(op,1,amount)==native,"gold delta matches original serializer fixture");}
    currency::Request parsed;
    const auto request=fixtures.at("deposit-request");check(currency::parse(request,parsed)==world::FrameState::complete&&parsed.consumed==10&&parsed.balances==currency::Balances{900,500100},"original native deposit final balances parsed");
    check(currency::parse(fixtures.at("withdraw-request"),parsed)==world::FrameState::complete&&parsed.balances==currency::Balances{1100,499900},"original native withdrawal field order");
    check(currency::parse(fixtures.at("drop-gold-request"),parsed)==world::FrameState::unknown_opcode,"gold drop is not silently treated as storage transfer");
    for(unsigned size=0;size<request.size();++size){parsed={{12,34},56};check(currency::parse(std::span(request).first(size),parsed)==world::FrameState::incomplete&&parsed.balances==currency::Balances{12,34}&&parsed.consumed==56,"fragmented request does not overwrite parse output");}
    auto coalesced=request;coalesced.insert(coalesced.end(),request.begin(),request.end());check(currency::parse(coalesced,parsed)==world::FrameState::complete&&parsed.consumed==10,"coalesced stream consumes one native request");

    const currency::Balances current{1000,500000};
    const auto deposit=currency::propose(current,{900,500100},{true,true,true});
    check(deposit.accepted()&&deposit.deposit&&deposit.amount==100&&deposit.balances==currency::Balances{900,500100},"authorized deposit derives exact movement");
    check(deposit.packets.size()==2&&read(deposit.packets[0],0,2)==334&&read(deposit.packets[1],0,2)==320&&replay_deltas(current,deposit.packets)==deposit.balances,"deposit native debit then credit yields final balances");
    const auto withdrawal=currency::propose(current,{1100,499900},{true,true,true});
    check(withdrawal.accepted()&&!withdrawal.deposit&&withdrawal.amount==100&&read(withdrawal.packets[0],0,2)==335&&read(withdrawal.packets[1],0,2)==319&&replay_deltas(current,withdrawal.packets)==withdrawal.balances,"withdrawal native debit then credit yields final balances");
    const auto unchanged=currency::propose(current,current,{true,false,false});check(unchanged.accepted()&&!unchanged.amount&&unchanged.packets.empty(),"authorized unchanged request has no repeated additions");
    check(currency::propose(current,current,{false,true,true}).status==currency::Reason::mode&&currency::propose(current,{900,500100},{false,true,true}).status==currency::Reason::mode,"closed or distant chest denies gold changes");
    check(currency::propose(current,{900,500100},{true,false,true}).status==currency::Reason::direction&&currency::propose(current,{1100,499900},{true,true,false}).status==currency::Reason::direction,"direction permissions enforced");
    for(const auto desired:{currency::Balances{1001,500000},currency::Balances{999,500000},currency::Balances{1000,500001}}){const auto rejected=currency::propose(current,desired,{true,true,true});check(rejected.status==currency::Reason::conservation&&rejected.balances==current&&rejected.packets.empty(),"forged mint or destruction rejects without mutation");}
    check(currency::propose(current,{currency::carried_cap+1,0},{true,true,true}).status==currency::Reason::limit&&currency::propose(current,{0,currency::stored_cap+1},{true,true,true}).status==currency::Reason::limit,"both native UI caps enforced");
    check(currency::total({UINT32_MAX,UINT32_MAX})==8589934590ull,"conservation totals cannot wrap DWORD arithmetic");
    const currency::Balances maximum{currency::carried_cap,currency::stored_cap};
    check(replay_deltas({},currency::updates({},maximum))==maximum&&replay_deltas(maximum,currency::updates(maximum,{}))==currency::Balances{},"initial restoration and full zero reconciliation stay within caps");
    check(currency::updates(maximum,maximum).empty(),"opening an already restored chest cannot add balances again");
    rejects([&]{currency::updates({currency::carried_cap+1,0},{});},"malformed known client gold rejected");
    rejects([&]{currency::delta_2022(319,1,0);},"empty delta omitted not published");
    rejects([&]{currency::delta_2022(320,1,currency::stored_cap+1);},"unsafe raw delta cannot exceed domain cap");
    rejects([&]{currency::delta_2022(321,1,1);},"inventory removal cannot masquerade as currency delta");

    // Many boundary/interior balances and arbitrary legal directions verify
    // conservation plus the actual integer client mutation sequence.
    std::uint32_t random=1234567;
    for(unsigned n=0;n<4000;++n){random=random*1664525u+1013904223u;currency::Balances before{random%(currency::carried_cap+1),0};random=random*1664525u+1013904223u;before.stored=random%(currency::stored_cap+1);
        random=random*1664525u+1013904223u;const bool inward=random&1;
        const auto available=inward?std::min(before.carried,currency::stored_cap-before.stored):std::min(before.stored,currency::carried_cap-before.carried);
        const auto amount=available?random%(available+1):0;auto desired=before;
        if(inward){desired.carried-=amount;desired.stored+=amount;}else{desired.carried+=amount;desired.stored-=amount;}
        const auto planned=currency::propose(before,desired,{true,true,true});check(planned.accepted()&&currency::total(before)==currency::total(planned.balances)&&replay_deltas(before,planned.packets)==desired,"conservation and delta property at legal limits");
    }
    // Identity projection must carry both persisted currency fields unchanged
    // until the caller's two-revision transaction succeeds.
    auto character=inventory::baseline(),bank=storage::baseline();character.gold=current.carried;bank.gold=current.stored;
    inventory::identity::Session identities;auto wire=identities.hydrate(character,bank);check(wire.character.gold==current.carried&&wire.bank.gold==current.stored,"hydration preserves gold without treating it as item identity");
    const auto revision=identities.revision();auto proposed_character=wire.character,proposed_bank=wire.bank;proposed_character.gold=deposit.balances.carried;proposed_bank.gold=deposit.balances.stored;
    auto projection=identities.prepare(proposed_character,proposed_bank);
    check(projection.durable.character.gold==900&&projection.durable.bank.gold==500100&&identities.revision()==revision,"gold prepared atomically in both durable domains without registry commit");
    const auto rollback=identities.prepare(wire.character,wire.bank);check(rollback.durable.character.gold==1000&&rollback.durable.bank.gold==500000,"discarded failed CAS preserves original durable gold state");
    check(identities.commit(std::move(projection)),"successful two-domain gold CAS commits identity projection");
    check(projection.wire.character.gold==900&&projection.wire.bank.gold==500100,"operational profiles remain synchronized after commit");
    std::cout<<"inventory gold tests passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

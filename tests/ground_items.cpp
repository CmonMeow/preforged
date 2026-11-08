#include "gw_ground_items.h"
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>
#include <limits>

namespace {
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
std::string hex(const fw::gw::items::Packet& p){const char* digits="0123456789abcdef";std::string s;
    for(auto b:p){s+=digits[b>>4];s+=digits[b&15];}return s;}
}
int main(int argc,char** argv)try{
    namespace g=fw::gw::world::ground;
    using fw::gw::items::Definition;
    const g::Scope creator{1,7,4},other{1,8,5},drop{1,73,6};
    Definition detail;detail.id=42;detail.file=1234;detail.type=11;detail.quantity=20;detail.encoded_name=u"Fixture";
    g::Item item{drop,creator,9,detail,{12.5f,-10.f,2},100,g::Allocation{creator,600100}};
    g::State state;
    require(state.add(item)&&state.size()==1,"ground item requires actual immutable definition");
    require(!state.add(item),"duplicate actor/item identity rejected");
    auto duplicate=item;duplicate.scope={1,74,7};require(!state.add(duplicate),"same client item identity cannot alias a second actor");
    duplicate=item;duplicate.definition.id=43;require(!state.add(duplicate),"same actor cannot alias another item");
    require(g::can_pickup(item,creator,{12.5f,-10.f,2},100,160),"owner can collect current own allocation");
    require(!g::can_pickup(item,other,{12.5f,-10.f,2},100,160),"other player cannot steal assignment");
    require(g::can_pickup(item,other,{12.5f,-10.f,2},600100,160),"exact allocation deadline permits another player");
    require(!g::can_pickup(item,creator,{12.5f,-10.f,1},100,160)&&
        !g::can_pickup(item,creator,{172.501f,-10.f,2},100,160),"different plane and out of range reject pickup");
    require(g::can_pickup(item,creator,{172.5f,-10.f,2},100,160),"closed exact private pickup range");
    require(!g::can_pickup(item,{2,7,4},{12.5f,-10.f,2},100,160)&&
        !g::can_pickup(item,{1,7,5},{12.5f,-10.f,2},100,160)&&
        !g::can_pickup(item,creator,{12.5f,-10.f,2},99,160),"scope and pre-creation time guards");
    for(float distance:{0.f,-1.f,513.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
        require(!g::can_pickup(item,creator,{12.5f,-10.f,2},100,distance),"invalid or missing range never grants pickup");
    const auto live=g::presentation(item,100);
    require(live.size()==3&&live[0][0]==96&&live[0][1]==1&&live[1][0]==32&&live[2][0]==52&&live[2][1]==1,
        "definition precedes raw type4 actor, then allocation follows proved native lifecycle");
    auto metadata=item;metadata.profession=6;metadata.customized=u"Fixture Owner";
    g::State metadata_state;require(metadata_state.add(metadata),"ground source preserves actual metadata");
    const auto decorated=g::presentation(*metadata_state.find(drop),100);
    require(decorated.size()==5&&decorated[0][0]==96&&decorated[1][0]==89&&decorated[1][1]==1&&
        decorated[2][0]==57&&decorated[2][1]==1&&decorated[3][0]==32&&decorated[4][0]==52,
        "original345 and313 metadata precede actor reference and ownership update");
    require(metadata_state.find(drop)->profession==6&&metadata_state.find(drop)->customized==u"Fixture Owner",
        "pickup source retains profession and customization exactly");
    require(state.release(600099).empty()&&state.find(drop)->allocation.has_value(),"timer cannot release early");
    require(state.release(600100).size()==1&&!state.find(drop)->allocation&&state.size()==1,"expiry unassigns but retains item");
    require(state.release(600101).empty(),"assignment expiration publishes once");
    require(!state.consume({1,73,7},1)&&!state.consume(drop,0)&&!state.consume(drop,21),"stale generation and invalid counts do not mutate");
    auto staged=state;require(staged.consume(drop,5)&&staged.find(drop)->definition.quantity==15&&
        state.find(drop)->definition.quantity==20,"staged partial pickup cannot alter committed state");
    state=std::move(staged);require(state.consume(drop,15)&&state.size()==0,"full pickup removes source stack");
    require(state.used_actor(73)&&state.used_item(42)&&!state.add(item),"retired wire identities cannot resolve old requests to a new drop");
    g::State owner_exit;require(owner_exit.add(item),"owner release fixture");
    require(owner_exit.release(0,g::Scope{1,7,3}).empty()&&owner_exit.find(drop)->allocation,"old owner generation cannot release replacement actor allocation");
    require(owner_exit.release(0,creator).size()==1&&!owner_exit.find(drop)->allocation&&owner_exit.size()==1,"owner leave releases ownership without deleting item");
    auto invalid=item;invalid.allocation->until=600101;g::State invalid_store;
    require(!invalid_store.add(invalid),"assignment greater than ten minutes is not supported");
    invalid=item;invalid.definition.file=0;require(!invalid_store.add(invalid),"invalid asset reference does not create ground item");
    invalid=item;invalid.definition.quantity=0;require(!invalid_store.add(invalid),"empty stacks cannot become floor items");
    invalid=item;invalid.definition.id=65536;require(!invalid_store.add(invalid),"private wire table bound prevents unbounded client allocations");
    invalid=item;invalid.scope.instance=2;require(!invalid_store.add(invalid),"different source instance rejected");
    invalid=item;invalid.position.x=std::numeric_limits<float>::quiet_NaN();require(!invalid_store.add(invalid),"nonfinite ground coordinates rejected");
    invalid=item;invalid.profession=12;require(!invalid_store.add(invalid),"invalid item profession does not reach a floor callback");
    invalid=item;invalid.customized=std::u16string(32,u'x');require(!invalid_store.add(invalid),"customization bounds preserved");
    g::State bounded;for(unsigned n=0;n<g::State::capacity;++n){auto actual=item;actual.scope.actor+=n;
        actual.scope.generation+=n;actual.definition.id+=n;require(bounded.add(actual),"bounded ground catalog fixture");}
    auto full=item;full.scope.actor+=100;full.definition.id+=100;require(!bounded.add(full),"ground catalog cannot overflow its bounded snapshot capacity");
    const fw::gw::items::Packet pickup{61,0,73,0,0,0,1};
    require(g::pickup_request(pickup)&&g::pickup_request(pickup)->actor==73&&g::pickup_request(pickup)->call_target,"native pickup fixture decodes actor identity");
    auto transport_pickup=pickup;transport_pickup[1]=0x80;
    require(g::pickup_request(transport_pickup)&&g::pickup_request(transport_pickup)->actor==73,"native transport-header bit accepted");
    for(auto p:{fw::gw::items::Packet{61,0,0,0,0,0,0},fw::gw::items::Packet{61,0,73,0,0,0,2},
        fw::gw::items::Packet{61,0,73,0,0,0},fw::gw::items::Packet{61,0,73,0,1,0,0}})
        require(!g::pickup_request(p),"malformed pickup rejected without accepting a persistent item ID");
    if(argc==2){std::ifstream fixtures(argv[1]);require(bool(fixtures),"native fixture file missing");std::string label,wire;unsigned count=0;
        while(fixtures>>label>>wire){std::string actual;
            if(label=="spawn")actual=hex(g::spawn_2022(73,42,12.5f,-10.f,2));
            else if(label=="owner")actual=hex(g::owner_2022(42,7,600));
            else if(label=="unassign")actual=hex(g::owner_2022(42,0,0));
            else if(label=="despawn")actual=hex(g::despawn_2022(73));
            else if(label=="delete")actual=hex(fw::gw::items::delete_2022(42));
            else if(label=="pickup")actual=hex(pickup);else throw std::runtime_error("unknown native ground fixture");
            // Original serializer adds transport header bit0x8000; the host
            // encodes that separately from these canonical opcode helpers.
            require(wire.size()>=4,"invalid native ground fixture");
            require(wire[2]=='8',"expected original transport-header flag");wire[2]='0';
            require(actual==wire,"ground lifecycle wire differs from original serializer");++count;
        }require(count==6,"six original serializer fixtures required");
    }else require(argc==1,"invalid ground test arguments");
    std::cout<<"Ground item state: original wire lifecycle, allocations, exact lifetimes/range, staging, partial quantities, retired identities PASS\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"Ground items: "<<e.what()<<'\n';return 1;}

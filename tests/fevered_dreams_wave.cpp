#include "gw_fevered_wave.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using Kind=combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Fevered wave line "+std::to_string(at.line()));}
int main()try{
 const Scope caster{7,9,1},a{7,1,2},b{7,2,3},c{7,3,4};Registry cr(caster);
 auto make=[&](Scope scope,double x,bool hex){Registry r(scope);std::optional<FeveredOwnership> owner;
  if(hex){auto app=*prepare_fevered_dreams(r,cr,caster,scope,scope.actor,*fevered_dreams(12),100);r=app.registry;owner=app.owner;}
  return FeveredNode{scope,r,{},FeveredEvents(scope),owner,x,0,true};};
 std::vector<FeveredNode> nodes{make(caster,0,false),make(a,0,true),make(b,100,true),make(c,200,true)};
 check(nodes[2].conditions.apply(Kind::bleeding,9,100,10000));
 FeveredRelations relations;for(auto s:{a,b,c})relations[{caster,s}]=true;
 auto after=nodes[1].conditions;check(after.apply(Kind::poison,9,200,5000));
 auto wave=prepare_fevered_wave(nodes,relations,a,after,200,1,FeveredDazedSpan::attempted_duration);
 check(bool(wave));for(unsigned i=1;i<4;++i)check(wave->nodes[i].conditions.has(Kind::poison,200)&&wave->nodes[i].conditions.has(Kind::dazed,200));
 check(!nodes[1].conditions.has(Kind::poison,200)&&!nodes[3].conditions.has(Kind::dazed,200));
 check(wave->commits.size()>3&&wave->commits.size()<40);
 for(std::size_t i=0;i<wave->commits.size();++i)check(wave->commits[i].event.event==i+1);
 auto missing=relations;missing.erase({caster,c});check(!prepare_fevered_wave(nodes,missing,a,after,200,1,FeveredDazedSpan::attempted_duration));
 check(!prepare_fevered_wave(nodes,relations,a,after,200,UINT64_MAX,FeveredDazedSpan::attempted_duration));
 auto stale=nodes;stale[0].scope.generation++;check(!prepare_fevered_wave(stale,relations,a,after,200,1,FeveredDazedSpan::attempted_duration));
 auto replay=wave->nodes;check(!prepare_fevered_wave(replay,relations,a,replay[1].conditions,200,1,FeveredDazedSpan::attempted_duration));
 auto fresh=prepare_fevered_wave(replay,relations,a,replay[1].conditions,201,wave->next_event,FeveredDazedSpan::attempted_duration);check(fresh&&fresh->commits.size()==1);
 auto friendly=relations;friendly[{caster,c}]=false;auto fw=prepare_fevered_wave(nodes,friendly,a,after,200,1,FeveredDazedSpan::attempted_duration);check(fw&&!fw->nodes[3].conditions.has(Kind::poison,200));
 std::cout<<"PASS atomic multi-hex Fevered wave, converging branches, immutable rollback, ownership, allegiance and replay guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

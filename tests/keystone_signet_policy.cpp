#include "gw_keystone_signet.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw::skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Keystone policy line "+std::to_string(at.line()));}
int main()try{
 const Scope a{7,1,2},b{7,2,3};
 for(unsigned rank:{0u,1u,2u,12u,15u,20u}){auto plan=*keystone_signet(rank);check(plan.charges==interpolate(0,6,rank)&&plan.damage==interpolate(15,60,rank));KeystoneCharges state(a);check(state.apply(a,plan,1,1,100));
  for(unsigned i=0;i<plan.charges;++i){const KeystoneUse use{a,b,58,i+1,200+i,true,true};auto p=state.prepare(use);check(p&&p->triggers&&p->remaining==plan.charges-i-1&&p->damage==plan.damage&&state.commit(*p,a,b)&&!state.commit(*p,a,b));}
  auto spent=state.prepare({a,b,58,100,300,true,true});check(spent&&!spent->triggers&&state.commit(*spent,a,b));
 }
 KeystoneCharges state(a);check(state.apply(a,*keystone_signet(12),1,1,100));
 for(unsigned skill:{28u,63u,1u}){const bool foe=skill!=63;auto p=state.prepare({a,foe?b:a,skill,skill,200,false,foe});check(p&&!p->triggers);}
 for(auto use:{KeystoneUse{a,b,28,1,200,true,true},KeystoneUse{a,a,1,1,200,true,false},KeystoneUse{a,b,58,1,200,false,true}}){auto p=state.prepare(use);check(p&&!p->triggers);}
 auto p=*state.prepare({a,b,58,1,200,true,true});auto forged=p;forged.damage++;check(!state.commit(forged,a,b));auto stale=b;stale.generation++;check(!state.commit(p,a,stale));
 std::vector<KeystoneContact> contacts{{a,0,true,true,true},{b,0,false,true,true},{{7,3,4},166,true,true,false},{{7,4,5},165,true,true,true},{{7,5,6},167,true,true,true},{{7,6,7},1,true,false,true},{{7,7,8},1,false,true,true}};
 auto recipients=keystone_recipients(p,contacts);check(recipients&&recipients->size()==2&&(*recipients)[0].damage==51&&!(*recipients)[0].interrupt_attempt&&(*recipients)[1].interrupt_attempt);
 check(keystone_recipients(p,{})->empty());auto bad=contacts;bad.push_back({{7,3,99},1,true,true,true});check(!keystone_recipients(p,bad));forged=p;forged.remaining++;check(!keystone_recipients(forged,contacts));
 check(state.apply(a,*keystone_signet(12),2,2,200)&&!state.commit(p,a,b));
 auto next=*state.prepare({a,b,58,1,200,true,true});check(state.commit(next,a,b));check(state.apply(a,*keystone_signet(12),3,3,300));check(!state.prepare({a,b,58,1,400,true,true}));
 auto expired=state.prepare({a,b,58,2,20300,true,true});check(expired&&!expired->triggers);
 auto invalid=*keystone_signet(12);invalid.damage++;check(!state.apply(a,invalid,4,4,400));check(!state.apply(a,*keystone_signet(12),4,4,UINT64_MAX-19999));
 check(!keystone_signet(21)&&!state.prepare({a,a,58,2,400,true,true}));
 std::cout<<"PASS Keystone rank/charges, post-effect scoped proposal, replay/renewal/forgery guards, adjacent other-foe damage and active-only interrupt attempts\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

#include "gw_keystone_recharge.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw::skills;using namespace effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Keystone recharge line "+std::to_string(at.line()));}
int main()try{
 const Scope owner{7,1,2};
 KeystoneCooldowns state{owner,3,0,100,{{{63,0},9000,10000,false},{{1,0},5000,0,false},{{58,65535},2000,3000,false},{{2,0},0,4000,false},{{2,1},0,4000,true},{{1816,0},0,5000,false},{{1816,1},0,5000,true},{{28,0},3000,4000,false},{{1,1},150,150,false}}};
 const auto original=state;auto p=prepare_keystone_recharge(state,owner,200,1);
 check(p&&p->cleared==std::vector<Slot>{{1,0},{58,65535},{2,0},{1816,0}}&&state==original);
 check(p->after.slots[0]==original.slots[0]&&p->after.slots[4]==original.slots[4]&&p->after.slots[6]==original.slots[6]&&p->after.slots[7]==original.slots[7]&&p->after.slots[8]==original.slots[8]);
 auto forged=*p;forged.after.slots[4].spent=false;check(!commit_keystone_recharge(state,forged,owner)&&state==original);
 auto stale=owner;stale.generation++;check(!commit_keystone_recharge(state,*p,stale)&&state==original);
 auto changed=state;changed.slots[1].disabled_until=6000;check(!commit_keystone_recharge(changed,*p,owner));
 check(commit_keystone_recharge(state,*p,owner)&&state.revision==4&&state.last_reset==1&&!commit_keystone_recharge(state,*p,owner));
 auto empty=prepare_keystone_recharge(state,owner,200,2);check(empty&&empty->cleared.empty()&&commit_keystone_recharge(state,*empty,owner));
 auto duplicate=original;duplicate.slots.push_back(duplicate.slots[1]);check(!prepare_keystone_recharge(duplicate,owner,200,1));
 auto bad=original;bad.slots[1].spent=true;check(!prepare_keystone_recharge(bad,owner,200,1));bad=original;bad.slots[1].slot.copy=65536;check(!prepare_keystone_recharge(bad,owner,200,1));
 check(!prepare_keystone_recharge(original,owner,99,1)&&!prepare_keystone_recharge(original,owner,200,0));
 bad=original;bad.revision=UINT64_MAX;check(!prepare_keystone_recharge(bad,owner,200,1));
 std::cout<<"PASS Keystone scoped recharge/disable reset proposals, native2/1816 spent exceptions, copied slots, immutable failure and revision/event guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

#include "gw_cry_frustration.h"
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Cry selection policy");}
int main()try{
 check(cry_frustration(0)->damage==15&&cry_frustration(12)->damage==63&&cry_frustration(15)->damage==75&&cry_frustration(20)->damage==95&&!cry_frustration(21));
 Scope selected{1,2,1};std::vector<CryContact> contacts{{selected,0,true,true,CryActivity::skill},{{1,3,1},322,true,true,CryActivity::basic_attack},{{1,4,1},100,true,true,CryActivity::idle},{{1,5,1},322.01f,true,true,CryActivity::skill},{{1,6,1},100,true,false,CryActivity::skill},{{1,7,1},100,false,true,CryActivity::skill}};
 const auto plan=*cry_frustration(12);const auto result=prepare_cry_frustration(plan,selected,contacts);
 check(result&&result->triggered&&result->recipients.size()==3);
 check(result->recipients[0].damage==63&&result->recipients[0].interrupt_attempt&&result->recipients[1].interrupt_attempt&&!result->recipients[2].interrupt_attempt);
 check(contacts[0].activity==CryActivity::skill); // Proposal does not mutate activity.
 contacts[0].activity=CryActivity::basic_attack;
 const auto idle=prepare_cry_frustration(plan,selected,contacts);check(idle&&!idle->triggered&&idle->recipients.empty());
 contacts[0].activity=CryActivity::skill;check(prepare_cry_frustration(plan,selected,contacts)->triggered); // Attack skill is a skill.
 auto bad=contacts;bad.push_back(bad.front());check(!prepare_cry_frustration(plan,selected,bad));
 bad=contacts;auto duplicate=bad.front();++duplicate.owner.generation;bad.push_back(duplicate);check(!prepare_cry_frustration(plan,selected,bad));
 bad=contacts;bad[1].owner.instance=2;check(!prepare_cry_frustration(plan,selected,bad));
 bad=contacts;bad[1].distance=std::numeric_limits<float>::quiet_NaN();check(!prepare_cry_frustration(plan,selected,bad));
 bad=contacts;bad[0].distance=1;check(!prepare_cry_frustration(plan,selected,bad));
 bad=contacts;bad[0].activity=static_cast<CryActivity>(99);check(!prepare_cry_frustration(plan,selected,bad));
 auto forged=plan;++forged.damage;check(!prepare_cry_frustration(forged,selected,contacts));
 std::cout<<"PASS Cry initial skill-only gate, all-foe damage, attack/idle recipient distinctions,322 boundary and immutable scoped validation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

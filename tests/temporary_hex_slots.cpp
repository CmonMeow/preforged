#include "gw_temporary_hex_slots.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills;using namespace effects;
void check(bool ok){if(!ok)throw std::runtime_error("Temporary Hex slot regression");}
int main()try{
 const Scope owner{1,2,1};TemporaryHexSlots copies(owner);Slots bar{};bar[0]={22,0};bar[1]={28,0};
 Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[22]=1;policy.learned[28]=1;
 check(validate(bar,policy)==Reason::accepted);
 check(!copies.install(owner,bar,0,{22,0},{28,0},100)); // No duplicate skill/copy pair.
 const auto installed=copies.install(owner,bar,0,{22,0},{28,1},100);check(bool(installed));
 const auto grant=copies.authorize(owner,bar,0,{28,1},200);check(bool(grant));
 check(validate(bar,policy)==Reason::copy); // Persistent learning is unchanged.
 check(validate(*copies.eligibility_bar(owner,bar,200),policy)==Reason::accepted);
 check(copies.valid(*grant,bar,20099)&&!copies.valid(*grant,bar,20100));
 check(!copies.eligibility_bar(owner,bar,20100));
 const auto expired=copies.expire(owner,bar,20100);check(expired&&expired->size()==1&&expired->front().restored&&bar[0]==Slot{22,0});
 check(!copies.valid(*grant,bar,20100));
 check(bool(copies.install(owner,bar,0,{22,0},{28,1},20200)));
 check(!copies.valid(*grant,bar,20200)); // Same slot/pair cannot revive an old cast grant.
 bar[0]={30,0};const auto later=copies.expire(owner,bar,40200);check(later&&later->size()==1&&!later->front().restored&&bar[0]==Slot{30,0});
 bar[0]={1049,0};check(bool(copies.install(owner,bar,0,{1049,0},{28,1},40300)));
 const auto before_retirement=copies.authorize(owner,bar,0,{28,1},40300);check(bool(before_retirement));
 const auto retired=copies.retire(owner,bar,40400);check(retired&&bar[0]==Slot{1049,0}&&!copies.valid(*before_retirement,bar,40400));
 check(!copies.install(owner,bar,0,{1049,0},{28,1},40500));
 TemporaryHexSlots fresh(owner);check(!fresh.install({1,2,2},bar,0,{1049,0},{28,1},40500));
 check(!fresh.install(owner,bar,0,{1049,0},{3152,0},40500)); // Alias wire policy remains explicit/unresolved.
 check(!fresh.install(owner,bar,0,{1049,0},{28,1},UINT64_MAX-100));
 std::cout<<"PASS temporary Hex slot lifecycle/cast grants,20second expiry, restored original, stale-generation/changed-slot isolation, no persistent learning and explicit alias boundary\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

#include "gw_temporary_hex_admission.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills;using namespace effects;
void check(bool ok){if(!ok)throw std::runtime_error("Temporary Hex admission regression");}
int main()try{
 const Scope owner{1,2,1};TemporaryHexSlots copies(owner);Slots bar{};bar[0]={22,0};bar[1]={52,0};
 Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[22]=1;policy.learned[52]=1;
 check(bool(copies.install(owner,bar,0,{22,0},{53,0},100)));
 const auto copied=admit_temporary_hex_cast(copies,owner,bar,policy,0,{53,0},200);
 check(copied&&copied->copied&&!policy.learned.contains(53));
 check(valid_temporary_hex_admission(*copied,copies,bar,policy,20099));
 check(!valid_temporary_hex_admission(*copied,copies,bar,policy,20100));
 check(!admit_temporary_hex_cast(copies,{1,2,2},bar,policy,0,{53,0},200));
 check(!admit_temporary_hex_cast(copies,owner,bar,policy,1,{53,0},200));
 bar[2]={30,0};check(!admit_temporary_hex_cast(copies,owner,bar,policy,0,{53,0},200));bar[2]={};
 const auto ordinary=admit_temporary_hex_cast(copies,owner,bar,policy,1,{52,0},200);check(ordinary&&!ordinary->copied);
 check(bool(copies.expire(owner,bar,20100)));check(bar[0]==Slot{22,0});
 check(bool(copies.install(owner,bar,0,{22,0},{101,0},20200))); // Off-profession, unlearned hex capability.
 const auto off_profession=admit_temporary_hex_cast(copies,owner,bar,policy,0,{101,0},20200);
 check(off_profession&&off_profession->copied&&!policy.learned.contains(101));
 auto ordinary_wrong_profession=bar;ordinary_wrong_profession[2]={101,1};auto learned=policy;learned.learned[101]=2;
 check(!admit_temporary_hex_cast(copies,owner,ordinary_wrong_profession,learned,0,{101,0},20200));
 auto ordinary_wrong_campaign=bar;ordinary_wrong_campaign[2]={1049,0};auto limited=policy;limited.campaigns=1u<<1;limited.learned[1049]=1;
 check(!admit_temporary_hex_cast(copies,owner,ordinary_wrong_campaign,limited,0,{101,0},20200));
 auto without_source=policy;without_source.learned.erase(22);check(!admit_temporary_hex_cast(copies,owner,bar,without_source,0,{101,0},20200));
 check(!valid_temporary_hex_admission(*copied,copies,bar,policy,20200));
 std::cout<<"PASS temporary copied-slot admission, explicit elite/off-profession capabilities, no unrelated learning/profession/campaign bypass and completion expiry/scope/index isolation; Runtime pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

#include "gw_identification_catalog.h"
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>

int main(int argc,char** argv)try{
    using namespace fw::gw;using namespace identification;
    auto check=[](bool good,const char* message){if(!good)throw std::runtime_error(message);};
    check(argc==2,"native identification fixture required");
    std::ifstream input(argv[1]);check(bool(input),"missing native fixture");
    std::map<std::string,items::Packet> fixtures;std::string label,hex;
    while(input>>label>>hex){items::Packet packet;check(hex.size()%2==0,"odd hex");
        for(std::size_t at=0;at<hex.size();at+=2)packet.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(at,2),nullptr,16)));
        check(fixtures.emplace(label,std::move(packet)).second,"duplicate fixture");}
    auto native=[&](const char* name,items::Packet packet){packet[1]|=0x80;check(packet==fixtures.at(name),"differs from native2022 serializer");};
    auto p=inventory::baseline();p.next_item=44;p.gold=777;
    items::Definition kit;kit.id=43;kit.file=1234;kit.type=29;kit.model=999;kit.quantity=1;kit.price=50;
    kit.encoded_name=u"Fixture";kit.modifiers={0x25e80100,0x24580019};
    p.items.emplace(43,inventory::Item{kit,1,5,7,u"Known Owner"});
    items::Definition target;target.id=42;target.file=1234;target.type=27;target.model=567;target.unknown1=3;target.dye=0x123;
    target.price=100;target.encoded_name=u"Fixture";target.modifiers={0x25220001};
    p.items.emplace(42,inventory::Item{target,1,4,1,u"Known Owner"});
    Policy policy{true,true};const auto original=inventory::encode(p);
    auto charged=debit_kit(p,43,Kit::identification,policy);
    check(charged.accepted()&&charged.uses_before==25&&charged.uses_after==24&&!charged.exhausted,"one authentic charge debited");
    check(charged.profile.items.size()==2&&charged.profile.items.at(43).definition.quantity==1&&charged.profile.next_item==44,"uses are not quantity/new item");
    check(charged.profile.items.at(43).definition.modifiers==std::vector<std::uint32_t>{0x25e80100,0x24580018},"only low charge byte changes");
    check(charged.profile.items.at(43).definition.price==50&&charged.profile.gold==777,"no guessed prices or gold debited");
    check(charged.profile.items.at(43).profession==7&&charged.profile.items.at(43).customized==u"Known Owner","kit metadata preserved");
    check(charged.packets.size()==2,"remaining kit updates bounded");
    native("kit351-uses24",charged.packets[0]);native("kit312-quantity1",charged.packets[1]);
    check(inventory::encode(p)==original,"planner mutates input");
    auto reveal=p.items.at(42);reveal.definition.interaction|=1;reveal.definition.price=200;reveal.definition.modifiers.push_back(0x2798090c);
    {
        auto recipe=inventory::baseline();recipe.next_item=3;
        auto before=p.items.at(42);before.definition.id=1;before.bag=1;before.slot=0;
        auto after=reveal;after.definition.id=2;after.bag=1;after.slot=1;recipe.items={{1,before},{2,after}};
        auto encode_hex=[](const items::Packet& bytes){std::string result;for(auto b:bytes){result+="0123456789abcdef"[b>>4];result+="0123456789abcdef"[b&15];}return result;};
        const auto path=std::filesystem::temp_directory_path()/"gw-reviewed-identification-test.txt";
        const auto row=std::string(64,'b')+'\t'+encode_hex(inventory::encode(recipe))+'\n';
        {std::ofstream file(path);file<<row;}Catalog catalog;catalog.load(path);
        auto provided=catalog.reveal(p.items.at(42));check(provided&&provided->definition.id==42&&provided->bag==1&&provided->slot==4&&provided->customized==reveal.customized,"exact provider preserves real identity placement owner");
        check(identify(p,43,42,provided,policy).accepted(),"reviewed catalog feeds actual identification proposal");
        auto altered=p.items.at(42);altered.definition.model++;check(!catalog.reveal(altered),"different asset cannot use reveal recipe");
        altered=p.items.at(42);altered.customized=u"Other Owner";check(!catalog.reveal(altered),"customized owner is part of exact recipe match");
        auto second=recipe;second.items.at(1).definition.file=321;second.items.at(2).definition.file=321;second.items.at(2).definition.price=333;
        {std::ofstream file(path);file<<row<<std::string(64,'c')+'\t'+encode_hex(inventory::encode(second))+'\n';}catalog.load(path);
        auto second_source=p.items.at(42);second_source.definition.file=321;
        const auto second_reveal=catalog.reveal(second_source);
        check(catalog.size()==2&&catalog.reveal(p.items.at(42))->definition.price==200&&second_reveal&&
            second_reveal->definition.price==333&&second_reveal->definition.id==42&&second_reveal->slot==4,
            "signature index selects exact recipe and preserves actual identity and placement");
        ++second_source.definition.quantity;check(!catalog.reveal(second_source),"identification signature retains exact source quantity");
        auto rejects=[&]{bool failed=false;try{catalog.load(path);}catch(const std::invalid_argument&){failed=true;}check(failed&&catalog.size()==2&&catalog.reveal(p.items.at(42)),"catalog failure preserves prior complete index");};
        {std::ofstream file(path);file<<row<<row;}rejects();
        {std::ofstream file(path);file<<std::string(Catalog::maximum_line+1,'a');}rejects();
        std::filesystem::resize_file(path,Catalog::maximum_file+1);rejects();
        auto invalid=recipe;invalid.items.at(2).definition.file++;
        {std::ofstream file(path);file<<std::string(64,'b')+'\t'+encode_hex(inventory::encode(invalid))+'\n';}rejects();
        std::filesystem::remove(path);
    }
    auto done=identify(p,43,42,reveal,policy);
    check(done.accepted()&&done.packets.size()==5,"target definition and metadata follow charge update");
    native("target352-identified",done.packets[2]);native("target345-profession1",done.packets[3]);native("target313-customized",done.packets[4]);
    check(done.profile.items.at(42).definition.price==200&&done.profile.items.at(42).definition.modifiers==reveal.definition.modifiers,"explicit known reveal preserved");
    check(done.profile.items.at(42).bag==1&&done.profile.items.at(42).slot==4&&done.profile.items.at(42).profession==1,"target placement and profession conserved");
    auto persisted=inventory::decode(inventory::encode(done.profile));
    check(inventory::encode(persisted)==inventory::encode(done.profile),"result does not survive durable roundtrip");
    auto denial=[&](const inventory::Profile& before,const Proposal& value,Reason expected){
        check(value.status==expected&&!value.accepted()&&value.packets.empty(),"denial publishes false result");
        check(inventory::encode(value.profile)==inventory::encode(before),"denial consumes property/item");};
    denial(p,identify(p,43,42,{},policy),Reason::outcome);
    denial(p,identify(p,43,43,p.items.at(43),policy),Reason::target);
    denial(p,identify(p,99,42,reveal,policy),Reason::missing);
    denial(p,identify(p,43,42,reveal,{}),Reason::mode);
    denial(p,identify(p,43,42,reveal,{true,false}),Reason::source);
    auto bad=reveal;bad.definition.id=41;denial(p,identify(p,43,42,bad,policy),Reason::target);
    bad=reveal;bad.bag=2;denial(p,identify(p,43,42,bad,policy),Reason::target);
    bad=reveal;bad.customized=u"Other Owner";denial(p,identify(p,43,42,bad,policy),Reason::target);
    bad=reveal;bad.definition.file=999;denial(p,identify(p,43,42,bad,policy),Reason::target);
    bad=reveal;bad.definition.interaction|=0x80000;denial(p,identify(p,43,42,bad,policy),Reason::target);
    bad=reveal;bad.definition.modifiers.push_back(0xc0000000);denial(p,identify(p,43,42,bad,policy),Reason::outcome);
    auto armor=p;armor.items.at(42).definition.type=7;armor.items.at(42).reviewed_armor=inventory::ReviewedArmor{80,20,0};
    auto armor_reveal=armor.items.at(42);armor_reveal.definition.interaction|=1;
    check(identify(armor,43,42,armor_reveal,policy).accepted(),"identification preserves reviewed armor metadata");
    auto altered_armor=armor_reveal;altered_armor.reviewed_armor.reset();
    denial(armor,identify(armor,43,42,altered_armor,policy),Reason::target);
    altered_armor=armor_reveal;altered_armor.reviewed_armor->physical=0;
    denial(armor,identify(armor,43,42,altered_armor,policy),Reason::target);
    auto unrated=armor;unrated.items.at(42).reviewed_armor.reset();
    denial(unrated,identify(unrated,43,42,armor_reveal,policy),Reason::target);
    auto stacked=p;stacked.items.at(43).definition.quantity=2;denial(stacked,identify(stacked,43,42,reveal,policy),Reason::quantity);
    auto unknown=p;unknown.items.at(43).definition.modifiers.pop_back();denial(unknown,identify(unknown,43,42,reveal,policy),Reason::charges);
    auto duplicate=p;duplicate.items.at(43).definition.modifiers.push_back(0x24580020);denial(duplicate,identify(duplicate,43,42,reveal,policy),Reason::charges);
    auto zero=p;zero.items.at(43).definition.modifiers[1]=0x24580000;denial(zero,identify(zero,43,42,reveal,policy),Reason::charges);
    auto wrong=p;wrong.items.at(43).definition.modifiers[0]=0x25e80200;denial(wrong,identify(wrong,43,42,reveal,policy),Reason::charges);
    auto already=p;already.items.at(42).definition.interaction|=1;denial(already,identify(already,43,42,reveal,policy),Reason::already_identified);
    auto exhausted=p;exhausted.items.at(43).definition.modifiers[1]=0x24580001;
    auto last=identify(exhausted,43,42,reveal,policy);
    check(last.accepted()&&last.exhausted&&last.uses_before==1&&last.uses_after==0,"last charge state incorrect");
    check(last.profile.items.size()==1&&!last.profile.items.contains(43)&&last.profile.next_item==44,"exhaustion fails owned conservation");
    native("kit332-detach",last.packets[0]);native("kit348-delete",last.packets[1]);
    check(last.packets.size()==5&&last.profile.items.contains(42),"target lost while exhausting kit");
    denial(last.profile,identify(last.profile,43,42,last.profile.items.at(42),policy),Reason::missing);
    // Equipped target remains in its actual slot and all four memberships.
    auto equipped=p;equipped.items.at(42).bag=2;equipped.items.at(42).slot=0;equipped.weapons.sets[0].weapon=42;equipped.weapons.sets[2].weapon=42;
    auto equipped_reveal=reveal;equipped_reveal.bag=2;equipped_reveal.slot=0;
    auto result=identify(equipped,43,42,equipped_reveal,policy);
    check(result.accepted()&&result.profile.weapons==equipped.weapons&&result.profile.items.at(42).bag==2,"equipped target references changed");
    check(result.packets.size()==5,"same-ID352 sends unnecessary replacement/placement");
    // Installed bag definitions are owned without a physical placement; they
    // must be rejected cleanly, never dereferenced as a nonexistent bag0.
    auto installed=p;installed.next_item=45;installed.bags.at(1).container=44;
    auto bag_item=p.items.at(42);bag_item.definition.id=44;bag_item.definition.type=3;bag_item.bag=0;bag_item.slot=0;
    installed.items.emplace(44,bag_item);
    auto bag_reveal=bag_item;bag_reveal.definition.interaction|=1;
    denial(installed,identify(installed,43,44,bag_reveal,policy),Reason::target);
    denial(installed,identify(installed,44,42,reveal,policy),Reason::source);
    // Storage targets require actual caller-granted access. The consumed kit
    // remains carried; storage never becomes a source kit implicitly.
    auto bank=p;bank.bags.emplace(3,inventory::Bag{3,4,7,20,0});bank.items.at(42).bag=3;
    auto bank_reveal=reveal;bank_reveal.bag=3;
    denial(bank,identify(bank,43,42,bank_reveal,policy),Reason::target);
    auto bank_result=identify(bank,43,42,bank_reveal,{true,true,true},inventory::Bounds::combined_transfer);
    check(bank_result.accepted()&&bank_result.profile.items.at(42).bag==3&&bank_result.profile.items.at(43).definition.quantity==1,"authorized storage target changes possession placement");
    auto bank_source=p;bank_source.bags.emplace(3,inventory::Bag{3,4,7,20,0});bank_source.items.at(43).bag=3;
    denial(bank_source,identify(bank_source,43,42,reveal,{true,true,true}),Reason::source);
    std::cout<<"PASS identification native bytes, immutable planning, exact charge/exhaustion and authoritative reveal boundaries\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

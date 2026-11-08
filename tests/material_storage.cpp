#include "gw_material_storage.h"
#include "gw_inventory.h"
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

using namespace fw::gw;
namespace material=inventory::materials;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static items::Packet unhex(const std::string& text){
    check(!(text.size()%2),"fixture framing");items::Packet output;
    for(unsigned at=0;at<text.size();at+=2){unsigned value=0;std::istringstream stream(text.substr(at,2));stream>>std::hex>>value;check(!stream.fail(),"fixture hex");output.push_back(static_cast<std::uint8_t>(value));}return output;
}
static items::Packet native(items::Packet packet){check(packet.size()>=2,"fixture opcode");packet[1]&=0x7f;return packet;}
static inventory::Item item(unsigned id,unsigned model,unsigned count){
    const auto row=material::slot_for_model(model);check(bool(row),"test material model");
    inventory::Item value;value.definition.id=id;value.definition.file=1234;value.definition.type=11;
    value.definition.model=model;value.definition.quantity=count;value.definition.interaction=0x80000;
    value.definition.modifiers={0x25080000u|(row->element<<8)};value.bag=2;value.slot=0;return value;
}
int main(int argc,char** argv){try{
    check(argc==2,"material native fixture path required");std::ifstream input(argv[1]);check(bool(input),"material fixture unavailable");
    std::map<std::string,items::Packet> fixtures;for(std::string line;std::getline(input,line);){std::istringstream stream(line);std::string label,hex;stream>>label>>hex;if(!label.empty())fixtures.emplace(label,unhex(hex));}
    check(material::bag_2022(1,4)==native(fixtures.at("material318")),"material bag type/index/slot array size match native serializer");
    check(items::quantity_2022(42,2500)==native(fixtures.at("quantity2500")),"material quantities above ordinary250 match original serializer");
    check(items::location_2022(1,42,4,24)==native(fixtures.at("material317")),"Diamond occupies material element24 instead of display21");
    inventory::Request request;check(inventory::parse(fixtures.at("cmsg112"),request)==world::FrameState::complete&&request.item==42&&request.bag==4&&request.slot==0&&request.change==inventory::Change::move&&request.consumed==9,"original whole-stack move uses resolved bagID WORD and slot BYTE");
    const auto split=fixtures.at("cmsg115");check(inventory::parse(split,request)==world::FrameState::complete&&request.item==42&&request.quantity==2500&&request.bag==4&&request.slot==0&&request.change==inventory::Change::split&&request.consumed==13,"original partial-material transfer retains full DWORD quantity2500");
    for(unsigned size=0;size<split.size();++size){request.item=777;request.consumed=888;check(inventory::parse(std::span(split).first(size),request)==world::FrameState::incomplete&&request.item==777&&request.consumed==888,"fragmented material request does not overwrite previous parse result");}
    check(inventory::parse(fixtures.at("cmsg110"),request)==world::FrameState::unknown_opcode,"unclaimed110 is not guessed to be automatic material deposit");
    std::set<unsigned> models,elements,displays;
    for(const auto& row:material::rows){
        check(models.insert(row.model).second&&elements.insert(row.element).second&&displays.insert(row.display).second,"reviewed table has unique model/element/display mappings");
        auto source=item(1,row.model,250);const auto classified=material::element(source.definition);
        check(classified&&classified->element==row.element&&classified->display==row.display&&material::crafting_slot(row.element),"all36 material modifiers agree with reviewed models");
        source.definition.modifiers[0]^=0x100;check(!material::element(source.definition),"wrong-element material modifier rejected");
        source.definition.modifiers={0x25080000u|(row.element<<8),0x25080000u|(row.element<<8)};check(!material::element(source.definition),"duplicate material properties rejected");
        source.definition.modifiers={0x25090000u|(row.element<<8)};check(!material::element(source.definition),"broader native selector cannot reclassify unreviewed modifier kind");
    }
    for(unsigned index:{7u,28u,38u,39u,40u,41u,UINT32_MAX})check(!material::crafting_slot(index),"gaps and Zaishen coins excluded from crafting storage");
    check(!material::slot_for_model(1)&&!material::slot_for_model(UINT32_MAX),"unknown material model not invented");
    check(material::slot_for_model(921)->element==0&&material::slot_for_model(921)->display==7&&material::slot_for_model(935)->element==24&&material::slot_for_model(935)->display==21,"element/visual permutation remains explicit");
    for(unsigned capacity=250;capacity<=2500;capacity+=250)check(material::valid_capacity(capacity),"reviewed expansion increments admitted");
    for(unsigned capacity:{0u,249u,251u,2499u,2501u,65535u,65536u,UINT32_MAX})check(!material::valid_capacity(capacity),"invalid or overflow quota denied");

    auto source=item(42,921,250),destination=item(43,921,220);
    const auto source_before=items::definition_2022(source.definition),destination_before=items::definition_2022(destination.definition);
    const auto partial=material::transfer(source,destination,250,{250,250});
    check(partial.accepted()&&partial.element==0&&partial.moved==30&&partial.source_after==220&&partial.destination_after==250,"base quota merge moves only30 and preserves surplus");
    check(items::definition_2022(source.definition)==source_before&&items::definition_2022(destination.definition)==destination_before,"proposal leaves both input definitions unchanged until CAS");
    const auto strict=material::transfer(source,destination,250,{250,250},false);
    check(strict.status==material::Reason::full&&!strict.moved&&strict.source_after==250&&strict.destination_after==220,"strict transfer cannot partially consume items");
    source.definition.quantity=250;destination.definition.quantity=2250;
    const auto upgraded=material::transfer(source,destination,250,{250,2500});check(upgraded.accepted()&&upgraded.moved==250&&!upgraded.source_after&&upgraded.destination_after==2500,"explicit reviewed capacity2500 merges full carried stack");
    check(material::transfer(source,destination,250,{250,250}).status==material::Reason::quantity,"wrong account quota does not mint capacity");
    source.definition.quantity=2500;
    const auto withdrawal=material::transfer(source,{},2500,{2500,250});check(withdrawal.accepted()&&withdrawal.moved==250&&withdrawal.source_after==2250&&withdrawal.destination_after==250,"withdrawal to one empty ordinary slot respects250 cap");
    check(material::transfer(source,{},2500,{2500,250},false).status==material::Reason::full,"all-or-none withdrawal rejects surplus without discarding it");
    const auto requested=material::transfer(source,{},125,{2500,250},false);check(requested.accepted()&&requested.moved==125&&requested.source_after==2375&&requested.destination_after==125,"exact partial request conserves all2500 units");
    check(material::transfer(source,{},0,{2500,250}).status==material::Reason::quantity&&material::transfer(source,{},2501,{2500,250}).status==material::Reason::quantity,"zero and excess requests rejected");
    check(material::transfer(source,{},2500,{UINT32_MAX,250}).status==material::Reason::capacity,"malformed quota rejects before arithmetic");
    auto same=source;same.definition.quantity=100;check(material::transfer(source,same,1,{2500,250}).status==material::Reason::identity,"same wireID cannot serve as both ownership sides");
    source=item(42,935,250);destination=item(43,921,100);check(material::transfer(source,destination,1,{250,250}).status==material::Reason::incompatible,"different crafting materials cannot swap or merge in designated material slot");
    destination=item(43,935,100);destination.definition.file++;check(material::transfer(source,destination,1,{250,250}).status==material::Reason::incompatible,"unreviewed asset variation remains exact instead of guessed equivalence");
    destination=item(43,935,100);destination.profession=1;check(material::transfer(source,destination,1,{250,250}).status==material::Reason::incompatible,"profession metadata cannot disappear on merge");
    destination=item(43,935,100);destination.customized=u"Owner";check(material::transfer(source,destination,1,{250,250}).status==material::Reason::incompatible,"customization metadata cannot disappear on merge");
    source.profession=12;check(material::transfer(source,{},1,{250,250}).status==material::Reason::definition,"out-of-schema profession rejected");source.profession.reset();
    source.definition.type=3;check(material::transfer(source,{},1,{250,250}).status==material::Reason::material,"installed bag is not a crafting material");source=item(42,935,250);
    source.definition.interaction=0;check(material::transfer(source,{},1,{250,250}).status==material::Reason::material,"unstackable material-shaped definition is not split");source=item(42,935,250);
    source.definition.modifiers.push_back(0xc0000000);check(material::transfer(source,{},1,{250,250}).status==material::Reason::definition,"native sentinel is never an accepted wire modifier");

    // Check actual conserved count proposals across every quota and many
    // boundary/interior quantities, without reimplementing the planner.
    std::uint32_t random=218733;
    for(unsigned iteration=0;iteration<5000;++iteration){
        random=random*1664525u+1013904223u;const unsigned source_limit=250*(1+random%10);random=random*1664525u+1013904223u;const unsigned destination_limit=250*(1+random%10);
        random=random*1664525u+1013904223u;source=item(1,material::rows[iteration%36].model,1+random%source_limit);
        random=random*1664525u+1013904223u;const unsigned previous=random%(destination_limit+1);std::optional<inventory::Item> existing;if(previous)existing=item(2,source.definition.model,previous);
        random=random*1664525u+1013904223u;const unsigned quantity=1+random%source.definition.quantity;
        const auto proposed=material::transfer(source,existing,quantity,{source_limit,destination_limit});
        if(previous==destination_limit)check(proposed.status==material::Reason::full&&!proposed.moved,"full quota denies property case");
        else check(proposed.accepted()&&proposed.moved&&proposed.moved<=quantity&&proposed.source_after<source.definition.quantity&&
                   proposed.destination_after<=destination_limit&&proposed.source_after+proposed.destination_after==source.definition.quantity+previous,"conservation/limits for all quota levels and material identities");
    }
    std::cout<<"material storage tests passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

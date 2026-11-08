#include "gw_creation_equipment.h"
#include "gw_creation_armor.h"
#include <iostream>
using namespace fw::gw::creation;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
unsigned opcode(const std::vector<std::uint8_t>& p){return p[0]|(unsigned(p[1])<<8);}
int main()try{
    std::array<std::uint8_t,9> colors{};colors.fill(7);
    for(unsigned profession=1;profession<=6;++profession){
        auto armor=prophecies_armor_preview(profession,colors);
        check(armor.size()==4,"four verified body armor slots");
        const auto summary=summary_equipment_2022(armor);
        check(summary.size()==4&&summary[0].composite==armor[0].definition.file&&summary[0].color==7,
            "summary preserves body armor and dye");
        Equipment preview;auto batch=preview.replace(armor);
        check(batch.size()==14,"complete initial armor batch");
        Equipment existing_inventory(true);auto existing_batch=existing_inventory.replace(armor);
        check(existing_batch.size()==13&&opcode(existing_batch[0])==318,"existing inventory is not initialized twice");
        for(const auto& item:armor)check(item.definition.dye==7&&item.definition.model==0,"preview preserves dye without fabricating gameplay model");
    }
    Equipment session,other;
    for(unsigned campaign=2;campaign<=3;++campaign){
        for(unsigned profession=1;profession<=10;++profession){
            const bool supported=campaign==2?profession<=8:(profession<=6||profession>=9);
            bool rejected=false;
            try{
                auto armor=campaign_armor_preview(campaign,profession,colors);
                check(supported,"unavailable campaign class cannot receive starter art");
                Equipment preview;auto batch=preview.replace(armor);
                check(armor.size()==4&&batch.size()==14,"complete campaign preview ordering");
                for(const auto& piece:armor)check(piece.definition.model==0&&piece.definition.dye==7,
                    "campaign preview never fabricates gameplay inventory");
                if(campaign==3&&profession==1)check(armor[0].definition.file==0x541&&armor[0].definition.unknown1==19,
                    "reviewed Istani warrior chest file and tint");
                if(campaign==2&&profession==7)check(armor[0].definition.file==0x3b2&&armor[0].definition.unknown1==11,
                    "reviewed Shing Jea assassin chest file and tint");
            }catch(const std::invalid_argument&){rejected=true;}
            check(rejected==!supported,"invalid campaign profession is explicit");
        }
    }
    std::array<StarterItem,4> summary_items{{{5,{}},{4,{}},{2,{}},{0,{}}}};
    for(auto& item:summary_items){item.definition.file=100+item.slot;item.definition.interaction=0x20000004;}
    summary_items[0].definition.unknown1=19;
    summary_items[2].definition.interaction=4;
    const auto extracted=summary_equipment_2022(summary_items);
    check(extracted.size()==3&&extracted[0].composite==104&&extracted[1].composite==102&&extracted[2].composite==105,
        "retail slot order excludes weapons");
    check(extracted[0].parameter==43&&extracted[1].parameter==0&&extracted[2].parameter==19,
        "retail default tint and interaction gating");
    std::array<StarterItem,2> catalog{{{4,{0,1234}},{2,{0,5678}}}};
    auto initial=session.replace(catalog);
    const std::array<unsigned,8> order{323,318,352,352,330,330,348,348};
    check(initial.size()==order.size(),"initial packet count");
    for(std::size_t i=0;i<order.size();++i)check(opcode(initial[i])==order[i],"definition/placement/event order");
    check(session.slots()[4]==1&&session.slots()[2]==2,"transient item IDs assigned");
    auto invalid=catalog;invalid[1].slot=4;
    bool rejected=false;try{session.replace(invalid);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected&&session.slots()[2]==2&&session.slots()[4]==1,"invalid replacement preserves state");
    catalog[0].slot=5;auto updated=session.replace(catalog);
    check(updated.size()==8&&opcode(updated[0])==352,"replacement omits inventory reset and includes removal");
    check(opcode(updated[2])==348&&opcode(updated[3])==348&&opcode(updated[4])==330,
        "occupied slots removed before replacement placement");
    check(updated[2][2]==0&&updated[3][2]==0,"removal event does not introduce replacement early");
    check(session.slots()[5]==3&&session.slots()[4]==0,"replacement removes old slot and reuses no IDs");
    auto isolated=other.replace(catalog);
    check(other.slots()[5]==1&&session.slots()[5]==3,"preview inventories are isolated");
    std::cout<<"PASS creation equipment ordering, replacement, rollback and session isolation\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

#include "network.h"
#include "network_storage_adapter.h"
#include "common.h"
#include <algorithm>
#include <thread>
#include <iostream>
#include <fstream>
#include <sodium.h>
#include "framework/module.h"
#include "gw_token.h"
#include "gw_character_profile.h"
#include "gw_character_skills.h"
#include "gw_attributes.h"
#include "gw_summary_2022.h"
#include "gw_basic_armor_energy.h"
using namespace fw;
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::string password(const std::string& value) {
    std::string out(value.size() * 2 + 1, '\0');
    sodium_bin2hex(out.data(), out.size(), reinterpret_cast<const unsigned char*>(value.data()), value.size());
    out.pop_back(); return out;
}
int main(int argc, char** argv) try {
    check(argc == 4, "database path, phase and test session file required");
    check(sodium_init() >= 0, "sodium initialization");
    gwserver::NetworkManager manager(argv[1]); TestStorageAdapter adapter(manager);
    Accounts storage(argv[1]);
    unsigned call_index=0;
    auto call = [&](const std::string& op, const std::string& payload, uint32_t expected = FW_OK) {
        ++call_index;
        // Public application operations exercise the manager boundary. Import
        // and maintenance operations exercise storage directly, since they
        // are deliberately absent from the client/server operation contract.
        std::optional<gwserver::Operation> operation;
        for(unsigned n=0;n<=static_cast<unsigned>(gwserver::Operation::purchaseGuildTrim);++n){
            auto candidate=static_cast<gwserver::Operation>(n);
            if(gwserver::storage_operation(candidate)==op){operation=candidate;break;}
        }
        auto reply=operation?adapter.request(*operation,payload,3000):storage.execute(op,payload,[]{return true;});
        auto code=reply.status; auto result=std::move(reply.payload);
        if (code != expected) {
            auto message=op + " call " + std::to_string(call_index) + " returned " +
                std::to_string(code) + " expected " + std::to_string(expected);
            if(op=="gw-character-profile-import-utf8")message+=" detail="+result;
            throw std::runtime_error(message);
        }
        return result;
    };
    auto original = password("synthetic-account-password"), replacement = password("replacement-account-password");
    if (std::string(argv[2]) == "seed") {
        auto alice = split(call("register", "Alice\t" + original));
        check(alice.size() == 4 && alice[1] == "alice" && alice[2].size() == 64, "registration identity");
        check(call("gw-account-game-alias-get",alice[0])=="alice","new account defaults its game alias to the login name");
        check(call("gw-account-game-alias-set",alice[0]+"\tgw1")=="gw1"&&
              call("gw-account-game-alias-get",alice[0])=="gw1","captured game-account alias persists separately");
        check(call("gw-account-eula-get",alice[0])=="1","private-server agreement bypass persists for new accounts");
        check(call("gw-account-campaigns-get",alice[0])=="1,2,3,4","private development accounts receive all campaigns and expansion");
        {
            // The native name frame bounds UTF-16 units, not UTF-8 bytes.
            const auto name=gw::character_name_to_utf8(std::u16string(19,u'\uac00'));
            const auto created=split(call("gw-character-create",alice[0]+"\t"+name));
            check(created.size()==3&&created[1]==name,"creation preserves multibyte name at native unit bound");
            call("gw-character-delete",alice[0]+"\t"+created[0]);
            const auto before=call("gw-character-roster-page",alice[0]+"\t0");
            const auto too_long=gw::character_name_to_utf8(std::u16string(20,u'\uac00'),20);
            call("gw-character-create",alice[0]+"\t"+too_long,FW_INVALID);
            call("gw-character-create",alice[0]+"\t"+std::string("\xc0\xaf",2),FW_INVALID);
            call("gw-character-create",alice[0]+"\tBad/Name",FW_INVALID);
            check(call("gw-character-roster-page",alice[0]+"\t0")==before,"invalid UTF or disallowed ASCII leaves roster unchanged");
        }
        {
            const auto capacity_owner=split(call("register","CreationCapacity\t"+original))[0];
            std::vector<std::string> characters;
            for(unsigned index=0;index<8;++index)
                characters.push_back(split(call("gw-character-create",capacity_owner+"\tCapacity Hero "+std::to_string(index)))[0]);
            const auto before=call("gw-character-roster-page",capacity_owner+"\t0");
            call("gw-character-create",capacity_owner+"\tCapacity Overflow",FW_OVERLOADED);
            call("gw-character-create",capacity_owner+"\tBad/Name",FW_INVALID);
            check(call("gw-character-roster-page",capacity_owner+"\t0")==before,"capacity and invalid-name rejections preserve roster");
            call("gw-character-delete",capacity_owner+"\t"+characters.back());
            const auto retry=split(call("gw-character-create",capacity_owner+"\tCapacity Overflow"));
            check(retry.size()==3,"creation can retry after releasing a roster slot");
        }
        call("gw-account-eula-set",alice[0]+"\t1");
        check(call("gw-account-eula-get",alice[0])=="1","private-server agreement bypass persists per account");
        call("gw-account-eula-get","invalid",FW_INVALID);
        call("register", "ALICE\t" + original, FW_CONFLICT);
        call("login", "alice\t" + password("wrong-password-test"), FW_NOT_FOUND);
        call("login", "missing\t" + original, FW_NOT_FOUND);
        call("register", "bad/name\t" + original, FW_INVALID);
        call("register", "short\t00", FW_INVALID);
        auto bob = split(call("register", "Bob\t" + original));
        call("gw-account-game-alias-set",bob[0]+"\tgw1",FW_CONFLICT);
        call("gw-account-game-alias-set",bob[0]+"\tinvalid alias",FW_INVALID);
        // Distinct captures stay bound to their own account, with atomic
        // rejection of malformed replacement data.
        std::vector<std::uint8_t> main_profile(40,0),alt_profile(41,0);
        for(auto* profile:{&main_profile,&alt_profile}) {
            (*profile)[0]='F';(*profile)[1]='W';(*profile)[2]='A';(*profile)[3]='P';
            (*profile)[32]=static_cast<std::uint8_t>(profile->size()-39);
            (*profile)[profile->size()-5]=26;
        }
        main_profile[4]=1;main_profile[34]=0xa5;alt_profile[4]=2;alt_profile[34]=0x5a;
        auto blob_hex=[](const auto& bytes){std::string out(bytes.size()*2+1,'\0');
            sodium_bin2hex(out.data(),out.size(),bytes.data(),bytes.size());out.pop_back();return out;};
        const auto main_hex=blob_hex(main_profile),alt_hex=blob_hex(alt_profile);
        check(call("gw-account-profile-get",alice[0])=="-","unimported account has no captured profile");
        call("gw-account-profile-set",alice[0]+"\t"+main_hex);
        call("gw-account-profile-set",bob[0]+"\t"+alt_hex);
        check(call("gw-account-profile-get",alice[0])==main_hex && call("gw-account-profile-get",bob[0])==alt_hex,
              "captured account flags remain separate");
        call("gw-account-profile-set",alice[0]+"\t"+main_hex+"00",FW_INVALID);
        check(call("gw-account-profile-get",alice[0])==main_hex,"invalid replacement preserves stored capture");
        call("gw-account-profile-get",std::string(32,'f'),FW_NOT_FOUND);
        auto guid = [](const std::string& raw) {
            gw::GameToken id{};
            check(sodium_hex2bin(id.data(), id.size(), raw.data(), raw.size(), nullptr, nullptr, nullptr) == 0, "account id decode");
            return gw::format_token(id);
        };
        auto bound_ticket = split(call("gw-ticket-issue", alice[2]))[0];
        call("gw-ticket-authenticate", bound_ticket + "\t" + guid(bob[0]) + "\talice\tHero", FW_NOT_FOUND);
        call("gw-ticket-authenticate", bound_ticket + "\tbad\talice", FW_INVALID);
        auto empty_ticket=split(call("gw-ticket-issue",alice[2]))[0];
        auto empty_roster=split(call("gw-ticket-authenticate",empty_ticket+"\t"+guid(alice[0])+"\talice\t"));
        check(empty_roster.size()==8 && empty_roster[0]==alice[0] && empty_roster[3].empty() &&
              empty_roster[4].empty() && empty_roster[6]=="0",
              "ticket authentication preserves an empty roster without inventing a character");
        auto unknown_character_ticket=split(call("gw-ticket-issue",alice[2]))[0];
        call("gw-ticket-authenticate",unknown_character_ticket+"\t"+guid(alice[0])+"\talice\tHero",FW_NOT_FOUND);
        {
            fw::gw::CharacterProfile created_profile;
            created_profile.unknown1=6;created_profile.extension_count=37;created_profile.last_map_id=35;
            created_profile.extended[3]=1u<<4;
            const auto bytes=created_profile.encode();
            const auto hex=password(std::string(reinterpret_cast<const char*>(bytes.data()),bytes.size()));
            const auto before=call("gw-character-roster-page",alice[0]+"\t0");
            call("gw-character-create",alice[0]+"\tProfile Hero\t00",FW_INVALID);
            auto malformed=bytes;malformed[6]=8;
            call("gw-character-create",alice[0]+"\tProfile Hero\t"+
                password(std::string(reinterpret_cast<const char*>(malformed.data()),malformed.size())),FW_INVALID);
            check(call("gw-character-roster-page",alice[0]+"\t0")==before,"invalid creation profile leaves roster unchanged");
            for(unsigned invalid_primary:{0u,11u,15u}){
                auto invalid=created_profile;invalid.extended[3]=static_cast<std::uint16_t>(invalid_primary<<4);
                const auto invalid_bytes=invalid.encode();
                call("gw-character-create",alice[0]+"\tInvalid Profession\t"+
                    password(std::string(reinterpret_cast<const char*>(invalid_bytes.data()),invalid_bytes.size())),FW_INVALID);
                check(call("gw-character-roster-page",alice[0]+"\t0")==before,"invalid profession creation is atomic");
            }
            auto saved=split(call("gw-character-create",alice[0]+"\tProfile Hero\t"+hex));
            check(saved.size()==3&&saved[2]=="35","profile creation uses supplied map");
            const auto roster=split(call("gw-character-roster-page",alice[0]+"\t0"));
            check(roster.size()==6&&roster[2]==saved[0]&&roster[5]==hex,"creation atomically stores retail profile");
            call("gw-character-create",alice[0]+"\tProfile Hero\t"+hex,FW_CONFLICT);
            check(split(call("gw-character-roster-page",alice[0]+"\t0"))[5]==hex,"duplicate creation preserves original profile");
            call("gw-character-delete",alice[0]+"\t"+saved[0]);
        }
        for(const unsigned map:{212u,490u}){
            gw::CharacterProfile profile;profile.unknown1=6;profile.extension_count=37;profile.last_map_id=static_cast<std::uint16_t>(map);
            profile.extended[3]=1u<<4;
            const auto raw=profile.encode();const auto encoded=password(std::string(reinterpret_cast<const char*>(raw.data()),raw.size()));
            const auto created=split(call("gw-character-create",alice[0]+"\tCampaign Hero\t"+encoded));
            check(created.size()==3&&created[2]==std::to_string(map),"campaign creation destination persisted");
            check(split(call("gw-character-roster-page",alice[0]+"\t0"))[5]==encoded,"campaign creation summary persisted");
            call("gw-character-delete",alice[0]+"\t"+created[0]);
        }
        {
            gw::Summary2022 summary;summary.map=148;summary.level=1;summary.low_nibble=1;summary.appearance=1u<<20;
            summary.equipment={{91,7,19},{92,7,19},{94,7,19},{90,7,19}};
            const auto raw=summary.profile().encode();
            const auto encoded=password(std::string(reinterpret_cast<const char*>(raw.data()),raw.size()));
            const auto before=call("gw-character-roster-page",alice[0]+"\t0");
            unsigned checkpoints=0;
            Accounts cancellation_probe(argv[1]);
            const auto cancelled=cancellation_probe.execute("gw-character-create",alice[0]+"\tGear Hero\t"+encoded,[&]{return ++checkpoints<3;});
            check(cancelled.status==FW_TIMEOUT,"creation cancellation reaches inventory transaction");
            check(call("gw-character-roster-page",alice[0]+"\t0")==before,"cancelled inventory creation rolls back roster");
            const auto created=split(call("gw-character-create",alice[0]+"\tGear Hero\t"+encoded));
            const auto owned=alice[0]+"\t"+created[0];
            const auto skill_record=split(call("gw-character-skills-get",owned));
            check(skill_record.size()==2&&skill_record[0]=="1","creation persists profession before world entry");
            std::vector<std::uint8_t> skill_blob(skill_record[1].size()/2);
            check(!sodium_hex2bin(skill_blob.data(),skill_blob.size(),skill_record[1].data(),skill_record[1].size(),nullptr,nullptr,nullptr),"creation profession decode");
            const auto initial_skills=gw::skills::decode_profile(skill_blob);
            check(initial_skills.primary==1&&initial_skills.secondary==0&&initial_skills.profession_mask==0&&initial_skills.learned.empty(),"creation retains chosen profession without invented learned skills");
            call("gw-character-skills-get",bob[0]+"\t"+created[0],FW_NOT_FOUND);
            const auto stored=split(call("gw-character-inventory-get",owned));
            check(stored.size()==2&&stored[0]=="1","new character inventory exists before world login");
            std::vector<std::uint8_t> blob(stored[1].size()/2);
            check(!sodium_hex2bin(blob.data(),blob.size(),stored[1].data(),stored[1].size(),nullptr,nullptr,nullptr),"creation gear decode");
            const auto gear=gw::inventory::decode(blob);
            check(gear.items.size()==5&&gear.next_item==6,"new character owns armor and captured starter weapon");
            unsigned armor_count=0,weapon_count=0;
            for(const auto& [id,item]:gear.items){
                if(item.slot==0){++weapon_count;check(item.definition.file==39776&&item.definition.type==15&&gear.weapons.sets[0].weapon==id,"creation persists equipped Starter Hammer");}
                else{++armor_count;check(item.reviewed_armor&&item.reviewed_armor->base==25&&item.reviewed_armor->physical==20,"level-one Prophecies warrior receives starter armor stats");}
            }
            check(armor_count==4&&weapon_count==1,"creation armor/weapon cardinality changed");
            for(const auto& [id,item]:gear.items)check(item.profession==1&&item.customized==u"Gear Hero"&&item.bag==2,"creation equipment owner and profession");
            call("gw-character-inventory-get",bob[0]+"\t"+created[0],FW_NOT_FOUND);
            call("gw-character-delete",owned);
            call("gw-character-inventory-get",owned,FW_NOT_FOUND);
            call("gw-character-skills-get",owned,FW_NOT_FOUND);
        }
        auto created_hero=split(call("gw-character-create",alice[0]+"\tHero"));
        check(call("gw-character-experience-get",alice[0]+"\t"+created_hero[0])=="0","native characters start with known zero experience");
        call("gw-character-experience-get",bob[0]+"\t"+created_hero[0],FW_NOT_FOUND);
        call("gw-character-experience-get",alice[0]+"\tinvalid",FW_INVALID);
        fw::gw::CharacterProfile character_profile;
        character_profile.unknown0=0x12345678;character_profile.extension_count=62;
        character_profile.unknown1=8;character_profile.last_map_id=148;
        for(std::size_t i=0;i<character_profile.extended.size();++i)character_profile.extended[i]=static_cast<std::uint16_t>(i+1);
        auto profile_blob=character_profile.encode();
        auto profile_hex=password(std::string(reinterpret_cast<const char*>(profile_blob.data()),profile_blob.size()));
        const std::string unicode_character="\xC3\x89" "lan";
        auto unicode_import=split(call("gw-character-profile-import-utf8","bob\t"+
                                       password(unicode_character)+"\t"+profile_hex));
        check(unicode_import.size()==3&&unicode_import[1]==unicode_character,
              "ASCII-safe character profile import preserves UTF-8 names");
        auto unicode_bob_ticket=split(call("gw-ticket-issue",bob[2]))[0];
        auto unicode_bob_identity=guid(bob[0]);
        auto unicode_roster=split(call("gw-ticket-authenticate",unicode_bob_ticket+"\t"+unicode_bob_identity+
                                       "\tbob\t"+unicode_character));
        check(unicode_roster.size()==8&&unicode_roster[4]==unicode_character&&unicode_roster[6]=="1",
              "ticket authentication returns a bounded identity envelope");
        auto unicode_page=call("gw-character-roster-page",bob[0]+"\t0");
        auto unicode_page_fields=split(unicode_page);
        check(unicode_page.size()<=FW_MAX_PAYLOAD,"Unicode roster page stays under RPC payload bound");
        check(unicode_page_fields.size()==6,"single-character page has bounded response shape");
        check(unicode_page_fields[3]==unicode_character,"UTF-8 character name survives roster-page transport");
        check(unicode_page_fields[5]==profile_hex,"opaque profile survives roster-page transport");
        auto ordered_account=split(call("register","capture-order\t"+original));
        const std::string captured_uuid="2f1b9919-fc32-41d8-9285-b563801bef84";
        const std::string selected_uuid="8828a081-e5d3-47a7-aaff-b4e92b640438";
        auto imported_zed=split(call("gw-character-profile-import-utf8","capture-order\t"+password("Zed")+
                                     "\t"+profile_hex+"\t0\t"+captured_uuid));
        call("gw-character-profile-import-utf8","capture-order\t"+password("Alice")+
             "\t"+profile_hex+"\t1\t"+selected_uuid);
        auto ordered_page=split(call("gw-character-roster-page",ordered_account[0]+"\t0"));
        check(ordered_page.size()==10&&ordered_page[3]=="Zed"&&ordered_page[7]=="Alice",
              "captured roster order is preserved instead of alphabetically rewritten");
        std::array<std::uint8_t,16> imported_id{};
        check(ordered_page[2].size()==32&&
              sodium_hex2bin(imported_id.data(),imported_id.size(),ordered_page[2].data(),ordered_page[2].size(),nullptr,nullptr,nullptr)==0&&
              fw::gw::format_token(imported_id)==captured_uuid&&imported_zed[0]==ordered_page[2],
              "captured character UUID is preserved in client-visible GUID byte order");
        check(split(call("gw-character-profile-import-utf8","capture-order\t"+password("Zed")+
                         "\t"+profile_hex+"\t0\t"+captured_uuid))[0]==ordered_page[2],
              "reimport keeps the captured character identity stable");
        call("gw-character-profile-import-utf8","bob\t"+password("Other")+"\t"+profile_hex+
             "\t0\t"+captured_uuid,FW_CONFLICT);
        call("gw-character-profile-import-utf8","bob\t"+password("Other")+"\t"+profile_hex+
             "\t0\tnot-a-uuid",FW_INVALID);
        auto ordered_ticket=split(call("gw-ticket-issue",ordered_account[2]))[0];
        auto ordered_identity=split(call("gw-ticket-authenticate",ordered_ticket+"\t"+guid(ordered_account[0])+"\tcapture-order\tAlice"));
        check(ordered_identity.size()==8&&ordered_identity[4]=="Alice"&&ordered_identity[6]=="2",
              "character selection uses the captured name while retaining ordered roster data");
        auto imported=split(call("gw-character-profile-import","alice\tHero\t"+profile_hex));
        check(call("gw-character-experience-get",alice[0]+"\t"+created_hero[0])=="-","import clears uncaptured experience");
        check(imported.size()==3&&imported[0]==created_hero[0]&&imported[2]=="148","capture profile applies to the matching local character");
        auto hero_ticket=split(call("gw-ticket-issue",alice[2]))[0];
        auto authenticated = split(call("gw-ticket-authenticate", hero_ticket + "\t" + guid(alice[0]) + "\tbob\tHero"));
        check(authenticated.size() == 8 && authenticated[0] == alice[0] && authenticated[1] == "alice" && authenticated[3].size() == 32 && authenticated[4] == "Hero" && authenticated[5] == "148" && authenticated[6] == "1", "bound ticket returns compact selected identity");
        auto hero_roster=split(call("gw-character-roster-page",alice[0]+"\t0"));
        check(hero_roster.size()==6&&hero_roster[2]==authenticated[3]&&hero_roster[3]=="Hero"&&hero_roster[4]=="148"&&hero_roster[5]==profile_hex,
              "selected opaque character summary is fetched independently from identity");
        const auto position_key=alice[0]+"\t"+authenticated[3];
        check(call("gw-character-position-get",position_key)=="148\t0.000000\t0.000000","default Prophecies character position");
        call("gw-character-position-set",position_key+"\t148\t120.5\t-32.25");
        check(call("gw-character-position-get",position_key)=="148\t120.500000\t-32.250000","persistent character position update");
        call("gw-character-position-set",position_key+"\t148\t10001\t0",FW_INVALID);
        call("gw-character-position-get",bob[0]+"\t"+authenticated[3],FW_NOT_FOUND);
        call("gw-ticket-authenticate", hero_ticket + "\t" + guid(alice[0]) + "\talice\tHero", FW_NOT_FOUND);
        auto alternate_ticket = split(call("gw-ticket-issue", alice[2]))[0];
        call("gw-ticket-authenticate",alternate_ticket+"\t"+guid(alice[0])+"\talice\tMage",FW_NOT_FOUND);
        auto mage=split(call("gw-character-create",alice[0]+"\tMage"));
        check(mage.size()==3 && mage[1]=="Mage","explicit creation is required before selecting a character");
        auto selected_ticket=split(call("gw-ticket-issue",alice[2]))[0];
        auto alternate = split(call("gw-ticket-authenticate", selected_ticket + "\t" + guid(alice[0]) + "\talice\tMage"));
        check(alternate.size() == 8 && alternate[3] != authenticated[3] && alternate[4] == "Mage" && alternate[6] == "2", "multiple persistent characters returned in compact identity envelope");
        auto alternate_roster=split(call("gw-character-roster-page",alice[0]+"\t0"));
        check(alternate_roster.size()==10&&alternate_roster[2]==authenticated[3]&&alternate_roster[3]=="Hero"&&
              alternate_roster[5]==profile_hex&&alternate_roster[6]==alternate[3]&&alternate_roster[7]=="Mage"&&
              alternate_roster[8]=="148"&&alternate_roster[9]=="-", "multiple characters retain their ordered summaries in roster pages");
        auto stress_account=split(call("register","roster-stress\t"+original));
        call("gw-character-profile-import-utf8","roster-stress\t"+password("Probe Character")+"\t"+profile_hex,FW_INVALID);
        check(call("gw-character-roster-page",stress_account[0]+"\t0")=="0\t0",
              "placeholder profile import is rejected without creating a roster entry");
        const std::string cjk("\xE4\xB8\x80",3);
        std::vector<std::string> stress_names;
        for(unsigned i=0;i<8;++i) {
            auto name=std::string{};for(unsigned unit=0;unit<18;++unit)name+=cjk;name+=static_cast<char>('0'+i);
            stress_names.push_back(name);
            call("gw-character-profile-import-utf8","roster-stress\t"+password(name)+"\t"+profile_hex);
        }
        auto stress_ticket=split(call("gw-ticket-issue",stress_account[2]))[0];
        auto stress_identity=split(call("gw-ticket-authenticate",stress_ticket+"\t"+guid(stress_account[0])+"\troster-stress\t"+stress_names[0]));
        check(stress_identity.size()==8&&stress_identity[6]=="8","maximum roster authentication envelope is bounded");
        std::vector<std::string> paged_names;
        for(unsigned offset=0;offset<8;offset+=3) {
            const auto page_payload=call("gw-character-roster-page",stress_account[0]+"\t"+std::to_string(offset));
            const auto page=split(page_payload);
            const auto expected=std::min(3u,8u-offset);
            check(page_payload.size()<=FW_MAX_PAYLOAD&&page.size()==2+4*expected&&page[0]=="8"&&page[1]==std::to_string(expected),
                  "large Unicode profile roster page remains below the internal RPC payload limit");
            for(unsigned i=0;i<expected;++i) {
                const auto at=2+4*i;
                paged_names.push_back(page[at+1]);
                check(page[at].size()==32&&page[at+2]=="148"&&page[at+3]==profile_hex,
                      "paged Unicode profile record preserves identity, map and summary");
            }
        }
        check(paged_names==stress_names,"all eight imported Unicode character profiles are returned across stable roster pages");
        call("gw-character-roster-page",stress_account[0]+"\t1",FW_INVALID);
        call("gw-character-map-transition",alice[0]+"\t"+alternate[3]+"\t148\t35");
        check(call("gw-character-map-get",alice[0]+"\t"+alternate[3])=="35","map lookup follows world transition");
        call("gw-character-map-get",bob[0]+"\t"+alternate[3],FW_NOT_FOUND);
        call("gw-character-map-get","bad\t"+alternate[3],FW_INVALID);
        check(call("gw-character-position-get",alice[0]+"\t"+alternate[3])=="35\t0.000000\t0.000000","map transition resets spawn coordinates");
        call("gw-character-map-transition",alice[0]+"\t"+alternate[3]+"\t35\t0");
        check(call("gw-character-map-get",alice[0]+"\t"+alternate[3])=="0","map lookup preserves map zero");
        check(call("gw-character-position-get",alice[0]+"\t"+alternate[3])=="0\t0.000000\t0.000000","map zero position reads correctly");
        call("gw-character-position-set",alice[0]+"\t"+alternate[3]+"\t0\t120.5\t-32.25");
        call("gw-character-map-transition",alice[0]+"\t"+alternate[3]+"\t148\t36",FW_CONFLICT);
        call("gw-character-position-set",alice[0]+"\t"+alternate[3]+"\t148\t120.5\t-32.25",FW_CONFLICT);
        call("gw-character-delete",alice[0]+"\t"+alternate[3]);
        call("gw-character-map-get",alice[0]+"\t"+alternate[3],FW_NOT_FOUND);
        call("gw-character-position-get",alice[0]+"\t"+alternate[3],FW_NOT_FOUND);
        call("gw-character-delete",alice[0]+"\t"+alternate[3],FW_NOT_FOUND);
        call("gw-character-delete",bob[0]+"\t"+authenticated[3],FW_NOT_FOUND);
        auto created=split(call("gw-character-create",alice[0]+"\tNew Hero"));
        check(created.size()==3 && created[1]=="New Hero" && created[2]=="148","explicit character creation in Prophecies");
        call("gw-character-create",alice[0]+"\tNew Hero",FW_CONFLICT);
        const auto skill_owner=alice[0]+"\t"+created[0];
        check(call("gw-character-skills-get",skill_owner)=="0","uncaptured character skill profile absent");
        gw::skills::Profile skills;skills.primary=8;skills.secondary=7;skills.profession_mask=0x6fe;
        skills.learned={{3,81},{1239,1}};skills.bar[0]={3,80};skills.bar[1]={1239,0};
        const auto skill_hex=blob_hex(gw::skills::encode_profile(skills));
        check(call("gw-character-skills-import",skill_owner+"\t0\t"+skill_hex+"\t"+std::string(64,'a'))=="1","explicit owned skill import");
        check(call("gw-character-skills-get",skill_owner)=="1\t"+skill_hex,"durable captured learning exact");
        call("gw-character-skills-get",bob[0]+"\t"+created[0],FW_NOT_FOUND);
        call("gw-character-skills-import",skill_owner+"\t0\t"+skill_hex+"\t"+std::string(64,'a'),FW_CONFLICT);
        skills.bar[0]={};std::vector<std::uint8_t> bar;
        for(auto slot:skills.bar){gw::skills::dword(bar,slot.skill);gw::skills::dword(bar,slot.copy);}
        check(call("gw-character-bar-save",skill_owner+"\t1\t"+blob_hex(bar))=="2","accepted bar commits new revision");
        call("gw-character-bar-save",skill_owner+"\t1\t"+blob_hex(bar),FW_CONFLICT);
        check(call("gw-character-skills-get",skill_owner)=="2\t"+blob_hex(gw::skills::encode_profile(skills)),"stale session cannot overwrite latest bar");
        check(call("gw-character-attributes-get",skill_owner)=="0","attributes absent until explicit capture or baseline save");
        gw::attributes::Profile attrs;const auto attr_hex=blob_hex(attrs.encode());
        check(call("gw-character-attributes-save",skill_owner+"\t0\t"+attr_hex)=="1","owned baseline attribute state can be saved without capture");
        attrs.quest_bonus=30;call("gw-character-attributes-save",skill_owner+"\t1\t"+blob_hex(attrs.encode()),FW_INVALID);attrs.quest_bonus=0;
        auto selection_profile=character_profile;selection_profile.extended[12]=static_cast<std::uint16_t>((20u<<4)|(7u<<10)|0x8003u);
        call("gw-character-profile-import-utf8","alice\t"+password("New Hero")+"\t"+blob_hex(selection_profile.encode()));
        skills.secondary=3;
        call("gw-character-secondary-save",skill_owner+"\t2\t0\t"+blob_hex(gw::skills::encode_profile(skills))+"\t"+attr_hex,FW_CONFLICT);
        check(call("gw-character-skills-get",skill_owner).starts_with("2\t"),"attribute conflict rolls back secondary skill revision");
        check(call("gw-character-secondary-save",skill_owner+"\t2\t1\t"+blob_hex(gw::skills::encode_profile(skills))+"\t"+attr_hex)=="3\t2","secondary change commits skills and allocation atomically");
        bool secondary_summary_found=false;
        for(unsigned offset:{0u,3u,6u}){const auto page=split(call("gw-character-roster-page",alice[0]+"\t"+std::to_string(offset)));
            for(std::size_t at=2;at+3<page.size();at+=4)if(page[at]==created[0]){
                auto expected=selection_profile;expected.extended[12]=static_cast<std::uint16_t>((expected.extended[12]&~(15u<<10))|(3u<<10));
                check(page[at+3]==blob_hex(expected.encode()),"secondary roster update preserves every unrelated opaque byte");secondary_summary_found=true;}}
        check(secondary_summary_found,"secondary character remains in reconnect roster");
        call("gw-character-secondary-save",skill_owner+"\t2\t1\t"+blob_hex(gw::skills::encode_profile(skills))+"\t"+attr_hex,FW_CONFLICT);
        call("gw-character-delete",alice[0]+"\t"+created[0]);
        call("gw-character-skills-get",skill_owner,FW_NOT_FOUND);
        call("gw-character-attributes-get",skill_owner,FW_NOT_FOUND);
        call("gw-character-bar-save",skill_owner+"\t2\t"+blob_hex(bar),FW_NOT_FOUND);
        check(call("validate", alice[2]).starts_with(alice[0] + "\talice\t"), "account session identity");
        check(call("validate", bob[2]).starts_with(bob[0] + "\tbob\t"), "different account isolation");
        auto second = split(call("login", "alice\t" + original));
        check(second[2] != alice[2], "fresh session token");
        auto first_ticket = split(call("gw-ticket-issue", alice[2]));
        check(first_ticket.size() == 2 && first_ticket[0].size() == 36, "canonical game ticket");
        check(call("gw-ticket-consume", first_ticket[0]).starts_with(alice[0] + "\talice\t"), "game ticket owner");
        call("gw-ticket-consume", first_ticket[0], FW_NOT_FOUND);
        call("gw-ticket-consume", "bad", FW_INVALID);
        auto revoked_ticket = split(call("gw-ticket-issue", alice[2]))[0];
        call("logout", alice[2]); call("validate", alice[2], FW_NOT_FOUND);
        call("gw-ticket-consume", revoked_ticket, FW_NOT_FOUND);
        call("gw-ticket-issue", alice[2], FW_NOT_FOUND);
        call("validate", second[2]);
        auto password_ticket = split(call("gw-ticket-issue", second[2]))[0];
        call("change-password", second[2] + "\t" + password("incorrect-old-password") + "\t" + replacement, FW_NOT_FOUND);
        call("change-password", second[2] + "\t" + original + "\t" + replacement);
        call("validate", second[2], FW_NOT_FOUND);
        call("gw-ticket-consume", password_ticket, FW_NOT_FOUND);
        call("login", "alice\t" + original, FW_NOT_FOUND);
        auto changed = split(call("login", "alice\t" + replacement));
        call("validate", bob[2]);
        std::vector<std::string> tickets;
        for (unsigned i = 0; i < 4; ++i) tickets.push_back(split(call("gw-ticket-issue", changed[2]))[0]);
        call("gw-ticket-issue", changed[2], FW_OVERLOADED);
        for (unsigned i = 1; i < 4; ++i) call("gw-ticket-consume", tickets[i]);
        auto bob_ticket = split(call("gw-ticket-issue", bob[2]))[0];
        std::ofstream saved(argv[3]); saved << changed[2] << '\n' << bob[2] << '\n' << tickets[0] << '\n' << bob_ticket << '\n';
        const auto progress=split(call("gw-character-create",alice[0]+"\tProgress Hero"));
        std::ofstream characters(std::string(argv[3])+".characters");characters<<alice[0]<<'\n'<<authenticated[3]<<'\n'<<progress[0]<<'\n';
        check(bool(saved), "persist test session tokens");
        std::cout << "PASS registration, password verification/change, token isolation and revocation\n";
        auto cancelled=adapter.request(gwserver::Operation::registerAccount,"cancelled-account\t"+original,1);
        check(cancelled.status==FW_TIMEOUT,"direct call enforces deadline");
        call("login","cancelled-account\t"+original,FW_NOT_FOUND);
        std::cout<<"PASS expired direct account creation does not commit\n";
        for (unsigned i = 0; i < 9; ++i) call("login", "persist-rate\t" + original, FW_NOT_FOUND);
    } else if (std::string(argv[2]) == "restart") {
        std::ifstream saved(argv[3]); std::string alice, bob, alice_ticket, bob_ticket, account_id, character_id;
        std::getline(saved, alice); std::getline(saved, bob); std::getline(saved, alice_ticket); std::getline(saved, bob_ticket);
        std::ifstream characters(std::string(argv[3])+".characters");std::getline(characters,account_id);std::getline(characters,character_id);
        std::string progress_id;std::getline(characters,progress_id);
        check(call("gw-character-experience-get",account_id+"\t"+character_id)=="-","unknown imported experience persists through restart");
        check(call("gw-character-experience-get",account_id+"\t"+progress_id)=="0","new character experience persists through restart");
        call("gw-character-delete",account_id+"\t"+progress_id);
        call("gw-character-experience-get",account_id+"\t"+progress_id,FW_NOT_FOUND);
        check(!alice.empty() && !bob.empty(), "restart tokens");
        check(call("gw-character-position-get",account_id+"\t"+character_id)=="148\t120.500000\t-32.250000","character position survives Accounts host restart");
        check(call("gw-account-campaigns-get",account_id)=="1,2,3,4","all development campaign entitlements survive Accounts host restart");
        call("validate", alice); call("validate", bob);
        const auto profile_after_restart=call("gw-account-profile-get",account_id);
        check(profile_after_restart.size()==80 && profile_after_restart.substr(8,2)=="01" &&
              profile_after_restart.substr(68,2)=="a5","captured profile persists through restart");
        check(call("gw-ticket-consume", alice_ticket).find("\talice\t") != std::string::npos, "game ticket persists through restart");
        call("gw-ticket-consume", alice_ticket, FW_NOT_FOUND);
        auto another = split(call("login", "bob\t" + original));
        for (unsigned i = 0; i < 6; ++i) call("login", "bob\t" + original);
        call("login", "bob\t" + original, FW_OVERLOADED);
        call("logout-all", bob); call("validate", bob, FW_NOT_FOUND); call("validate", another[2], FW_NOT_FOUND);
        call("gw-ticket-consume", bob_ticket, FW_NOT_FOUND);
        call("validate", alice);
        auto expiring = split(call("gw-ticket-issue", alice))[0];
        saved.close();
        std::ofstream appended(argv[3], std::ios::app); appended << expiring << '\n'; appended.close();
        call("login", "persist-rate\t" + original, FW_NOT_FOUND);
        call("login", "persist-rate\t" + original, FW_OVERLOADED);
        // Attempt state is also durable and bounded across restart.
        for (unsigned i = 0; i < 10; ++i) call("login", "unknown-rate\t" + original, FW_NOT_FOUND);
        call("login", "unknown-rate\t" + original, FW_OVERLOADED);
        std::cout << "PASS accounts/sessions survive host restart; logout-all isolation, session cap and durable attempt limits\n";
    } else if (std::string(argv[2]) == "ticket-expired") {
        std::ifstream saved(argv[3]); std::string alice, ticket; std::getline(saved, alice);
        for (unsigned i = 0; i < 4; ++i) std::getline(saved, ticket);
        call("validate", alice);
        call("gw-ticket-consume", ticket, FW_NOT_FOUND);
        std::cout << "PASS game ticket expiry independent of parent session\n";
    } else {
        check(std::string(argv[2]) == "expired", "unknown accounts test mode");
        std::ifstream saved(argv[3]); std::string alice; std::getline(saved, alice);
        call("validate", alice, FW_NOT_FOUND);
        std::string ticket;
        for (unsigned i = 0; i < 4; ++i) std::getline(saved, ticket);
        call("gw-ticket-consume", ticket, FW_NOT_FOUND);
        std::cout << "PASS persisted session expiry enforced after restart\n";
    }
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }


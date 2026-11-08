#include "../tools/items/capture_import.h"
#include <stdexcept>
#include <iostream>
#include <source_location>
void check(bool value,std::source_location where=std::source_location::current()){if(!value)throw std::runtime_error("inventory import regression failed at line "+std::to_string(where.line()));}
#include <sstream>
int main()try{using namespace item_capture;
 check(text(capture::Parser("\"41004200\"").parse(),2)==u"AB");
 bool denied=false;try{text(capture::Parser("\"0000\"").parse(),2);}catch(...){denied=true;}check(denied);
 const auto id=fw::gw::parse_token("00000000-0000-0000-0000-000000000001");check(bool(id));
 std::istringstream old("{\"event\":\"start\",\"schema\":1}\n");denied=false;try{read(old,*id,false);}catch(const std::exception& e){denied=std::string(e.what()).find("schema1")!=std::string::npos;}check(denied);
 auto p=snapshot(capture::Parser(R"({"inventory_id":1,"gold_character":0,"bags":[{"wire_id":1,"type":1,"index":0,"capacity":20,"container_item":0,"items":[]}],"weapon_sets":[{"weapon":0,"offhand":0},{"weapon":0,"offhand":0},{"weapon":0,"offhand":0},{"weapon":0,"offhand":0}],"active_weapon_set":0})").parse(),false);
 check(fw::gw::inventory::decode(fw::gw::inventory::encode(p)).items.empty());
 denied=false;try{snapshot(capture::Parser(R"({"inventory_id":1,"gold_character":0,"bags":[{"wire_id":1,"type":1,"index":0,"capacity":20,"container_item":999,"items":[]}],"weapon_sets":[{"weapon":0,"offhand":0},{"weapon":0,"offhand":0},{"weapon":0,"offhand":0},{"weapon":0,"offhand":0}],"active_weapon_set":0})").parse(),false);}catch(...){denied=true;}check(denied);

 const std::string nonempty=R"({"inventory_id":1,"gold_character":123,"bags":[{"wire_id":1,"type":1,"index":0,"capacity":20,"container_item":0,"items":[{"slot":0,"id":10,"model_file":100,"model":200,"type":2,"quantity":1,"interaction":0,"base_value":45,"profession":0,"wire_unknown1":7,"wire_dye":4660,"wire_materials":123,"wire_unknown2":1,"name_utf16_hex":"4100","customized_utf16_hex":"4200","modifiers":[3221225472]}]}],"weapon_sets":[{"weapon":10,"offhand":0},{"weapon":10,"offhand":0},{"weapon":0,"offhand":0},{"weapon":0,"offhand":0}],"active_weapon_set":1})";
 const auto captured=snapshot(capture::Parser(nonempty).parse(),false);const auto encoded=fw::gw::inventory::encode(captured);check(encoded[3]==5);const auto decoded=fw::gw::inventory::decode(encoded);check(decoded.gold==123&&decoded.weapons.active==1&&decoded.weapons.sets[1].weapon==10);const auto& item=decoded.items.at(10);check(item.profession==0&&item.customized==u"B"&&item.definition.modifiers.empty());check(item.definition.unknown1==7&&item.definition.dye==4660&&item.definition.materials==123&&item.definition.unknown2==1);
 auto bad=capture::Parser(nonempty).parse();bad.members.at("gold_character").number=100001;denied=false;try{snapshot(bad,false);}catch(...){denied=true;}check(denied);
 bad=capture::Parser(nonempty).parse();bad.members.at("bags").items[0].members.at("items").items[0].members.at("profession").number=12;denied=false;try{snapshot(bad,false);}catch(...){denied=true;}check(denied);
 bad=capture::Parser(nonempty).parse();bad.members.at("weapon_sets").items[0].members.at("weapon").number=999;denied=false;try{snapshot(bad,false);}catch(...){denied=true;}check(denied);
 bad=capture::Parser(nonempty).parse();bad.members.at("bags").items[0].members.at("items").items[0].members.at("modifiers").items.clear();denied=false;try{snapshot(bad,false);}catch(...){denied=true;}check(denied);
 std::istringstream missing_start(nonempty);denied=false;try{read(missing_start,*id,false);}catch(...){denied=true;}check(denied);

 const std::string start=R"({"event":"start","schema":2,"inventory_id_source":"private_export_rebind","read_only":true,"client_sha256":"8e50edfb83515fabfec50bfd9d17968e8635a39b4c3e796e311bbc5cd15e5878","gwca_revision":"ef3d3670d0d6343471341373680aafd9de911447","layout_evidence_dll_sha256":"b33375229bcb06e415b71246ba317966c0ec6c76f71f284bc49f1783685e1975"})";
 std::string event=nonempty;event.insert(1,"\"event\":\"inventory\",\"character_guid\":\"00000000-0000-0000-0000-000000000001\",");std::istringstream valid(start+"\n"+event+"\n");check(read(valid,*id,false).gold==123);
 auto wrong=start;wrong.replace(wrong.find("8e50"),4,"0000");std::istringstream mismatch(wrong+"\n"+event+"\n");denied=false;try{read(mismatch,*id,false);}catch(...){denied=true;}check(denied);

 auto installed=capture::Parser(nonempty).parse();auto& cb=installed.members.at("bags").items[0];auto container=cb.members.at("items").items[0];container.members.at("type").number=3;container.members.at("id").number=20;cb.members.at("container_item").number=20;cb.members["container_definition"]=container;cb.members["container_ownership"]=capture::Parser("\"item_table_exact_installed_bag_backlink_no_slot\"").parse();const auto cp=snapshot(installed,false);check(cp.items.at(20).bag==0&&cp.items.at(20).slot==0);check(fw::gw::inventory::decode(fw::gw::inventory::encode(cp)).bags.at(1).container==20);
 auto reject_container=[&](capture::Json j){bool rejected=false;try{snapshot(j,false);}catch(...){rejected=true;}check(rejected);};
 bad=installed;bad.members.at("bags").items[0].members.erase("container_definition");reject_container(bad);
 bad=installed;bad.members.at("bags").items[0].members.at("container_definition").members.at("id").number=21;reject_container(bad);
 bad=installed;bad.members.at("bags").items[0].members.at("container_definition").members.at("quantity").number=2;reject_container(bad);
 bad=installed;bad.members.at("bags").items[0].members.at("container_definition").members.at("type").number=2;reject_container(bad);
 bad=installed;bad.members.at("bags").items[0].members.at("container_item").number=0;reject_container(bad);
 bad=installed;bad.members.at("bags").items[0].members.at("container_ownership").text="guess";reject_container(bad);
 bad=installed;bad.members.at("bags").items[0].members.at("items").items.push_back(container);reject_container(bad);

 // The native element24 is Diamond's physical material slot; visual row21 is not.
 const std::string material=R"({"inventory_id":1,"gold_storage":123456,"material_storage_capacity":2500,"material_storage_capacity_source":"matching_gwca_dll_account_count131","material_storage_expansion_count":9,"bags":[{"wire_id":4,"type":5,"index":5,"capacity":41,"container_item":0,"items":[{"slot":24,"id":10,"model_file":100,"model":935,"type":11,"quantity":2500,"interaction":524288,"base_value":45,"profession":0,"wire_unknown1":7,"wire_dye":4660,"wire_materials":123,"wire_unknown2":1,"name_utf16_hex":"4100","customized_utf16_hex":"4200","modifiers":[621287424,3221225472]}]}]})";
 const auto mp=snapshot(capture::Parser(material).parse(),true);check(mp.material_quota==2500&&mp.gold==123456&&mp.items.at(10).slot==24);check(mp.items.at(10).definition.modifiers==std::vector<uint32_t>{0x25081800});check(mp.items.at(10).profession==0&&mp.items.at(10).customized==u"B");check(fw::gw::inventory::encode(fw::gw::inventory::decode(fw::gw::inventory::encode(mp)))==fw::gw::inventory::encode(mp));
 auto reject_bank=[&](capture::Json j){bool rejected=false;try{snapshot(j,true);}catch(...){rejected=true;}check(rejected);};
 bad=capture::Parser(material).parse();bad.members.erase("material_storage_capacity");reject_bank(bad);
 bad=capture::Parser(material).parse();bad.members.at("material_storage_capacity_source").text="inferred_quantity";reject_bank(bad);
 bad=capture::Parser(material).parse();bad.members.at("material_storage_expansion_count").number=8;reject_bank(bad);
 bad=capture::Parser(material).parse();bad.members.at("material_storage_capacity").number=251;reject_bank(bad);
 bad=capture::Parser(material).parse();bad.members.at("bags").items[0].members.at("items").items[0].members.at("slot").number=21;reject_bank(bad);
 bad=capture::Parser(material).parse();bad.members.at("bags").items[0].members.at("items").items[0].members.at("model").number=499;reject_bank(bad);
 bad=capture::Parser(material).parse();bad.members.at("bags").items[0].members.at("items").items[0].members.at("modifiers").items.push_back(capture::Parser("621287424").parse());reject_bank(bad);
 bad=capture::Parser(material).parse();bad.members.at("material_storage_capacity").number=250;bad.members.at("material_storage_expansion_count")=capture::Parser("null").parse();bad.members.at("bags").items[0].members.at("items").items[0].members.at("quantity").number=250;check(snapshot(bad,true).material_quota==250);
 // Same observation, separate ownership: a material pane is never character-owned.
 auto both=capture::Parser(nonempty).parse();both.members.at("bags").items.push_back(capture::Parser(material).parse().members.at("bags").items[0]);check(snapshot(both,false).bags.size()==1&&snapshot(both,false).material_quota==250);
 auto schema3=start;schema3.replace(schema3.find("\"schema\":2"),10,"\"schema\":3");std::string material_event=material;material_event.insert(1,"\"event\":\"inventory\",\"character_guid\":\"00000000-0000-0000-0000-000000000001\",");std::istringstream quota_capture(schema3+"\n"+material_event+"\n");check(read(quota_capture,*id,true).material_quota==2500);
 std::istringstream quota_legacy(start+"\n"+material_event+"\n");denied=false;try{read(quota_legacy,*id,true);}catch(const std::exception& e){denied=std::string(e.what()).find("schema2 material")!=std::string::npos;}check(denied);
 std::cout<<"PASS inventory capture import, installed containers, material quota and ownership\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

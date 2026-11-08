// NEW unregistered selected-hex optional owner-level contract; no fake Player.
#include "gw_selected_hex_application.h"
#include <stdexcept>
#include <iostream>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("NPC selected hex owner-credit contract");}
int main(){const Scope owner{1,1,1},caster{1,100,1};Registry registry(owner);const Entry incoming{1,caster.actor,26,0,Category::hex,1000,6000};combat::Actor npc{100,1,combat::Health{500,1000},{},{},true};
 const auto unprotected=selected_hex_application(registry,owner,caster,incoming,Replace::same_skill,npc,std::nullopt,1000);check(unprotected&&unprotected->kind==HexApplicationKind::applied);
 check(!selected_hex_application(registry,owner,caster,incoming,Replace::same_skill,npc,0,1000));
 check(registry.apply(owner,{2,owner.actor,10,12,Category::stance,100,65100},Replace::same_skill).accepted);
 check(!selected_hex_application(registry,owner,caster,incoming,Replace::same_skill,npc,std::nullopt,1000));
 const auto prevented=selected_hex_application(registry,owner,caster,incoming,Replace::same_skill,npc,20,1000);check(prevented&&prevented->kind==HexApplicationKind::prevented&&prevented->retaliation&&prevented->retaliation->reaction.caster==caster);
 std::cout<<"PASS selected NPC hex requires owner level only for actual HexBreaker retaliation\n";
}

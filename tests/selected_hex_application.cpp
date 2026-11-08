#include "gw_selected_hex_application.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Selected hex application outcome regression");}
int main()try{
 const Scope owner{1,2,1},caster{1,3,1};Registry original(owner);
 check(original.apply(owner,{88,3,100,12,Category::hex,100,20100},Replace::same_skill).accepted);
 check(original.observe_applications(owner,100));
 check(original.apply(owner,{50,3,28,0,Category::hex,100,10100},Replace::same_skill).accepted);
 original.take_applications(owner);
 check(original.apply(owner,{1,2,10,12,Category::stance,100,65100},Replace::category).accepted);
 const Entry incoming{99,3,28,12,Category::hex,200,10200};combat::Actor actor{3,1,combat::Health{400,500},{},0,true};
 auto blocked=selected_hex_application(original,owner,caster,incoming,Replace::same_skill,actor,20,200);
 check(blocked&&blocked->kind==HexApplicationKind::prevented&&!blocked->application.accepted&&blocked->retaliation);
 check(blocked->registry.lookup(owner,50)&&!blocked->registry.lookup(owner,99)&&!blocked->registry.lookup(owner,1));
 check(blocked->registry.take_applications(owner).empty()); // Prevented application cannot trigger Soul Barbs.
 check(blocked->retaliation->caster.health->current==361&&blocked->retaliation->hit.skill==10);
 check(original.lookup(owner,1)&&original.lookup(owner,50));
 auto unprotected=original;unprotected.remove(owner,1);
 auto accepted=selected_hex_application(unprotected,owner,caster,incoming,Replace::same_skill,actor,20,200);
 check(accepted&&accepted->kind==HexApplicationKind::applied&&accepted->application.accepted&&!accepted->retaliation);
 check(accepted->application.removed.size()==1&&!accepted->registry.lookup(owner,50)&&accepted->registry.lookup(owner,99));
 check(accepted->registry.take_applications(owner).size()==1);
 auto invalid=incoming;invalid.applied=199;check(!selected_hex_application(original,owner,caster,invalid,Replace::same_skill,actor,20,200));
 check(!selected_hex_application(original,{1,2,2},caster,incoming,Replace::same_skill,actor,20,200));
 check(bool(original.lookup(owner,1)));
 std::cout<<"PASS applied/prevented/invalid hex outcomes,preserve olderhex,consume only proposed stance,retaliation and no blocked application observers;World adapter pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

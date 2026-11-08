#include "shared_completion_reaction_publication_fixture_draft.h"
int main()try{
 CompletionPublicationFixture f;publication_check(f.w.aura_restoration_enchantment(f.owner,180,0,100));const auto packets=f.finish(3000);const auto state=*f.w.resources(1);publication_check(f.payments==1&&f.paid==10&&f.completed==1&&f.finalized==1&&f.copied==1&&state.health==125&&f.health_at_finalize==125&&f.health_at_copy==125);
 const auto expected=vitals::public_health_2022(1,state).back();const auto last=std::find_if(packets.rbegin(),packets.rend(),[&](const auto& packet){return packet.size()==expected.size()&&std::equal(packet.begin(),packet.end()-sizeof(float),expected.begin());});publication_check(last!=packets.rend());auto stale=state;stale.health=105;std::cout<<"actualHealth="<<state.health<<" last_packet_is_stale105="<<(*last==vitals::public_health_2022(1,stale).back())<<" last_matches125="<<(*last==expected)<<'\n';
 publication_check(*last==expected);std::cout<<"PASS real1375+254+180 final Health packet follows actual125 state\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

#include "gw_collector_panel.h"
#include "gw_presearing_collectors.h"
#include <fstream>
#include <iostream>

int main(int argc,char** argv)try{
 namespace n=fw::gw::collectors::native;namespace c=fw::gw::collectors;namespace inv=fw::gw::inventory;
 const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 check(argc==2,"original completion fixture path required");
 std::ifstream input(argv[1]);std::string label,ack_fixture;input>>label>>ack_fixture;
 check(label=="complete"&&ack_fixture=="cb8002","original completion fixture unavailable");
 const auto offers=c::presearing_offers();auto stored=inv::baseline();stored.next_item=2;
 inv::Item trophy;trophy.definition=offers[0].trophy;trophy.definition.id=1;trophy.definition.quantity=10;trophy.bag=1;
 stored.items.emplace(1,trophy);inv::identity::Session identities;
 auto wire=identities.hydrate(stored,fw::gw::storage::baseline());
 std::uint64_t revision=10,durable_revision=10;n::Binding binding;n::MenuIds ids;
 n::Observation observation{10012,146,6,99,7,revision,500,true};
 const auto open=[&]{observation.revision=revision;
  return n::open_panel(binding,ids,offers,observation,"Brownlow [Collector]");};
 check(bool(open()),"initial native panel unavailable");
 const auto request=[&](unsigned opcode,unsigned reward,unsigned quantity){fw::gw::items::Packet packet;
  fw::gw::items::put(packet,opcode,2);fw::gw::items::put(packet,2,1);fw::gw::items::put(packet,0,4);
  fw::gw::items::put(packet,1,2);fw::gw::items::put(packet,1,4);
  if(opcode==75){fw::gw::items::put(packet,1,2);fw::gw::items::put(packet,quantity,1);}
  fw::gw::items::put(packet,0,4);fw::gw::items::put(packet,1,2);fw::gw::items::put(packet,reward,4);
  if(opcode==75){fw::gw::items::put(packet,1,2);fw::gw::items::put(packet,1,1);}return packet;};
 inv::Batch published;unsigned saves=0;std::size_t inventory_packets=0;
 const auto transact=[&](const fw::gw::items::Packet& packet,bool save_succeeds){published.clear();
  observation.revision=revision;const auto plan=n::authorize_wire(binding,packet,offers,wire.character,observation);
  if(!plan)return false;
  const auto result=n::commit_selected_exchange(*plan,identities,wire.character,wire.bank,revision,146,6,u"Tester",
   [&](const inv::Profile& candidate,std::uint64_t expected)->std::optional<std::uint64_t>{++saves;
    check(expected==durable_revision,"flow CAS expected stale revision");if(!save_succeeds)return {};
    stored=candidate;return ++durable_revision;});
  if(!result)return false;
  inventory_packets=result->packets.size();published=result->packets;published.push_back({203,0,2});
  auto next=open();check(bool(next),"successful purchase did not refresh native panel");
  published.insert(published.end(),next->packets.begin(),next->packets.end());return true;};
 auto first=request(75,binding.offers[0].first,5);const auto before=inv::encode(wire.character);
 check(!transact(first,false)&&saves==1&&published.empty()&&revision==10&&durable_revision==10&&
       inv::encode(wire.character)==before,"failed durable write published inventory or completion");
 check(bool(open()),"reinteraction after failed CAS unavailable");
 first=request(75,binding.offers[0].first,5);
 check(transact(first,true)&&revision==11&&durable_revision==11&&stored.items.at(1).definition.quantity==5,
       "partial-stack native exchange failed");
 check(published.at(inventory_packets)==fw::gw::items::Packet{203,0,2}&&
       published.at(inventory_packets+1).front()==195,"completion preceded inventory or panel refresh missing");
 check(!transact(first,true)&&saves==2&&published.empty()&&revision==11,"old panel request replay reached CAS");
 check(bool(open()),"native panel unavailable after rejected replay");
 auto second=request(72,binding.offers[0].first,0);
 check(transact(second,true)&&revision==12&&durable_revision==12&&!stored.items.contains(1)&&stored.items.size()==2,
       "subsequent whole-stack native purchase failed or lost trusted rewards");
 for(const auto& [id,item]:stored.items)check(item.definition.file==offers[0].reward.definition.file,"untrusted flow reward");
 std::cout<<"PASS native panel admission, exact partial/full debit, durable CAS, publication ordering and refreshed-menu replay rejection\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

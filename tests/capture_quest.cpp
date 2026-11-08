#include "../tools/capture/quest_observation.h"
#include "../tools/capture/quest_info_queue.h"
#include "../tools/capture/quest_info_request.h"
#include <stdexcept>
int main(){
 auto require=[](bool b){if(!b)throw std::runtime_error("quest/visibility lifecycle");};
 capture::QuestInfoQueue queue;
 std::array<std::uint8_t,43> signature={0x55,0x8b,0xec,0x83,0xec,0x08,0x8b,0x45,0x08,0x89,0x45,0xfc,0x8d,0x45,0xf8,0x50,0x6a,0x08,0xc7,0x45,0xf8,0x12,0,0,0,0xe8,0x02,0x09,0xb7,0xff,0x50,0xe8,0xfc,0xc2,0xeb,0xff,0x83,0xc4,0x0c,0x8b,0xe5,0x5d,0xc3};
 require(capture::quest_info_sender_matches(signature));
 for(unsigned i=0;i<signature.size();++i){auto changed=signature;changed[i]^=1;require(!capture::quest_info_sender_matches(changed));}
 require(!capture::quest_info_sender_matches(std::span(signature).first(42)));
 const std::map<std::uint32_t,bool> quests{{0,false},{101,false},{102,true},{103,false}};
 require(!queue.next(1,7,0,quests,false));
 require(queue.next(1,7,0,quests,true)==101);
 require(!queue.next(1,7,999,quests,true));
 require(queue.next(1,7,1000,quests,true)==103);
 require(!queue.next(1,7,2000,quests,true));
 require(!queue.next(1,7,100,quests,true));
 require(queue.next(2,7,2001,quests,true)==101);
 require(queue.next(2,8,2002,quests,true)==101);
 require(!queue.next(2,0,3002,quests,true));
 capture::QuestSnapshotTracker q;
 require(q.observe(1,7,{{7,"accepted"}})&&q.revision==1);
 require(!q.observe(1,7,{{7,"accepted"}})&&q.revision==1);
 require(q.observe(1,7,{{7,"completed"}})&&q.revision==2);
 require(q.observe(1,0,{})&&q.revision==3);
 require(q.observe(2,0,{})&&q.revision==4);
 capture::NpcVisibilityTracker n;
 auto d=n.observe(1,{{10,{20,false}}});require(d.size()==1&&d[0].appeared);
 require(n.observe(1,{{10,{20,false}}}).empty());
 d=n.observe(1,{{10,{21,false}}});require(d.size()==2&&!d[0].appeared&&d[1].appeared);
 d=n.observe(2,{{11,{22,false}}});require(d.size()==1&&d[0].actor==11&&d[0].appeared);
 d=n.observe(2,{{11,{22,true}}});require(d.size()==1&&d[0].dead&&d[0].status_changed&&d[0].appeared);
 d=n.observe(2,{});require(d.size()==1&&!d[0].appeared);
}

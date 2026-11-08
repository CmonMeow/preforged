#include "../tools/packet_logger/packet_logger.cpp"
#include <iostream>
unsigned calls=0;
void* received=nullptr;
bool __cdecl receiver(void* p){++calls;received=p;return false;}
int main(){
 originals[7]=receiver;lengths[7]=8;
 uint32_t packet[]{7,123};
 if(forward<7>(packet)||calls!=1||received!=packet)return 1;
 unsigned char bytes[8]{};
 if(!copy(bytes,packet,sizeof packet)||memcmp(bytes,packet,sizeof packet))return 2;
 if(copy(bytes,nullptr,sizeof bytes))return 3;
 auto path=std::filesystem::temp_directory_path()/L"gw-packet-logger-test.jsonl";
 output.open(path);recording=true;
 if(forward<7>(packet)||calls!=2)return 4;
 installed=true;state=4;
 StopCapture(nullptr);
 if(forward<7>(packet)||calls!=3)return 5;
 if(state!=3||recording||StartCapture(nullptr)!=0||state!=4||!recording)return 8;
 if(StartCapture(nullptr)!=0||forward<7>(packet)||calls!=4)return 9;
 StopCapture(nullptr);
 if(StopCapture(nullptr)!=0||forward<7>(packet)||calls!=5)return 10;
 output.close();std::ifstream input(path);std::string line;std::getline(input,line);
 if(line.find("070000007b000000")==std::string::npos||line.find("\"complete\":true")==std::string::npos)return 6;
 unsigned paused=0,resumed=0,packets=1;
 while(std::getline(input,line)){
  if(line.find("\"event\":\"paused\"")!=line.npos)++paused;
  else if(line.find("\"event\":\"resumed\"")!=line.npos)++resumed;
  else if(line.find("\"event\":\"packet\"")!=line.npos)++packets;
  else return 7;
 }
 if(paused!=2||resumed!=1||packets!=2)return 11;
 input.close();std::filesystem::remove(path);
 HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(capture::InteractionContext),capture::interaction_mapping().c_str());
 auto* context=static_cast<capture::InteractionContext*>(MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,sizeof(capture::InteractionContext)));
 if(!context)return 12;
 context->sequence=2;context->map=55;context->agent=77;context->tick=GetTickCount64();
 wcscpy_s(context->name,L"Xunlai: Agent/Storage");
 schema_rows="{\"event\":\"schema\",\"opcode\":7,\"fields\":[7,16]}\n";
 recent_packets.clear();
 for(unsigned i=0;i<200;++i)retain_recent(context->tick,"{\"event\":\"packet\",\"test_prelude\":true}\n");
 if(recent_packets.size()!=recent_packet_limit)return 17;
 begin_interaction(77);interaction.flush();
 begin_interaction(77);interaction.flush();
 if(interaction_serial!=2)return 13;
 begin_interaction(0);if(interaction.is_open())return 14;
 wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);
 unsigned verified=0;
 for(const auto& file:std::filesystem::directory_iterator(std::filesystem::path(exe).parent_path()/L"interactions")){
  if(file.path().filename().wstring().find(L"npc-Xunlai_ Agent_Storage-"+std::to_wstring(GetCurrentProcessId())+L"-")!=0)continue;
  std::ifstream record(file.path());std::getline(record,line);
  if(line.find("\"name_verified\":true")==line.npos||line.find("\"map\":55")==line.npos)return 15;
  std::getline(record,line);if(line!=schema_rows.substr(0,schema_rows.size()-1))return 18;
  std::getline(record,line);if(line.find("\"context_only\":true")==line.npos)return 19;
  unsigned prelude=0;
  while(std::getline(record,line)&&line.find("interaction_packets_begin")==line.npos){
   if(line.find("\"test_prelude\":true")==line.npos)return 20;
   ++prelude;
  }
  if(prelude!=recent_packet_limit||line.find("interaction_packets_begin")==line.npos)return 21;
  record.close();std::filesystem::remove(file.path());++verified;
 }
 UnmapViewOfFile(context);CloseHandle(mapping);
 if(verified!=2)return 16;
 recent_packets.clear();retain_recent(1,"old");retain_recent(2002,"new");
 if(recent_packets.size()!=1||recent_packets.front().row!="new")return 22;
 installed=true;state=4;recording=true;StopCapture(nullptr);
 if(!recent_packets.empty())return 23;
 // Capture active item data without consuming/changing the original handler's
 // input or result. Invalid arrays remain visibly incomplete header records.
 const std::array<std::uint32_t,15> item_schema{353,0x404,0x404,0x104,0x104,0x204,0x204,0x104,0x404,0x404,0x404,0x404,0x4017,0x400c,0x404};
 item_layouts[353]=packet_capture::item_layout(item_schema);lengths[353]=4;originals[353]=receiver;
 std::array<unsigned char,436> item_packet{};item_packet.fill(0xcc);
 const std::uint32_t header=353,count=2,over=65;
 memcpy(item_packet.data(),&header,4);memcpy(item_packet.data()+0xb0,&count,4);
 const std::uint16_t item_name[]{0x108,0x107,0};memcpy(item_packet.data()+0x30,item_name,sizeof item_name);
 const auto before=item_packet;const auto before_calls=calls;
 output.open(path,std::ios::trunc);recording=true;
 if(forward<353>(item_packet.data())||calls!=before_calls+1||received!=item_packet.data()||item_packet!=before)return 24;
 memcpy(item_packet.data()+0xb0,&over,4);
 if(forward<353>(item_packet.data())||calls!=before_calls+2)return 25;
 recording=false;output.close();input.open(path);std::getline(input,line);
 if(line.find("\"bytes\":436")==line.npos||line.find("\"complete\":true")==line.npos||line.find("\"normalized_unused_capacity\":true")==line.npos)return 26;
 std::getline(input,line);
 if(line.find("\"bytes\":4")==line.npos||line.find("\"complete\":false")==line.npos||line.find("\"item_definition_capture_failed\":true")==line.npos)return 27;
 input.close();std::filesystem::remove(path);
 // A collector's incoming item-list capture must never mutate its source,
 // skip its original callback, or consume the callback's false return value.
 const std::array<std::uint32_t,3> window_schema{132,0x102b,6};
 window_items_layouts[132]=packet_capture::window_items_layout(window_schema);lengths[132]=4;originals[132]=receiver;
 std::array<std::uint32_t,18> window_packet{};window_packet.fill(0xcccccccc);
 window_packet[0]=132;window_packet[1]=3;window_packet[2]=506;window_packet[3]=585;window_packet[4]=586;
 const auto original_window=window_packet;const auto window_calls=calls;
 output.open(path,std::ios::trunc);recording=true;
 if(forward<132>(window_packet.data())||calls!=window_calls+1||received!=window_packet.data()||window_packet!=original_window)return 28;
 window_packet[1]=17;const auto invalid_window=window_packet;
 if(forward<132>(window_packet.data())||calls!=window_calls+2||received!=window_packet.data()||window_packet!=invalid_window)return 29;
 SYSTEM_INFO memory_info{};GetSystemInfo(&memory_info);
 auto* pages=static_cast<unsigned char*>(VirtualAlloc(nullptr,memory_info.dwPageSize*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
 if(!pages)return 34;
 auto* unreadable_ids=pages+memory_info.dwPageSize-8;
 const std::uint32_t short_header[]{132,3};memcpy(unreadable_ids,short_header,sizeof short_header);
 DWORD old_protection=0;
 if(!VirtualProtect(pages+memory_info.dwPageSize,memory_info.dwPageSize,PAGE_NOACCESS,&old_protection))return 35;
 const bool forwarded=forward<132>(unreadable_ids);
 const bool original_received=received==unreadable_ids;
 VirtualFree(pages,0,MEM_RELEASE);
 if(forwarded||calls!=window_calls+3||!original_received)return 36;
 recording=false;
 if(forward<132>(window_packet.data())||calls!=window_calls+4)return 30;
 output.close();input.open(path);std::getline(input,line);
 if(line.find("\"bytes\":72")==line.npos||line.find("\"complete\":true")==line.npos||
    line.find("8400000003000000fa010000490200004a020000")==line.npos||line.find("cccccccc")!=line.npos)return 31;
 std::getline(input,line);
 if(line.find("\"bytes\":4")==line.npos||line.find("\"complete\":false")==line.npos||
    line.find("\"window_items_capture_failed\":true")==line.npos)return 32;
 std::getline(input,line);
 if(line.find("\"bytes\":4")==line.npos||line.find("\"complete\":false")==line.npos||
    line.find("\"window_items_capture_failed\":true")==line.npos)return 37;
 if(std::getline(input,line))return 33;
 input.close();std::filesystem::remove(path);
 std::cout<<"Forwarding, original return value, bounded copy, fault handling, raw bytes and stop passed\n";
}

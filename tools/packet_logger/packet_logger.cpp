#include <windows.h>
#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <utility>
#include <cstring>
#include <sstream>
#include <deque>
#include <vector>
#include "../capture/interaction_context.h"
#include "../capture/name_json.h"
#include "item_layout.h"
#include "window_items_layout.h"

// Native decoded StoC layout and discovery fingerprint from Toolbox PacketLoggerWindow.
// No GWCA initialization: the handler table is resolved from the live client image.
namespace {
// The native client handlers are cdecl (packet at [ebp+8]). Toolbox's
// fastcall declaration describes GWCA's substituted callback, not this table.
using Handler = bool(__cdecl*)(void*);
struct Descriptor {uint32_t* fields; uint32_t count; Handler callback;};
struct Table {Descriptor* buffer; uint32_t capacity,size,param;};
constexpr size_t limit=1024;
std::array<Handler,limit> originals{};
std::array<size_t,limit> lengths{};
std::array<std::optional<packet_capture::ItemLayout>,limit> item_layouts{};
std::array<std::optional<packet_capture::WindowItemsLayout>,limit> window_items_layouts{};
HMODULE self;
std::atomic<unsigned> state{0};
std::atomic<bool> recording{false};
std::atomic<bool> installed{false};
std::mutex mutex;
std::ofstream output;
std::ofstream interaction;
std::uint64_t interaction_serial=0;
std::string schema_rows;
struct RecentPacket {std::uint64_t tick;std::string row;};
std::deque<RecentPacket> recent_packets;
constexpr std::size_t recent_packet_limit=128;
constexpr std::uint64_t recent_packet_ms=2000;
void retain_recent(std::uint64_t tick,std::string row){
 recent_packets.push_back({tick,std::move(row)});
 while(!recent_packets.empty()&&(recent_packets.size()>recent_packet_limit||
       (tick>=recent_packets.front().tick&&tick-recent_packets.front().tick>recent_packet_ms)))recent_packets.pop_front();
}
void begin_interaction(std::uint32_t agent){
 interaction.close();
 if(!agent)return;
 capture::InteractionContext snapshot{};
 HANDLE mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,capture::interaction_mapping().c_str());
 if(mapping){
  auto* view=static_cast<const capture::InteractionContext*>(MapViewOfFile(mapping,FILE_MAP_READ,0,0,sizeof snapshot));
  if(view){
   LONG before=view->sequence;MemoryBarrier();memcpy(&snapshot,view,sizeof snapshot);MemoryBarrier();
   if((before&1)||before!=view->sequence)snapshot={};
   UnmapViewOfFile(view);
  }CloseHandle(mapping);
 }
 const auto now=GetTickCount64();
 const bool matched=snapshot.agent==agent&&now>=snapshot.tick&&now-snapshot.tick<=2000;
 snapshot.name[127]=0;
 std::wstring name=matched?snapshot.name:L"";
 if(name.empty())name=L"Unknown-NPC";
 auto filename=name;
 for(auto& c:filename)if(c<32||wcschr(L"<>:\"/\\|?*",c))c=L'_';
 while(!filename.empty()&&(filename.back()==L'.'||filename.back()==L' '))filename.pop_back();
 wchar_t path[32768];GetModuleFileNameW(self,path,32768);
 auto directory=std::filesystem::path(path).parent_path()/L"interactions";
 std::filesystem::create_directories(directory);
 interaction.open(directory/(L"npc-"+filename+L"-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(now)+L"-"+std::to_wstring(++interaction_serial)+L"-"+std::to_wstring(agent)+L".jsonl"));
 interaction<<"{\"event\":\"interaction_start\",\"process_id\":"<<GetCurrentProcessId()<<",\"tick_ms\":"<<now<<",\"agent\":"<<agent
  <<",\"map\":"<<(matched?snapshot.map:0)<<",\"name\":"<<capture::name_json(name)
  <<",\"name_verified\":"<<(matched&&name!=L"Unknown-NPC"?"true":"false")<<"}\n";
 // A standalone interaction must retain its decoding schemas. A bounded
 // prelude also preserves dialog content delivered before DialogSender.
 // Prelude packets are context only, not proven responses from this NPC.
 interaction<<schema_rows;
 interaction<<"{\"event\":\"interaction_prelude\",\"context_only\":true,\"max_packets\":"<<recent_packet_limit
  <<",\"max_age_ms\":"<<recent_packet_ms<<"}\n";
 for(const auto& previous:recent_packets)if(now>=previous.tick&&now-previous.tick<=recent_packet_ms)interaction<<previous.row;
 interaction<<"{\"event\":\"interaction_packets_begin\",\"tick_ms\":"<<now<<"}\n";
}
bool copy(void* dest,const void* src,size_t n) noexcept {
 SIZE_T read=0;
 return src&&dest&&ReadProcessMemory(GetCurrentProcess(),src,dest,n,&read)&&read==n;
}
bool executable(void* p){MEMORY_BASIC_INFORMATION m{};return VirtualQuery(p,&m,sizeof m)&&m.State==MEM_COMMIT&&!(m.Protect&PAGE_GUARD)&&(m.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY));}
void log(size_t id,void* packet) noexcept {
 if(!recording)return;
 try {
  std::array<unsigned char,8192> bytes{};
  size_t n=lengths[id];bool normalized=false;
  bool valid=false;
  if(item_layouts[id]){
   normalized=packet_capture::copy_item(*item_layouts[id],packet,bytes,copy);
   n=normalized?item_layouts[id]->size:4;
   valid=normalized||copy(bytes.data(),packet,n);
  }else if(window_items_layouts[id]){
   normalized=packet_capture::copy_window_items(*window_items_layouts[id],packet,bytes,copy);
   n=normalized?window_items_layouts[id]->size:4;
   valid=normalized||copy(bytes.data(),packet,n);
  }else valid=copy(bytes.data(),packet,n);
  std::lock_guard lock(mutex);
  if(!recording)return;
  if(id==129&&valid&&n==8){std::uint32_t agent=0;memcpy(&agent,bytes.data()+4,4);begin_interaction(agent);}
  if(id==410&&valid&&n==32){interaction.close();recent_packets.clear();}
  const auto tick=GetTickCount64();
  std::ostringstream row;
  row<<"{\"event\":\"packet\",\"direction\":\"StoC\",\"process_id\":"<<GetCurrentProcessId()<<",\"tick_ms\":"<<tick<<",\"opcode\":"<<id<<",\"bytes\":"<<(valid?n:0)<<",\"complete\":"<<(n>4&&valid?"true":"false")<<",\"hex\":\"";
  constexpr char hex[]="0123456789abcdef";
  if(valid)for(size_t i=0;i<n;++i)row<<hex[bytes[i]>>4]<<hex[bytes[i]&15];
  row<<"\"";if(normalized)row<<",\"normalized_unused_capacity\":true";
  else if(item_layouts[id])row<<",\"item_definition_capture_failed\":true";
  else if(window_items_layouts[id])row<<",\"window_items_capture_failed\":true";
  row<<"}\n";auto serialized=row.str();output<<serialized;if(interaction)interaction<<serialized;
  retain_recent(tick,std::move(serialized));
 }catch(...){recording=false;}
}
template<size_t I> bool __cdecl forward(void* packet){log(I,packet);return originals[I](packet);}
template<size_t... I> constexpr auto callbacks(std::index_sequence<I...>){return std::array<Handler,sizeof...(I)>{&forward<I>...};}
constexpr auto forwards=callbacks(std::make_index_sequence<limit>{});
Table* discover(){
 auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
 IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS32 nt{};
 if(!copy(&dos,base,sizeof dos)||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<0||dos.e_lfanew>1048576||!copy(&nt,base+dos.e_lfanew,sizeof nt))return nullptr;
 if(nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||!nt.OptionalHeader.SizeOfImage)return nullptr;
 constexpr unsigned char signature[]{0x75,0x04,0x33,0xc0,0x5d,0xc3,0x8b,0x41,0x08,0xa8,0x01,0x75};
 auto sections=reinterpret_cast<IMAGE_SECTION_HEADER*>(base+dos.e_lfanew+sizeof(DWORD)+sizeof(IMAGE_FILE_HEADER)+nt.FileHeader.SizeOfOptionalHeader);
 Table* found=nullptr;
 for(unsigned s=0;s<nt.FileHeader.NumberOfSections;++s){
  if(!(sections[s].Characteristics&IMAGE_SCN_MEM_EXECUTE))continue;
  if(sections[s].VirtualAddress>=nt.OptionalHeader.SizeOfImage||sections[s].Misc.VirtualSize>nt.OptionalHeader.SizeOfImage-sections[s].VirtualAddress)continue;
  auto start=base+sections[s].VirtualAddress;size_t size=sections[s].Misc.VirtualSize;
  for(size_t i=6;i+sizeof signature<=size;++i){
   if(memcmp(start+i,signature,sizeof signature))continue;
   uintptr_t pointer=0,server=0,codec=0;
   if(!copy(&pointer,start+i-6,4)||pointer<reinterpret_cast<uintptr_t>(base)||pointer>reinterpret_cast<uintptr_t>(base)+nt.OptionalHeader.SizeOfImage-sizeof(uint32_t)||!copy(&server,reinterpret_cast<void*>(pointer),4)||!server||server>UINTPTR_MAX-12||!copy(&codec,reinterpret_cast<void*>(server+8),4)||!codec||codec>UINTPTR_MAX-44)continue;
   // gs_codec: 12 bytes, ls_codec pointer, 12 bytes, codec[4], handlers.
   auto candidate=reinterpret_cast<Table*>(codec+44);
   Table table{};if(!copy(&table,candidate,sizeof table)||!table.buffer||!table.size||table.size>limit||table.size>table.capacity)continue;
   if(found&&found!=candidate)return nullptr;
   found=candidate;
  }
 }
 return found;
}
DWORD WINAPI run(void*){
 wchar_t path[32768];GetModuleFileNameW(self,path,32768);
 output.open(std::filesystem::path(path).parent_path()/(L"packets-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L".jsonl"));
 if(!output){state=3;return 1;}
 Table* address=discover();Table table{};
 if(!address||!copy(&table,address,sizeof table)||!table.buffer||!table.size||table.size>limit||table.size>table.capacity){output<<"{\"event\":\"unsupported_handler_table\"}\n";output.flush();state=3;return 2;}
 // Validate the entire table before installing any forwarding callback.
 for(size_t i=0;i<table.size;++i){
  Descriptor d{};if(!copy(&d,table.buffer+i,sizeof d)||d.count>256){output<<"{\"event\":\"invalid_descriptor\",\"opcode\":"<<i<<"}\n";output.flush();state=3;return 3;}
  if(!d.count||!d.callback){lengths[i]=0;continue;}
  if(!executable(reinterpret_cast<void*>(d.callback))){output<<"{\"event\":\"invalid_callback\",\"opcode\":"<<i<<"}\n";output.flush();state=3;return 3;}
  originals[i]=d.callback;size_t n=4;bool complete=true;
  std::ostringstream schema;
  std::vector<std::uint32_t> fields;
  schema<<"{\"event\":\"schema\",\"opcode\":"<<i<<",\"fields\":[";
  for(unsigned j=0;j<d.count;++j){uint32_t f=0;if(!copy(&f,d.fields+j,4)){state=3;return 4;}fields.push_back(f);if(j)schema<<',';schema<<f;
   if(!j)continue;
   auto type=f&15,count=(f>>8)&65535;
   switch(type){case 0:case 1:case 4:case 8:n+=4;break;case 2:n+=8;break;case 3:n+=12;break;case 5:case 9:n+=count;break;case 7:n+=count*2;break;case 6:case 10:break;default:complete=false;}
  }
  schema<<"]}\n";schema_rows+=schema.str();output<<schema.str();lengths[i]=complete&&n<=8192?n:4;
  if(!fields.empty()&&fields.front()==i){
   item_layouts[i]=packet_capture::item_layout(fields);
   window_items_layouts[i]=packet_capture::window_items_layout(fields);
  }
 }
 output<<"{\"event\":\"ready\",\"handlers\":"<<table.size<<",\"layout\":\"decoded_client_memory\"}\n";output.flush();
 HMODULE pinned=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(&run),&pinned);
 for(size_t i=0;i<table.size;++i){
  if(!originals[i])continue;
  auto slot=reinterpret_cast<void* volatile*>(&table.buffer[i].callback);
  if(InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(forwards[i]),reinterpret_cast<void*>(originals[i]))!=reinterpret_cast<void*>(originals[i])){
   for(size_t j=0;j<i;++j)if(originals[j])InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(&table.buffer[j].callback),reinterpret_cast<void*>(originals[j]),reinterpret_cast<void*>(forwards[j]));
   output<<"{\"event\":\"handler_changed_during_install\"}\n";output.flush();state=3;return 5;
  }
 }
 {std::lock_guard lock(mutex);installed=true;recording=true;state=4;}
 // Callbacks and DLL remain pinned. Keep one flusher alive across recording
 // pauses; reinstalling would capture our own forwarding callbacks recursively.
 for(;;){Sleep(500);std::lock_guard lock(mutex);output.flush();interaction.flush();}
}
}
extern "C" DWORD WINAPI StartCapture(void*){
 std::lock_guard lock(mutex);
 if(state==4)return 0;
 if(state==3&&installed){
  output<<"{\"event\":\"resumed\",\"tick_ms\":"<<GetTickCount64()<<"}\n";output.flush();
  recording=true;state=4;return 0;
 }
 unsigned expected=0;if(!state.compare_exchange_strong(expected,1))return 1;
 HANDLE t=CreateThread(nullptr,0,run,nullptr,0,nullptr);if(!t){state=3;return 2;}CloseHandle(t);return 0;
}
extern "C" DWORD WINAPI StopCapture(void*){
 std::lock_guard lock(mutex);
 if(state==1)return 1; // Do not race callback installation.
 if(state==4){recording=false;state=3;interaction.close();recent_packets.clear();output<<"{\"event\":\"paused\",\"tick_ms\":"<<GetTickCount64()<<"}\n";output.flush();}
 return 0;
}
extern "C" DWORD WINAPI CaptureStatus(void*){return state;}
BOOL WINAPI DllMain(HMODULE h,DWORD reason,void*){if(reason==DLL_PROCESS_ATTACH){self=h;DisableThreadLibraryCalls(h);}return TRUE;}

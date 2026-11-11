// Developer collector; /tp sends travel only on an explicit local command.
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <bit>
#include <functional>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <atomic>
#include <array>
#include <map>
#include <memory>
#include "name_json.h"
#include "interaction_context.h"
#include <cstdio>
#include <GWCA/Utilities/Debug.h>
#include "entry_tracker.h"
#include "local_observation_gate.h"
#include "transition_tracker.h"
#include "event_hook.h"
#include "chat_hook.h"
#include "tls_dispatch_guard.h"
#include <GWCA/Managers/EventMgr.h>
#include "travel_command.h"
#include "travel_request.h"
#include <GWCA/Utilities/Hooker.h>
#include "arrival_tracker.h"
#include "item_observation.h"
#include "npc_observation.h"
#include "quest_observation.h"
#include "quest_info_queue.h"
#include "quest_info_request.h"
#include "loot_message.h"
#include <GWCA/GameEntities/Quest.h>
#include <GWCA/Managers/QuestMgr.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/NPC.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/MerchantMgr.h>
#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/GameEntities/Map.h>
namespace {
HMODULE module;
HANDLE diagnostics=INVALID_HANDLE_VALUE;
std::atomic<bool> capture_ready{false};
GW::HookEntry tls_frame_hook;
void panic(void*,const char* expression,const char* file,unsigned line,const char* function) noexcept {
    char message[4096];
    const int n=snprintf(message,sizeof message,"GWCA panic: file=%.700s line=%u function=%.500s expression=%.2000s\r\n",
        file?file:"",line,function?function:"",expression?expression:"");
    if(diagnostics!=INVALID_HANDLE_VALUE&&n>0){DWORD written=0;
        WriteFile(diagnostics,message,static_cast<DWORD>((std::min)(n,int(sizeof(message)-1))),&written,nullptr);
        void* frames[32]{};const auto count=CaptureStackBackTrace(0,32,frames,nullptr);
        for(unsigned i=0;i<count;++i){
            HMODULE owner=nullptr;wchar_t path[512]{};
            if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(frames[i]),&owner))GetModuleFileNameW(owner,path,512);
            const int size=snprintf(message,sizeof message,"frame=%u module=%ls rva=0x%lx\r\n",i,path,
                static_cast<unsigned long>(reinterpret_cast<std::uintptr_t>(frames[i])-reinterpret_cast<std::uintptr_t>(owner)));
            if(size>0)WriteFile(diagnostics,message,static_cast<DWORD>((std::min)(size,int(sizeof(message)-1))),&written,nullptr);
        }
        FlushFileBuffers(diagnostics);
    }
    // Diagnostic only: do not suppress GWCA's fatal path or resume bad state.
}
GW::HookEntry hook;
GW::HookEntry travel_hook;
GW::HookEntry chat_hook;
std::atomic<bool> started=false, stop=false, finished=false;
std::atomic<unsigned> dropped=0, sample_errors=0;
std::mutex sampling;
std::mutex mutex;
std::vector<std::string> pending;
void travel_command(GW::HookStatus* status,const wchar_t*,int argc,const LPWSTR* argv){
    status->blocked=true;
    const auto feedback=[](const wchar_t* text){GW::Chat::WriteChat(GW::Chat::CHANNEL_WARNING,text,L"World Logger");};
    if(stop){feedback(L"The logger is stopped.");return;}
    const auto map=argc==2&&argv&&argv[1]?capture::travel_map_id(argv[1],static_cast<unsigned>(GW::Constants::MapID::Count)):std::nullopt;
    if(!map){feedback(L"Usage: /tp <map ID>.");return;}
    if(GW::Map::GetInstanceType()==GW::Constants::InstanceType::Loading){feedback(L"Wait until the map has finished loading.");return;}
    const auto destination=static_cast<GW::Constants::MapID>(*map);
    const auto region=GW::Map::GetRegion();
    const auto language=GW::Map::GetLanguage();
    const auto source=GW::Map::GetMapID();
    const auto image=reinterpret_cast<const std::uint8_t*>(GetModuleHandleW(nullptr));
    const bool compatible=capture::tls_guard::readable(reinterpret_cast<std::uintptr_t>(image+0x14dbae),32)
        &&capture::travel_layout_matches({image+0x14dbae,32});
    capture::TravelRequest request{static_cast<unsigned>(destination),static_cast<int>(region),static_cast<unsigned>(language),0};
    const bool accepted=compatible&&GW::UI::SendUIMessage(static_cast<GW::UI::UIMessage>(capture::ordinary_travel_message),&request);
    {std::lock_guard lock(mutex);pending.push_back("{\"event\":\"manual_travel_request\",\"map_id\":"+std::to_string(*map)+",\"source_map\":"+std::to_string(static_cast<unsigned>(source))+",\"region\":"+std::to_string(static_cast<int>(region))+",\"language\":"+std::to_string(static_cast<int>(language))+",\"district\":0,\"accepted\":"+(accepted?"true":"false")+"}");}
    if(!accepted)feedback(L"The client could not start that travel request.");
}
struct NameResult { std::wstring encoded,decoded; bool ready=false,published=false; std::mutex guard; };
// Entries are never erased: asynchronous decoder callbacks retain their address,
// even across zoning or StopCapture. The pinned DLL owns them until process exit.
std::map<std::wstring,std::unique_ptr<NameResult>> names;
bool copy_name(const wchar_t* source,std::wstring& out){
    if(!source)return false;
    for(size_t i=0;i<1024;++i){wchar_t c=0;SIZE_T read=0;
        if(!ReadProcessMemory(GetCurrentProcess(),source+i,&c,sizeof c,&read)||read!=sizeof c)return false;
        if(!c)return true;out+=c;
    }
    return false;
}
void __cdecl name_decoded(void* context,const wchar_t* value) noexcept {
    try {std::wstring decoded;bool terminated=false;
        if(!value)return;for(std::size_t i=0;i<8192;++i){wchar_t c=0;SIZE_T read=0;if(!ReadProcessMemory(GetCurrentProcess(),value+i,&c,sizeof c,&read)||read!=sizeof c)return;if(!c){terminated=true;break;}decoded+=c;}if(!terminated)return;
        auto& result=*static_cast<NameResult*>(context);
        std::lock_guard lock(result.guard);result.decoded=std::move(decoded);result.ready=true;
    }catch(...){++sample_errors;}
}
std::string observed_name(const wchar_t* source){
    std::wstring encoded;
    if(!copy_name(source,encoded)||encoded.empty())
        return ",\"name_encoded_utf16\":null,\"name\":null,\"name_status\":\"unavailable\"";
    auto found=names.find(encoded);
    if(found==names.end()&&names.size()<4096){
        auto result=std::make_unique<NameResult>();result->encoded=encoded;
        found=names.emplace(encoded,std::move(result)).first;
        GW::UI::AsyncDecodeStr(found->second->encoded.c_str(),name_decoded,found->second.get());
    }
    auto row=",\"name_encoded_utf16\":"+capture::encoded_name_json(encoded);
    if(found==names.end())return row+",\"name\":null,\"name_status\":\"cache_limit\"";
    std::lock_guard lock(found->second->guard);
    return row+",\"name\":"+(found->second->ready?capture::name_json(found->second->decoded):"null")
        +",\"name_status\":\""+(found->second->ready?"decoded":"pending")+"\"";
}
void publish_decoded_names(){
    // Decoder completion can outlive the map/entity that requested it. Keep a
    // session dictionary keyed by the exact encoded text, independent of zone.
    // Retry after queue saturation; mark published only after successful enqueue.
    for(auto& [encoded,result]:names){
        std::lock_guard result_lock(result->guard);
        if(!result->ready||result->published)continue;
        std::string row="{\"schema\":1,\"event\":\"name_decoded\",\"name_encoded_utf16\":"+
            capture::encoded_name_json(encoded)+",\"name\":"+capture::name_json(result->decoded)+"}";
        std::lock_guard queue_lock(mutex);
        if(pending.size()>=30)return;
        pending.push_back(std::move(row));result->published=true;
    }
}
std::uint64_t last=0,epoch=0;
void observe_loot_log(std::uint64_t now){
 // Read-only getter5258 reads initialized Chat pointer53b1c; no hooks or UI
 // forwarding changes. Ignore history present at collector startup.
 static std::array<std::pair<std::uintptr_t,std::uint64_t>,512> seen{};
 static bool initialized=false;
 auto* source=GW::Chat::GetChatLog();GW::Chat::ChatBuffer log{};SIZE_T copied=0;
 if(!source||!ReadProcessMemory(GetCurrentProcess(),source,&log,sizeof log,&copied)||copied!=sizeof log)return;
 for(std::size_t i=0;i<512;++i){
  auto* message=log.messages[i];if(!message)continue;
  struct Header{std::uint32_t channel,unknown;FILETIME timestamp;} h{};
  if(!ReadProcessMemory(GetCurrentProcess(),message,&h,sizeof h,&copied)||copied!=sizeof h)continue;
  const auto stamp=(std::uint64_t(h.timestamp.dwHighDateTime)<<32)|h.timestamp.dwLowDateTime;
  const auto identity=std::pair<std::uintptr_t,std::uint64_t>{reinterpret_cast<std::uintptr_t>(message),stamp};
  if(seen[i]==identity)continue;const auto previous=seen[i];seen[i]=identity;if(!initialized)continue;
  // Only system channels; never inspect normal chat payloads or whispers.
  if(h.channel!=7&&h.channel!=10&&h.channel!=13)continue;
  std::wstring encoded;if(!copy_name(message->message,encoded))continue;
  const auto report=capture::loot::parse(encoded);if(!report)continue;
  std::string row="{\"schema\":1,\"event\":\"loot_notification\",\"tick_ms\":"+std::to_string(now)+
   ",\"timestamp_filetime\":"+std::to_string(stamp)+",\"map_id\":"+std::to_string(static_cast<unsigned>(GW::Map::GetMapID()))+
   ",\"map_epoch\":"+std::to_string(epoch)+",\"template_id\":"+std::to_string(report->template_id)+
   ",\"encoded_template\":"+capture::encoded_name_json(report->encoded)+",\"encoded_template_recipient_removed\":true"+
   ",\"source_agent_id\":null,\"confidence\":\"report_name_only\",\"source_reported_name\":{";
  auto named=observed_name(report->source_atom.c_str());row+=named.substr(1)+"},\"item_reported_name\":{";
  named=observed_name(report->item_atom.c_str());row+=named.substr(1)+"}}";
  std::lock_guard lock(mutex);if(pending.size()<30)pending.push_back(std::move(row));else seen[i]=previous; // retry bounded ring entry
 }
 initialized=true;
}
std::uint32_t old_map=0,old_time=0;
bool was_loaded=false;
capture::EntryTracker entry;
capture::TransitionTracker transitions;
capture::ArrivalTracker arrivals;
std::map<unsigned,std::string> merchant_previous;
std::uint64_t merchant_epoch=0;
std::string equipment_previous;
std::uint64_t equipment_epoch=0;
std::map<std::uint32_t,std::string> npc_definitions_previous;
std::uint64_t npc_definitions_epoch=0;
void sample_npc_definition(std::uint32_t id,std::uint32_t map,std::uint32_t time){
    if(npc_definitions_epoch!=epoch){npc_definitions_previous.clear();npc_definitions_epoch=epoch;}
    const auto* table=GW::Agents::GetNPCArray();
    if(!table||!table->valid()||id>=table->size()||table->size()>65536)return;
    GW::NPC npc{};SIZE_T read=0;
    if(!ReadProcessMemory(GetCurrentProcess(),&(*table)[id],&npc,sizeof npc,&read)||read!=sizeof npc)return;
    // Bounds protect the collector from a changed layout or partial table entry.
    // Do not emit a truncated model-file list as though it were complete.
    if(npc.files_count>npc.files_capacity||npc.files_count>64||
        (npc.files_count&&!npc.model_files))return;
    capture::NpcDefinition value;
    value.model_file=npc.model_file_id;value.skin_file=npc.skin_file_id;
    value.appearance=npc.appearance;value.flags=npc.npc_flags;
    value.primary=static_cast<std::uint32_t>(npc.primary);
    value.secondary=static_cast<std::uint32_t>(npc.secondary);value.level=npc.default_level;
    static_assert(sizeof npc.visual_adjustment==4);
    std::memcpy(value.visual_adjustment.data(),&npc.visual_adjustment,4);
    value.model_files.resize(npc.files_count);
    const auto bytes=value.model_files.size()*sizeof(std::uint32_t);
    if(bytes&&(!ReadProcessMemory(GetCurrentProcess(),npc.model_files,value.model_files.data(),bytes,&read)||read!=bytes))return;
    // Name is from the NPC definition, distinct from an agent's override name.
    auto definition=capture::npc_definition_json(value);
    definition.pop_back();definition+=observed_name(npc.name_enc)+"}";
    const auto found=npc_definitions_previous.find(id);
    if(found!=npc_definitions_previous.end()&&found->second==definition)return;
    std::string row="{\"schema\":1,\"event\":\"npc_definition_observed\",\"map\":"+std::to_string(map)+
        ",\"instance_epoch\":"+std::to_string(epoch)+",\"instance_ms\":"+std::to_string(time)+
        ",\"npc_table_id\":"+std::to_string(id)+",\"definition\":"+definition+"}";
    std::lock_guard lock(mutex);
    if(pending.size()<30){pending.push_back(std::move(row));npc_definitions_previous[id]=std::move(definition);}
    else ++dropped;
}
std::string observe_item(const GW::Item& item){
    capture::ItemObservation observation;
    observation.model=item.model_id;observation.file=item.model_file_id;
    observation.interaction=item.interaction;observation.type=static_cast<unsigned>(item.type);
    observation.value=item.value;observation.quantity=item.quantity;
    observation.profession=item.profession;observation.slot=item.slot;
    observation.tint=item.dye.dye_tint;
    observation.dye1=static_cast<unsigned>(item.dye.dye1);
    observation.dye2=static_cast<unsigned>(item.dye.dye2);
    observation.dye3=static_cast<unsigned>(item.dye.dye3);
    observation.dye4=static_cast<unsigned>(item.dye.dye4);
    std::vector<std::uint32_t> modifiers;
    bool complete=item.mod_struct_size==0;
    if(item.mod_struct&&item.mod_struct_size&&item.mod_struct_size<=64){
        modifiers.resize(item.mod_struct_size);SIZE_T read=0;
        const auto bytes=modifiers.size()*sizeof(std::uint32_t);
        complete=ReadProcessMemory(GetCurrentProcess(),item.mod_struct,modifiers.data(),bytes,&read)&&read==bytes;
        if(!complete)modifiers.clear();
    }
    auto row=capture::item_json(observation,modifiers,complete);
    row.pop_back();row+=observed_name(item.name_enc);return row+"}";
}
void sample_equipment(std::uint32_t map,std::uint32_t time){
    if(equipment_epoch!=epoch){equipment_previous.clear();equipment_epoch=epoch;}
    auto inventory=GW::Items::GetInventory();if(!inventory)return;
    auto bag=inventory->equipped_items;
    if(!bag||bag->bag_type!=GW::Constants::BagType::Equipped||!bag->items.valid()||bag->items.size()>64)return;
    auto local=GW::Agents::GetControlledCharacter();
    if(!local||local->agent_model_type!=0x3000)return;
    std::string items="[";bool first=true;
    for(auto item:bag->items){
        if(!item)continue;
        if(!first)items+=",";first=false;items+=observe_item(*item);
    }
    items+="]";
    const auto context=",\"character_level\":"+std::to_string(local->level)
        +",\"primary_profession\":"+std::to_string(static_cast<unsigned>(local->primary))
        +",\"secondary_profession\":"+std::to_string(static_cast<unsigned>(local->secondary));
    const auto fingerprint=context+items;
    if(fingerprint==equipment_previous)return;
    std::string row="{\"schema\":1,\"event\":\"local_equipment_observed\",\"map\":"+std::to_string(map)
        +",\"instance_epoch\":"+std::to_string(epoch)+",\"instance_ms\":"+std::to_string(time)
        +context+",\"starter_equipment_verified\":false,\"items\":"+items+"}";
    std::lock_guard lock(mutex);
    if(pending.size()<30){pending.push_back(std::move(row));equipment_previous=fingerprint;}else ++dropped;
}
std::map<unsigned,std::string> inventory_previous;
std::uint64_t inventory_epoch=0;
void sample_inventory(std::uint32_t map,std::uint32_t time){
    if(inventory_epoch!=epoch){inventory_previous.clear();inventory_epoch=epoch;}
    // Only installed, readable containers: unloaded bank panes are not empty.
    for(unsigned id=1;id<static_cast<unsigned>(GW::Constants::Bag::Max);++id){
        auto bag=GW::Items::GetBag(static_cast<GW::Constants::Bag>(id));
        if(!bag||static_cast<unsigned>(bag->bag_id())!=id||!bag->items.valid()||bag->items.size()>512)continue;
        std::string items="[";bool first=true;unsigned omitted=0;
        for(unsigned slot=0;slot<bag->items.size();++slot){
            auto item=bag->items[slot];if(!item)continue;
            if(item->bag!=bag||item->slot!=slot||!item->item_id){++omitted;continue;}
            if(!first)items+=",";first=false;
            auto row=observe_item(*item);row.pop_back();
            items+=row+",\"observed_item_id\":"+std::to_string(item->item_id)+"}";
        }
        items+="]";
        // container_item is a pointer, not an item ID. Read it before any
        // dereference, then verify the same table/backlink used by items_logger.
        // Missing/unreadable is distinct from an observed absent container.
        std::string container="null",container_status="absent";
        if(bag->container_item){
            GW::Item copy{};SIZE_T bytes=0;
            auto pointer=reinterpret_cast<GW::Item*>(bag->container_item);
            if(ReadProcessMemory(GetCurrentProcess(),pointer,&copy,sizeof(copy),&bytes)&&bytes==sizeof(copy)&&
               capture::installed_container_valid(copy.item_id,static_cast<unsigned>(copy.type),copy.quantity,
                   GW::Items::GetItemById(copy.item_id)==pointer,copy.bag!=nullptr,copy.bag_equipped==bag)){
                container=observe_item(copy);container.pop_back();
                container+=",\"observed_item_id\":"+std::to_string(copy.item_id)+"}";
                container_status="item_table_exact_installed_bag_backlink_no_slot";
            }else container_status="unreadable_or_unverified";
        }
        const auto fingerprint=std::to_string(static_cast<unsigned>(bag->bag_type))+":"+std::to_string(bag->items.size())+":"+std::to_string(omitted)+":"+items+":"+container_status+":"+container;
        if(auto previous=inventory_previous.find(id);previous!=inventory_previous.end()&&previous->second==fingerprint)continue;
        std::string row="{\"schema\":1,\"event\":\"local_inventory_observed\",\"map\":"+std::to_string(map)
            +",\"instance_epoch\":"+std::to_string(epoch)+",\"instance_ms\":"+std::to_string(time)
            +",\"bag_id\":"+std::to_string(id)+",\"bag_type\":"+std::to_string(static_cast<unsigned>(bag->bag_type))
            +",\"slot_capacity\":"+std::to_string(bag->items.size())
            +",\"omitted_item_count\":"+std::to_string(omitted)
            +",\"all_storage_panes_loaded\":false,\"container_status\":\""+container_status+"\",\"container_definition\":"+container+",\"items\":"+items+"}";
        std::lock_guard lock(mutex);
        if(pending.size()<30){pending.push_back(std::move(row));inventory_previous[id]=fingerprint;}else ++dropped;
    }
}
std::map<unsigned,std::string> ground_previous;
std::uint64_t ground_epoch=0;
void sample_ground_items(std::uint32_t map,std::uint32_t time,const GW::AgentArray& agents){
    if(ground_epoch!=epoch){ground_previous.clear();ground_epoch=epoch;}
    std::map<unsigned,std::string> current;
    for(auto agent:agents){
        auto drop=agent?agent->GetAsAgentItem():nullptr;
        if(!drop||!drop->item_id||!std::isfinite(drop->pos.x)||!std::isfinite(drop->pos.y))continue;
        auto item=GW::Items::GetItemById(drop->item_id);
        const bool valid=item&&item->item_id==drop->item_id&&item->agent_id==drop->agent_id&&!item->bag;
        auto definition=valid?observe_item(*item):"null";
        const auto fields=",\"agent_id\":"+std::to_string(drop->agent_id)+",\"observed_item_id\":"+std::to_string(drop->item_id)
            +",\"x\":"+std::to_string(drop->pos.x)+",\"y\":"+std::to_string(drop->pos.y)
            +",\"plane\":"+std::to_string(drop->pos.zplane)
            +",\"drop_source_agent\":null,\"drop_source_status\":\"not_exposed_by_ground_snapshot\",\"definition\":"+definition;
        current[drop->agent_id]=fields;
        if(auto old=ground_previous.find(drop->agent_id);old!=ground_previous.end()&&old->second==fields)continue;
        std::string row="{\"schema\":1,\"event\":\"ground_item_observed\",\"map\":"+std::to_string(map)
            +",\"instance_epoch\":"+std::to_string(epoch)+",\"instance_ms\":"+std::to_string(time)+fields+"}";
        std::lock_guard lock(mutex);
        if(pending.size()<30){pending.push_back(std::move(row));ground_previous[drop->agent_id]=fields;}else ++dropped;
    }
    // Disappearance does not prove pickup: visibility and despawn are possible.
    for(auto it=ground_previous.begin();it!=ground_previous.end();){
        if(current.contains(it->first)){++it;continue;}
        std::string row="{\"schema\":1,\"event\":\"ground_item_no_longer_observed\",\"map\":"+std::to_string(map)
            +",\"instance_epoch\":"+std::to_string(epoch)+",\"instance_ms\":"+std::to_string(time)
            +",\"agent_id\":"+std::to_string(it->first)+",\"pickup_confirmed\":false}";
        std::lock_guard lock(mutex);
        if(pending.size()<30){pending.push_back(std::move(row));it=ground_previous.erase(it);}else{++dropped;++it;}
    }
}
void sample_merchants(std::uint32_t map,std::uint32_t time){
    if(merchant_epoch!=epoch){merchant_previous.clear();merchant_epoch=epoch;}
    using Type=GW::Merchant::TransactionType;
    for(auto type:{Type::MerchantBuy,Type::CollectorBuy,Type::CrafterBuy,Type::SkillTrainer,Type::TraderBuy}){
        const auto count=GW::Merchant::GetMerchantItems(type);
        if(count>4096)continue;
        std::vector<std::uint32_t> ids(count);
        if(count&&GW::Merchant::GetMerchantItems(type,ids.size(),ids.data())!=count)continue;
        std::string items="[";bool first=true;
        for(auto id:ids){
            auto item=GW::Items::GetItemById(id);if(!item)continue;
            if(!first)items+=",";first=false;
            items+=observe_item(*item);
        }
        items+="]";auto key=static_cast<unsigned>(type);auto previous=merchant_previous.find(key);
        if(previous==merchant_previous.end()&&ids.empty())continue;
        if(previous!=merchant_previous.end()&&previous->second==items)continue;
        std::string row="{\"schema\":1,\"event\":\"merchant_inventory_observed\",\"map\":"+std::to_string(map)+",\"instance_epoch\":"+std::to_string(epoch)+",\"instance_ms\":"+std::to_string(time)+",\"transaction_type\":"+std::to_string(key)+",\"vendor_identity_confirmed\":false,\"quoted_prices_included\":false,\"items\":"+items+"}";
        std::lock_guard lock(mutex);
        if(pending.size()<30){pending.push_back(std::move(row));merchant_previous[key]=std::move(items);}else ++dropped;
    }
}
capture::QuestSnapshotTracker quest_snapshots;
capture::NpcVisibilityTracker quest_npc_visibility;
std::uint64_t quest_context_epoch=0;
std::uint32_t quest_context_actor=0;
std::string quest_text(const wchar_t* source){
 std::wstring encoded;
 if(!source)return "{\"encoded_utf16\":null,\"decoded\":null,\"status\":\"absent\"}";
 bool ended=false;
 for(std::size_t i=0;i<8192;++i){wchar_t c=0;SIZE_T read=0;
  if(!ReadProcessMemory(GetCurrentProcess(),source+i,&c,sizeof c,&read)||read!=sizeof c)break;
  if(!c){ended=true;break;}encoded+=c;
 }
 if(!ended)return "{\"encoded_utf16\":null,\"decoded\":null,\"status\":\"unreadable_or_over_bound\"}";
 if(encoded.empty())return "{\"encoded_utf16\":[],\"decoded\":\"\",\"status\":\"empty\"}";
 auto found=names.find(encoded);
 if(found==names.end()&&names.size()<4096){auto result=std::make_unique<NameResult>();result->encoded=encoded;
  found=names.emplace(encoded,std::move(result)).first;GW::UI::AsyncDecodeStr(found->second->encoded.c_str(),name_decoded,found->second.get());}
 std::string row="{\"encoded_utf16\":"+capture::encoded_name_json(encoded);
 if(found==names.end())return row+",\"decoded\":null,\"status\":\"cache_limit\"}";
 std::lock_guard lock(found->second->guard);
 return row+",\"decoded\":"+(found->second->ready?capture::name_json(found->second->decoded):"null")+",\"status\":\""+(found->second->ready?"decoded":"pending")+"\"}";
}
capture::QuestInfoQueue quest_info_queue;
bool sample_quests(std::uint32_t map,std::uint32_t time,std::uint32_t actor){
 const auto sampled_tick=GetTickCount64();
 auto quests=GW::QuestMgr::GetQuestLog();
 if(!quests||!quests->valid()||quests->size()>1024)return false;
 std::map<std::uint32_t,std::string> entries;std::map<std::uint32_t,bool> details_loaded;std::size_t bytes=0;
 for(std::size_t i=0;i<quests->size();++i){GW::Quest q{};SIZE_T read=0;
  if(!ReadProcessMemory(GetCurrentProcess(),&(*quests)[i],&q,sizeof q,&read)||read!=sizeof q)return false;
  const bool marker_finite=std::isfinite(q.marker.x)&&std::isfinite(q.marker.y);
  auto id=static_cast<std::uint32_t>(q.quest_id);
  details_loaded.emplace(id,(q.log_state&1)&&q.description&&q.objectives);
  std::string row="{\"quest_id\":"+std::to_string(id)+",\"log_state\":"+std::to_string(q.log_state)
   +",\"completed\":"+((q.log_state&2)?"true":"false")+",\"current_mission\":"+((q.log_state&16)?"true":"false")
   +",\"primary\":"+((q.log_state&32)?"true":"false")+",\"area_primary\":"+((q.log_state&64)?"true":"false")
   +",\"map_from\":"+std::to_string(static_cast<unsigned>(q.map_from))+",\"map_to\":"+std::to_string(static_cast<unsigned>(q.map_to))
   +",\"marker\":{\"x\":"+(std::isfinite(q.marker.x)?std::to_string(q.marker.x):"null")+",\"y\":"+(std::isfinite(q.marker.y)?std::to_string(q.marker.y):"null")+",\"plane\":"+std::to_string(q.marker.zplane)
   +",\"x_bits\":"+std::to_string(std::bit_cast<std::uint32_t>(q.marker.x))+",\"y_bits\":"+std::to_string(std::bit_cast<std::uint32_t>(q.marker.y))+",\"finite\":"+(marker_finite?"true":"false")+"}"
   +",\"unknown_0024\":"+std::to_string(q.h0024)+",\"category\":"+quest_text(q.location)+",\"name\":"+quest_text(q.name)
   +",\"npc\":"+quest_text(q.npc)+",\"description\":"+quest_text(q.description)+",\"objectives\":"+quest_text(q.objectives)+"}";
  bytes+=row.size();if(bytes>8*1024*1024||!entries.emplace(id,std::move(row)).second)return false;
 }
 const auto game=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
 const auto sender=game+capture::quest_info_sender_rva;
 const bool binding_ready=capture::tls_guard::readable(sender,43)&&capture::quest_info_sender_matches({reinterpret_cast<const std::uint8_t*>(sender),43});
 if(const auto requested=quest_info_queue.next(epoch,actor,sampled_tick,details_loaded,binding_ready)){
  reinterpret_cast<void(__cdecl*)(std::uint32_t)>(sender)(*requested);
  std::lock_guard lock(mutex);
  const auto row="{\"schema\":1,\"event\":\"quest_info_requested\",\"quest_id\":"+std::to_string(*requested)+",\"map\":"+std::to_string(map)+",\"instance_epoch\":"+std::to_string(epoch)+",\"local_actor\":"+std::to_string(actor)+",\"process_id\":"+std::to_string(GetCurrentProcessId())+",\"tick_ms\":"+std::to_string(sampled_tick)+",\"response_received\":false}";
  if(pending.size()<30)pending.push_back(row);else ++dropped;
 }
 auto active=static_cast<unsigned>(GW::QuestMgr::GetActiveQuestId());
 auto proposed=quest_snapshots;
 bool new_context=quest_context_epoch!=epoch||quest_context_actor!=actor;
 if(new_context)proposed.known=false;
 if(proposed.observe(epoch,active,entries)){
  std::string row="{\"schema\":1,\"event\":\"quest_snapshot_changed\",\"process_id\":"+std::to_string(GetCurrentProcessId())+",\"tick_ms\":"+std::to_string(sampled_tick)+",\"map\":"+std::to_string(map)+",\"instance_epoch\":"+std::to_string(epoch)
   +",\"instance_ms\":"+std::to_string(time)+",\"local_actor\":"+std::to_string(actor)+",\"quest_snapshot_revision\":"+std::to_string(proposed.revision)
   +",\"active_quest_id\":"+std::to_string(active)+",\"full_snapshot\":true,\"absence_does_not_prove_abandonment\":true,\"quests\":[";
  bool first=true;for(auto& [id,value]:entries){if(!first)row+=",";first=false;row+=value;}row+="]}";
  std::lock_guard lock(mutex);if(pending.size()>=30){++dropped;return false;}pending.push_back(std::move(row));quest_snapshots=std::move(proposed);
  quest_context_epoch=epoch;quest_context_actor=actor;
 }
 return true;
}
void sample_quest_npc_visibility(std::uint32_t map,std::uint32_t time,const GW::AgentArray& agents){
 const auto sampled_tick=GetTickCount64();
 std::map<std::uint32_t,capture::NpcVisibilityTracker::State> current;
 for(auto a:agents){auto npc=a?a->GetAsAgentLiving():nullptr;if(npc&&npc->agent_model_type==0x2000)current.emplace(npc->agent_id,capture::NpcVisibilityTracker::State{npc->player_number,npc->GetIsDead()});}
 auto proposed=quest_npc_visibility;auto deltas=proposed.observe(epoch,current);if(deltas.empty())return;
 std::string row="{\"schema\":1,\"event\":\"npc_visibility_changed\",\"process_id\":"+std::to_string(GetCurrentProcessId())+",\"tick_ms\":"+std::to_string(sampled_tick)+",\"map\":"+std::to_string(map)+",\"instance_epoch\":"+std::to_string(epoch)
  +",\"instance_ms\":"+std::to_string(time)+",\"quest_snapshot_revision\":"+std::to_string(quest_snapshots.revision)
  +",\"correlation_only\":true,\"spawn_or_death_not_proven\":true,\"changes\":[";
 bool first=true;for(auto d:deltas){if(!first)row+=",";first=false;row+="{\"agent\":"+std::to_string(d.actor)+",\"model\":"+std::to_string(d.model)+",\"visible\":"+(d.appeared?"true":"false")+",\"observed_dead\":"+(d.dead?"true":"false")+",\"status_changed_without_visibility_change\":"+(d.status_changed?"true":"false")+"}";}row+="]}";
 std::lock_guard lock(mutex);if(pending.size()>=30){++dropped;return;}pending.push_back(std::move(row));quest_npc_visibility=std::move(proposed);
}
void sample_impl(){
    auto now=GetTickCount64();
    // Publish selected NPC identity without new UI/chat detours. The packet
    // recorder only uses it when the server's dialog sender matches this agent.
    static HANDLE context_handle=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(capture::InteractionContext),capture::interaction_mapping().c_str());
    static auto* context=context_handle?static_cast<capture::InteractionContext*>(MapViewOfFile(context_handle,FILE_MAP_WRITE,0,0,sizeof(capture::InteractionContext))):nullptr;
    if(context){
        capture::InteractionContext snapshot{};snapshot.tick=now;
        if(GW::Map::GetIsMapLoaded()){
            snapshot.map=static_cast<unsigned>(GW::Map::GetMapID());
            const auto* target=GW::Agents::GetTargetAsAgentLiving();
            if(target&&target->agent_model_type==0x2000){
                snapshot.agent=target->agent_id;
                const auto* encoded=GW::Agents::GetAgentEncName(target);
                (void)observed_name(encoded);
                std::wstring key;
                if(copy_name(encoded,key))if(auto it=names.find(key);it!=names.end()){
                    std::lock_guard lock(it->second->guard);
                    if(it->second->ready)wcsncpy_s(snapshot.name,it->second->decoded.c_str(),_TRUNCATE);
                }
            }
        }
        InterlockedIncrement(&context->sequence);
        memcpy(reinterpret_cast<char*>(context)+sizeof(LONG),reinterpret_cast<char*>(&snapshot)+sizeof(LONG),sizeof(snapshot)-sizeof(LONG));
        InterlockedIncrement(&context->sequence);
    }
    static std::uint64_t last_names=0;
    if(now-last_names>=1000){last_names=now;publish_decoded_names();observe_loot_log(now);}
    const bool loaded=GW::Map::GetIsMapLoaded();
    if(!loaded){entry.observe(false,false,false,now);transitions.observe(false,{},now);was_loaded=false;return;}
    auto map=static_cast<std::uint32_t>(GW::Map::GetMapID());auto time=GW::Map::GetInstanceTime();
    if(!was_loaded||map!=old_map||time<old_time)++epoch;
    was_loaded=true;old_map=map;old_time=time;
    const bool map_observing=GW::Map::GetIsObserving();
    auto local=map_observing?nullptr:GW::Agents::GetControlledCharacter();
    const bool valid=local&&capture::valid_local_observation(map_observing,local->agent_id,local->agent_model_type,local->pos.x,local->pos.y,local->rotation_angle);
    std::optional<capture::TransitionPoint> point;
    if(valid)point=capture::TransitionPoint{map,time,local->pos.zplane,epoch,now,local->pos.x,local->pos.y,local->rotation_angle};
    if(auto transition=transitions.observe(true,point,now)){
        auto row=capture::transition_json(*transition);
        std::lock_guard lock(mutex);if(pending.size()<30)pending.push_back(std::move(row));else ++dropped;
    }
    if(entry.observe(true,valid,local&&local->GetIsMoving(),now)){
        std::string row="{\"schema\":1,\"event\":\"local_entry_candidate\",\"map\":"+std::to_string(map)+",\"instance_epoch\":"+std::to_string(epoch)+",\"instance_ms\":"+std::to_string(time)+",\"x\":"+std::to_string(local->pos.x)+",\"y\":"+std::to_string(local->pos.y)+",\"plane\":"+std::to_string(local->pos.zplane)+",\"facing\":"+std::to_string(local->rotation_angle)+",\"source\":\"observed_loading_transition\",\"verified_spawn\":false}";
        std::lock_guard lock(mutex);if(pending.size()<30)pending.push_back(std::move(row));else ++dropped;
    }
    auto agents=GW::Agents::GetAgentArray();if(!agents||!agents->valid()||agents->size()>65536)return;
    if(valid){
        std::vector<std::uint32_t> ids;
        for(auto agent:*agents){auto player=agent?agent->GetAsAgentLiving():nullptr;
            if(player&&player->agent_model_type==0x3000)ids.push_back(player->agent_id);}
        for(auto id:arrivals.observe(epoch,ids,local->agent_id)){
            auto player=GW::Agents::GetAgentByID(id);
            if(!player||!std::isfinite(player->pos.x)||!std::isfinite(player->pos.y)||!std::isfinite(player->rotation_angle))continue;
            std::string row="{\"schema\":1,\"event\":\"other_player_first_seen\",\"map\":"+std::to_string(map)+",\"instance_epoch\":"+std::to_string(epoch)+",\"instance_ms\":"+std::to_string(time)+",\"x\":"+std::to_string(player->pos.x)+",\"y\":"+std::to_string(player->pos.y)+",\"plane\":"+std::to_string(player->pos.zplane)+",\"facing\":"+std::to_string(player->rotation_angle)+",\"arrival_confirmed\":false,\"verified_spawn\":false}";
            std::lock_guard lock(mutex);if(pending.size()<30)pending.push_back(std::move(row));else ++dropped;
        }
    }
    if(now-last<1000)return;last=now;
    if(valid&&sample_quests(map,time,local->agent_id))sample_quest_npc_visibility(map,time,*agents);
    sample_merchants(map,time);
    sample_ground_items(map,time,*agents);
    // Gadgets include chests, but an unknown gadget must not be labelled a chest.
    // Record complete visible snapshots, including empty ones, per instance.
    std::string gadgets="{\"schema\":1,\"event\":\"gadget_observations\",\"map\":"+std::to_string(map)+",\"instance_epoch\":"+std::to_string(epoch)+",\"instance_ms\":"+std::to_string(time)+",\"observed_positions_not_spawns\":true,\"visibility_is_not_full_map_coverage\":true,\"gadgets\":[";
    bool first_gadget=true;
    for(auto agent:*agents){
        auto gadget=agent?agent->GetAsAgentGadget():nullptr;
        if(!gadget||!std::isfinite(gadget->pos.x)||!std::isfinite(gadget->pos.y)||!std::isfinite(gadget->rotation_angle))continue;
        if(!first_gadget)gadgets+=",";first_gadget=false;
        gadgets+="{\"agent\":"+std::to_string(gadget->agent_id)+",\"gadget_id\":"+std::to_string(gadget->gadget_id)+",\"extra_type_raw\":"+std::to_string(gadget->extra_type)+",\"x\":"+std::to_string(gadget->pos.x)+",\"y\":"+std::to_string(gadget->pos.y)+",\"plane\":"+std::to_string(gadget->pos.zplane)+",\"facing\":"+std::to_string(gadget->rotation_angle)+",\"classification\":\"unverified_gadget\"}";
    }
    gadgets+="]}";
    {std::lock_guard lock(mutex);if(pending.size()<30)pending.push_back(std::move(gadgets));else ++dropped;}
    if(valid){sample_equipment(map,time);sample_inventory(map,time);}
    std::string s="{\"schema\":1,\"event\":\"npc_observations\",\"map\":"+std::to_string(map)+",\"instance_epoch\":"+std::to_string(epoch)+",\"instance_ms\":"+std::to_string(time)+",\"observed_positions_not_spawns\":true,\"npcs\":[";
    bool first=true;
    for(auto agent:*agents){
        auto npc=agent?agent->GetAsAgentLiving():nullptr;
        if(!npc||npc->agent_model_type!=0x2000)continue;
        sample_npc_definition(npc->player_number,map,time);
        if(!std::isfinite(npc->pos.x)||!std::isfinite(npc->pos.y)||!std::isfinite(npc->rotation_angle))continue;
        if(!first)s+=",";first=false;
        s+="{\"agent\":"+std::to_string(npc->agent_id)+",\"model\":"+std::to_string(npc->player_number)+",\"x\":"+std::to_string(npc->pos.x)+",\"y\":"+std::to_string(npc->pos.y)+",\"plane\":"+std::to_string(npc->pos.zplane)+",\"facing\":"+std::to_string(npc->rotation_angle)+",\"level\":"+std::to_string(npc->level)+",\"allegiance\":"+std::to_string(static_cast<unsigned>(npc->allegiance))+observed_name(GW::Agents::GetAgentEncName(npc))+"}";
    }
    s+="]}";
    std::lock_guard lock(mutex);
    if(pending.size()<30)pending.push_back(std::move(s));else ++dropped;
}
void sample(GW::HookStatus*) noexcept {
    try {
        std::lock_guard lock(sampling);
        if(stop)return;
        sample_impl();
    } catch(...) { ++sample_errors;stop=true; }
}
void initialization_observer(const char* name,bool begin,bool retained)noexcept{
 if(diagnostics==INVALID_HANDLE_VALUE)return;char text[256];const auto length=snprintf(text,sizeof text,"module_%s name=%.80s retained=%u\r\n",begin?"begin":"end",name,unsigned(retained));
 if(length>0){DWORD written=0;WriteFile(diagnostics,text,static_cast<DWORD>((std::min)(length,int(sizeof(text)-1))),&written,nullptr);FlushFileBuffers(diagnostics);}
}
LONG CALLBACK initialization_exception(EXCEPTION_POINTERS* exception)noexcept{
 if(!capture::tls_guard::initialization_stage||diagnostics==INVALID_HANDLE_VALUE||!exception||!exception->ExceptionRecord)return EXCEPTION_CONTINUE_SEARCH;
 char text[512];HMODULE owner=nullptr;wchar_t path[260]{};const auto address=exception->ExceptionRecord->ExceptionAddress;
 if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(address),&owner))GetModuleFileNameW(owner,path,260);
 const auto length=snprintf(text,sizeof text,"initialization_exception code=0x%08lx stage=%u module=%ls rva=0x%lx\r\n",exception->ExceptionRecord->ExceptionCode,capture::tls_guard::initialization_stage.load(),path,static_cast<unsigned long>(reinterpret_cast<uintptr_t>(address)-reinterpret_cast<uintptr_t>(owner)));
 if(length>0){DWORD written=0;WriteFile(diagnostics,text,static_cast<DWORD>((std::min)(length,int(sizeof(text)-1))),&written,nullptr);FlushFileBuffers(diagnostics);}
 return EXCEPTION_CONTINUE_SEARCH; // Diagnostics only, never resume an unsafe state.
}
DWORD run_impl(){
    wchar_t location[32768];GetModuleFileNameW(module,location,32768);
    auto root=std::filesystem::path(location).parent_path();
    auto directory=root;std::filesystem::create_directories(directory);
    auto path=directory/(L"world-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L".jsonl");
    auto diagnostic_path=path;diagnostic_path.replace_extension(L"diagnostic.txt");
    diagnostics=CreateFileW(diagnostic_path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    GW::RegisterPanicHandler(panic,nullptr);
    std::ofstream output(path);
    if(!output)return 1;
    output<<"{\"event\":\"collector_start\",\"gwca_revision\":\"1ce867e5557622408132495b92766ba3e59c1b76\"}\n";output.flush();
    output<<"{\"event\":\"initialization_begin\"}\n";output.flush();
    capture::tls_guard::install(GetModuleHandleW(L"gwca.dll"),GetModuleHandleW(nullptr));
    output<<"{\"event\":\"initialization_guards_installed\",\"dispatcher_detour\":true,\"bootstrap_detour\":true,\"unused_stoc_initializer_guard\":true}\n";output.flush();
    capture::tls_guard::observe_initialization=&initialization_observer;
    capture::tls_guard::install_snapshot_modules();
    output<<"{\"event\":\"initialization_modules_configured\",\"retained\":9,\"omitted\":11,\"module_stage_diagnostics\":true}\n";output.flush();
    const auto exception_observer=AddVectoredExceptionHandler(1,&initialization_exception);
    struct RemoveExceptionObserver{PVOID handle;~RemoveExceptionObserver(){if(handle)RemoveVectoredExceptionHandler(handle);}} remove_exception_observer{exception_observer};
    if(!GW::Initialize()){capture::tls_guard::inert=true;output<<"{\"event\":\"initialization_failed\"}\n";return 2;}
    if(!capture::tls_guard::stoc_snapshot_safe()){
        capture::tls_guard::inert=true;output<<"{\"event\":\"initialization_failed\",\"reason\":\"unused_stoc_guard_contract_failed\",\"skipped_initializers\":"<<capture::tls_guard::skipped_stoc_initializers.load()<<"}\n";output.flush();return 2;
    }
    output<<"{\"event\":\"gwca_initialize_returned\",\"unused_stoc_initializers_skipped\":1,\"native_decoder_untouched\":true}\n";output.flush();
    // Observational capture needs only the game-thread callback. Explicitly
    // disable any hooks initialized by GWCA before enabling this one module.
    capture::tls_guard::disable_observation_modules();
    capture::tls_guard::enable();
    // RenderFinished can run on a render thread lacking the game's static TLS.
    // Keep its queued bootstrap deferred; the frame pump offers a TLS-validated
    // dispatch opportunity without claiming an arbitrary thread is game-owned.
    GW::EventMgr::RegisterEventCallback(&tls_frame_hook,GW::EventMgr::EventID::kFrameTick,
        [](GW::HookStatus*,GW::EventMgr::EventID,void*,uint32_t){capture::tls_guard::dispatch();});
    GW::GameThread::EnableHooks();
    const auto bootstrap_event=capture::event_dispatch_target(GetModuleHandleW(L"gwca.dll"));
    GW::Hook::EnableHooks(GW::Hook::HookTarget(reinterpret_cast<void*>(bootstrap_event)));
    const auto bootstrap_deadline=GetTickCount64()+8000;
    while(!stop&&!capture::tls_guard::bootstrapped()&&GetTickCount64()<bootstrap_deadline)Sleep(10);
    if(stop||!capture::tls_guard::bootstrapped()){
        capture::tls_guard::inert=true;
        output<<"{\"event\":\"initialization_failed\",\"reason\":\"game_tls_bootstrap_timeout\",\"deferred_dispatches\":"<<capture::tls_guard::deferred.load()<<"}\n";output.flush();return 2;
    }
    output<<"{\"event\":\"initialization_complete\",\"tls_verified\":true,\"bootstrap_retries\":"<<capture::tls_guard::bootstrap_retries.load()<<"}\n";output.flush();
    GW::GameThread::RegisterGameThreadCallback(&hook,sample);
    output<<"{\"event\":\"callback_registered\"}\n";output.flush();
    GW::GameThread::EnableHooks();
    const auto event_target=capture::event_dispatch_target(GetModuleHandleW(L"gwca.dll"));
    GW::Hook::EnableHooks(GW::Hook::HookTarget(reinterpret_cast<void*>(event_target)));
    GW::GameThread::Enqueue([]{
        try {
            const auto chat=capture::chat_command_hook(GetModuleHandleW(L"gwca.dll"));
            using Dispatch=void(__cdecl*)(GW::HookStatus*,GW::UI::UIMessage,void*,void*);
            GW::UI::RegisterUIMessageCallback(&chat_hook,static_cast<GW::UI::UIMessage>(0x3000001b),reinterpret_cast<Dispatch>(chat.dispatcher));
            GW::Chat::CreateCommand(&travel_hook,L"tp",travel_command);
            GW::Hook::EnableHooks(GW::Hook::HookTarget(reinterpret_cast<void*>(chat.target)));
            std::lock_guard lock(mutex);pending.push_back("{\"event\":\"travel_command_enabled\",\"syntax\":\"/tp <map ID>\"}");
            capture_ready=true;
        }catch(...){++sample_errors;stop=true;std::lock_guard lock(mutex);pending.push_back("{\"event\":\"initialization_failed\",\"reason\":\"travel_command_hook_setup_failed\"}");}
    });
    output<<"{\"event\":\"game_thread_hook_enabled\"}\n";output.flush();
    while(!stop){
        Sleep(250);std::vector<std::string> batch;
        {std::lock_guard lock(mutex);batch.swap(pending);}
        for(auto& row:batch)output<<row<<'\n';output.flush();
    }
    GW::GameThread::Enqueue([]{GW::Chat::DeleteCommand(&travel_hook,L"tp");});
    // Keep the module loaded and the registered callback inert. Removing or
    // unloading while the game thread can enter the callback is unsafe.
    {
        std::lock_guard gate(sampling);
        std::vector<std::string> batch;
        {std::lock_guard lock(mutex);batch.swap(pending);}
        for(auto& row:batch)output<<row<<'\n';
    }
    output<<"{\"event\":\"collector_stop\",\"dropped_samples\":"<<dropped.load()
          <<",\"sample_errors\":"<<sample_errors.load()<<"}\n";output.flush();
    return output?0:3;
}
DWORD WINAPI run(void*) noexcept {
    DWORD result=0;
    try { result=run_impl(); } catch(...) {stop=true;result=ERROR_UNHANDLED_EXCEPTION;}
    stop=true;finished=true;return result;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI StopCapture(void*){if(started)stop=true;return 0;}
extern "C" __declspec(dllexport) DWORD WINAPI CaptureStatus(void*){
    return finished?3:stop?2:capture_ready?4:started?1:0;
}
// Explicit initialization avoids doing library setup inside the loader lock.
extern "C" __declspec(dllexport) DWORD WINAPI StartCapture(void*){
    if(started.exchange(true))return 0;
    HMODULE pinned=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&StartCapture),&pinned)){started=false;return GetLastError();}
    auto thread=CreateThread(nullptr,0,run,nullptr,0,nullptr);
    if(!thread){started=false;return GetLastError();}CloseHandle(thread);return 0;
}
BOOL WINAPI DllMain(HMODULE handle,DWORD reason,void*){
    if(reason==DLL_PROCESS_ATTACH){module=handle;DisableThreadLibraryCalls(handle);}return TRUE;
}

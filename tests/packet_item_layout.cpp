#include "../tools/packet_logger/item_layout.h"
#include "../tools/packet_logger/window_items_layout.h"
#include <iostream>
#include <stdexcept>
#include <vector>
int main()try{
 using namespace packet_capture;
 const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
 std::vector<std::uint32_t> schema{353,0x404,0x404,0x104,0x104,0x204,0x204,0x104,0x404,0x404,0x404,0x404,0x4017,0x400c,0x404};
 for(unsigned opcode:{353u,354u,352u}){
  if(opcode==352)schema={352,0x404,0x404,0x404,0x4017,0x400c,0x404};else schema[0]=opcode;
  const auto layout=item_layout(schema);check(bool(layout),"exact item schema recognized");
  for(unsigned count:{0u,2u,64u,65u}){
   std::vector<unsigned char> source(layout->modifiers+count*4,0xcc),output(layout->size);
   std::memcpy(source.data(),&opcode,4);std::memcpy(source.data()+layout->count,&count,4);
   const std::uint16_t name[]{0x108,0x107,0};std::memcpy(source.data()+layout->name,name,sizeof name);
   unsigned reads=0;const auto read=[&](void* to,const void* from,std::size_t bytes){++reads;
    const auto offset=static_cast<const unsigned char*>(from)-source.data();
    if(offset<0||std::size_t(offset)>source.size()||bytes>source.size()-std::size_t(offset))return false;
    std::memcpy(to,from,bytes);return true;};
   check(copy_item(*layout,source.data(),output,read)==(count<=64),"array count bounded before payload copy");
   check(reads==(count>0&&count<=64?2u:1u),"no unused modifier capacity read");
   if(count>64)continue;
   check(std::all_of(output.begin()+layout->name+4,output.begin()+layout->count,[](auto v){return v==0;}),"unused encoded-name capacity normalized");
   check(std::all_of(output.begin()+layout->modifiers+count*4,output.end(),[](auto v){return v==0;}),"unused modifier capacity normalized");
   check(std::equal(source.begin()+layout->modifiers,source.end(),output.begin()+layout->modifiers),"active modifier bytes preserved exactly");
   if(count){source.pop_back();check(!copy_item(*layout,source.data(),output,read),"truncated active modifier array rejected");}
   check(!copy_item(*layout,nullptr,output,read),"null item packet rejected");
   check(!copy_item(*layout,source.data(),std::span(output).first(layout->size-1),read),"undersized destination rejected");
   const auto wrong=opcode+1;std::memcpy(source.data(),&wrong,4);
   check(!copy_item(*layout,source.data(),output,read),"packet header must match verified item schema");
  }
  schema.back()=0x204;check(!item_layout(schema),"unverified array-element schema rejected");schema.back()=0x404;
  schema.push_back(0x404);check(!item_layout(schema),"extra unknown layout field rejected");schema.pop_back();
 }
 const std::array<std::uint32_t,4> unknown{25,0x204,0x104,0x400c};
 check(!item_layout(unknown),"other variable arrays remain unsupported");
 for(unsigned opcode:{131u,132u}){
  std::array<std::uint32_t,3> fields{opcode,0x102b,6};
  const auto layout=window_items_layout(fields);check(bool(layout),"verified window item-list schema recognized");
  for(unsigned count:{0u,3u,16u,17u}){
   std::vector<unsigned char> source(8+4*count,0xcc),destination(layout->size,0xee);
   std::memcpy(source.data(),&opcode,4);std::memcpy(source.data()+4,&count,4);
   unsigned reads=0;const auto read=[&](void* to,const void* from,std::size_t bytes){++reads;
    const auto offset=static_cast<const unsigned char*>(from)-source.data();
    if(offset<0||std::size_t(offset)>source.size()||bytes>source.size()-std::size_t(offset))return false;
    std::memcpy(to,from,bytes);return true;};
   check(copy_window_items(*layout,source.data(),destination,read)==(count<=16),"window list count bounded before IDs read");
   check(reads==(count&&count<=16?2u:1u),"window list never reads unused capacity");
   if(count>16)continue;
   check(std::equal(source.begin(),source.end(),destination.begin()),"active window IDs preserved exactly");
   check(std::all_of(destination.begin()+source.size(),destination.end(),[](auto v){return v==0;}),"stale window-list capacity zeroed");
   check(!copy_window_items(*layout,nullptr,destination,read),"null window list rejected");
   check(!copy_window_items(*layout,source.data(),std::span(destination).first(layout->size-1),read),"short window-list destination rejected");
   if(count){source.pop_back();check(!copy_window_items(*layout,source.data(),destination,read),"truncated active window list rejected");}
   source[0]=0;check(!copy_window_items(*layout,source.data(),destination,read),"wrong window-list header rejected");
  }
  fields[2]=0x404;check(!window_items_layout(fields),"different window array element encoding rejected");
  fields={opcode,0x202b,6};check(!window_items_layout(fields),"different window array capacity rejected");
 }
 std::cout<<"Exact item layouts, bounded active arrays and normalized unused capacity passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

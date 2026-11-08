#include "../tools/inventory/lifetime.h"
#include "../tools/inventory/modifier_observations.h"
#include <array>
#include <iostream>
#include <mutex>
#include <thread>
#include <future>
#include <atomic>
void check(bool v){if(!v)throw std::runtime_error("inventory decoder lifetime regression");}
int main()try{
 // Final drain must wait for an in-flight sample, while stop prevents queued work.
 std::mutex sample_mutex;std::atomic<bool> stop=false;std::promise<void> entered,release;auto allow=release.get_future();std::vector<int> records;
 std::thread sample([&]{std::lock_guard lock(sample_mutex);entered.set_value();allow.wait();records.push_back(1);});entered.get_future().wait();stop=true;release.set_value();
 {std::lock_guard lock(sample_mutex);check(records.size()==1);records.push_back(2);}sample.join();
 std::thread queued([&]{std::lock_guard lock(sample_mutex);if(!stop)records.push_back(3);});queued.join();check(records==std::vector<int>({1,2}));
 using namespace inventory_capture;Identity source{"12345678-1234-1234-1234-123456789012",1,7,100,std::string(168,'0'),"0100",{1,2}};
 for(unsigned field=0;field<4;++field){auto malformed=source;if(field==0)malformed.character[0]='x';if(field==1)malformed.character[8]='0';if(field==2)malformed.raw[0]='x';if(field==3)malformed.encoded[0]='x';check(!valid(malformed)&&!same(malformed,malformed));}
 check(valid(source)&&same(source,source));auto changed=source;changed.epoch++;check(!same(source,changed));changed=source;changed.item++;check(!same(source,changed));changed=source;changed.pointer++;check(!same(source,changed));changed=source;changed.raw[0]='1';check(!same(source,changed));changed=source;changed.modifiers.push_back(3);check(!same(source,changed));changed=source;changed.encoded="01000100";check(!same(source,changed));changed=source;changed.character[0]='9';check(!same(source,changed));changed=source;changed.modifiers.resize(65);check(!valid(changed));const std::array<std::uint32_t,3> raw{0x24580019,0x21e80903,0xc0000000};check(charges(raw)==25&&attribute_grade(raw)==3);const std::array<std::uint32_t,2> duplicates{0x24580019,0x24580005};check(!charges(duplicates));std::cout<<"PASS decoder ownership, epoch, exact identity and bounded definition checks\n";
}catch(const std::exception&e){std::cerr<<e.what();return 1;}

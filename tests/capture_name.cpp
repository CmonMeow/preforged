#include "../tools/capture/name_json.h"
#include <iostream>
int main(){
    using capture::name_json;
    if(name_json(L"A\"\\\n")!="\"\\u0041\\u0022\\u005c\\u000a\"")return 1;
    if(name_json(L"\x4e2d\x6587")!="\"\\u4e2d\\u6587\"")return 2;
    if(name_json(L"\xd83d\xde00")!="\"\\ud83d\\ude00\"")return 3;
    if(name_json(L"\xd800X\xdc00")!="\"\\ufffd\\u0058\\ufffd\"")return 4;
    if(capture::encoded_name_json(L"\x8101\x0100")!="[33025,256]")return 5;
    if(name_json(L"")!="\"\"")return 6;
    std::cout<<"UTF-16 names and opaque encoded units preserved\n";
}

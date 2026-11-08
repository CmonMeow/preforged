#include "../tools/skills/batch_template.h"
#include <iostream>
#include <functional>
void check(bool b){if(!b)throw std::runtime_error("test failed");}
void rejects(std::function<void()> f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected);}
int main(){try{using namespace skillbatch;
    for(unsigned p=1;p<=10;++p)for(unsigned s=0;s<=10;++s)if(p!=s)for(unsigned boundary:{0u,255u,256u,511u,512u,1023u,1024u,2047u,2048u,3432u,3494u,65535u}){
        Build b{p,s,{boundary,1,2,3,4,5,6,7}};if(boundary&&boundary<8)b.skills[1]=0;check(decode(encode(b))==b);
    }
    // Independently constructed fixed vector: type14/version0, R/none,
    // zero attributes, eight zero IDs at minimum width, optional zero tail.
    const auto fixed=encode(Build{2,0,{}});if(fixed!="OgAAAAAAAAAAAAAA")throw std::runtime_error("fixed vector actual="+fixed);
    // Independent published Wiki Skill_template.html example. Attributes are
    // SpearMastery37=12 and Leadership40=12; codec retains professions/skills.
    check(decode("OQCiUyo8AkVwR4KMMGAAAEAA")==Build{9,0,{1600,1546,1553,1557,1584,0,0,2}});
    rejects([]{decode("bad!");});rejects([]{decode("O");});rejects([]{encode(Build{8,8,{}});});
    rejects([]{encode(Build{8,1,{1,1}});});rejects([]{encode(Build{8,0,{65536}});});
    auto code=encode(Build{});code.back()='g';rejects([&]{decode(code);});
    std::cout<<"template profession/boundary/invalid-input tests passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

#include "gw_exploration.h"
#include <iostream>
#include <iterator>
using namespace fw::gw::exploration;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void write_bytes(const std::filesystem::path& path,const Packet& bytes){
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());out.close();
    check(bool(out),"test save write failed");
}
Packet read_bytes(const std::filesystem::path& path){
    std::ifstream in(path,std::ios::binary);check(bool(in),"test save read failed");
    return Packet(std::istreambuf_iterator<char>(in),{});
}
std::vector<std::uint32_t> decode(const std::vector<Packet>& packets,unsigned width,unsigned height){
    const auto& init=packets.front();unsigned bytes=0;for(unsigned i=0;i<4;++i)bytes|=unsigned(init[10+i])<<(8*i);
    Packet data;for(size_t i=1;i<packets.size();++i){check(packets[i][0]==137&&packets[i].size()<=260,"restore chunk opcode/capacity");data.insert(data.end(),packets[i].begin()+4,packets[i].end());}
    check(data.size()>=bytes&&data.size()-bytes<4,"restore fragment padding");data.resize(bytes);
    std::vector<std::uint32_t> words(width*height/32);size_t at=0;
    for(unsigned band=0;band<height/16;++band){check(at+2<=data.size(),"missing band header");const size_t end=at+2+data[at]+(unsigned(data[at+1])<<8);at+=2;
        check(end<=data.size(),"band length out of bounds");unsigned cell=band*width*16;bool bit=false,first_run=true;
        while(at<end){unsigned run=0;unsigned part;do{check(at<end,"unterminated run");part=data[at++];run+=part;}while(part==255);
            if(!first_run)++run;first_run=false;
            check(cell+run<=(band+1)*width*16,"run exceeds band");for(unsigned i=0;i<run;++i,++cell)if(bit)words[cell/32]|=1u<<(cell%32);bit=!bit;}
        check(cell==(band+1)*width*16,"band does not cover all cells");
    }check(at==data.size(),"trailing compressed bytes");return words;
}
int main()try{
    const auto cell=locate(55,{-100,-200,100,200},-100,200);
    check(cell&&cell->x==116&&cell->y==147,"map rectangle/terrain origin conversion");
    const auto shifted=locate(55,{-100,-200,100,200},2972,-2872);
    check(shifted&&shifted->x==117&&shifted->y==148,"3072 scale and inverted Y");
    check(!locate(65535,{-1,-1,1,1},0,0),"unknown map accepted");
    Progress progress({64,128});check(decode(progress.restore_packets(),64,128)==progress.words(),"empty progress decoder mismatch");
    Progress corner({64,128});corner.reveal({0,0});const auto restored=corner.restore_packets();
    const Packet first_band{8,0,0,1,61,1,255,255,255,192};
    check(std::equal(first_band.begin(),first_band.end(),restored[1].begin()+4),"retail first-run/subsequent-run length convention changed");
    check(progress.reveal({31,15}),"initial reveal missing");check(!progress.reveal({31,15}),"repeat reveal adds cells");
    progress.reveal({0,0});progress.reveal({63,127});
    check(decode(progress.restore_packets(),64,128)==progress.words(),"word/16-row/edge boundaries do not restore");
    Progress dense({256,512});for(unsigned y=0;y<512;y+=2)for(unsigned x=0;x<256;x+=2)dense.reveal({x,y});
    check(decode(dense.restore_packets(),256,512)==dense.words(),"255-byte run continuations do not restore");
    const auto directory=std::filesystem::temp_directory_path()/("gw-exploration-test-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
    const auto path=directory/"character-a.bin";Progress a({64,128}),b({64,128});a.reveal({2,2});b.reveal({40,80});a.save(path);b.save(path);
    Progress loaded({64,128});check(!loaded.load(path),"valid save reported recovery");
    check(!loaded.reveal({2,2})&&!loaded.reveal({40,80}),"save loses merged progress");
    const auto a_before=a.words();check(!a.save(path)&&a.words()==a_before,"disk merge silently changes the session mask");
    check(a.reveal({40,80}),"visit to another session's cells no longer emits a reveal");
    check(b.reveal({2,2}),"second session silently absorbed another client's cells");
    Progress isolated({64,128});check(!isolated.load(directory/"character-b.bin"),"missing save reported recovery");
    check(isolated.reveal({2,2}),"progress leaked between characters");

    const auto corrupt=directory/"corrupt.bin";
    std::vector<std::filesystem::path> preserved;
    const auto valid=read_bytes(path);
    Packet wrong_magic=valid;wrong_magic[0]^=1;
    Packet wrong_dimensions=valid;wrong_dimensions[4]=128;
    Packet trailing=valid;trailing.push_back(42);
    const std::vector<Packet> malformed{{1,2,3},wrong_magic,wrong_dimensions,
        Packet(valid.begin(),valid.end()-1),trailing};
    for(const auto& bytes:malformed){
        write_bytes(corrupt,bytes);Progress recovered({64,128});const auto result=recovered.load(corrupt);
        check(result&&!result->diagnostic.empty(),"malformed save has no recovery diagnostic");
        check(!std::filesystem::exists(corrupt),"malformed save still blocks future loads");
        check(std::find(preserved.begin(),preserved.end(),result->preserved_path)==preserved.end(),"recovery reuses a preserved filename");
        check(read_bytes(result->preserved_path)==bytes,"recovery does not preserve the original bytes");
        preserved.push_back(result->preserved_path);
        check(std::all_of(recovered.words().begin(),recovered.words().end(),[](auto word){return word==0;}),"malformed save installs partial progress");
        check(!recovered.load(corrupt),"repeated load treats preserved corruption as current save");
    }
    write_bytes(corrupt,wrong_magic);Progress saved_recovery({64,128});saved_recovery.reveal({10,10});
    const auto saved_result=saved_recovery.save(corrupt);
    check(saved_result&&read_bytes(saved_result->preserved_path)==wrong_magic,"save did not report and preserve malformed prior data");
    preserved.push_back(saved_result->preserved_path);
    Progress fresh({64,128});check(!fresh.load(corrupt)&&!fresh.reveal({10,10}),"save recovery did not write valid current progress");

    const auto locked=CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    check(locked!=INVALID_HANDLE_VALUE,"could not lock test save");bool read_failed=false;
    try{loaded.load(path);}catch(const std::runtime_error& error){read_failed=std::string(error.what()).find("open failed")!=std::string::npos;}
    CloseHandle(locked);
    check(read_failed,"existing unreadable save silently becomes empty progress");
    check(read_bytes(path)==valid,"I/O failure rewrote the existing save");
    bool directory_failed=false;try{fresh.load(directory);}catch(const std::runtime_error&){directory_failed=true;}
    check(directory_failed,"directory is treated as a missing or malformed save");
    std::filesystem::remove(path);std::filesystem::remove(corrupt);
    for(const auto& file:preserved)std::filesystem::remove(file);
    std::filesystem::remove(directory);
    const auto mark=Progress::mark({20,25});check(mark==Packet({139,0,20,0,0,0,25,0,0,0}),"retail reveal packet differs");
    std::cout<<"exploration conversion, restore codec, reveal and persistence passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

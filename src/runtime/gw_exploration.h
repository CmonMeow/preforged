#pragma once
#include <windows.h>
#include "gw_world_map_dimensions_generated.h"
#include "gw_exploration_rects_generated.h"
#include <vector>
#include <optional>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <cerrno>
namespace fw::gw::exploration {
using Packet=std::vector<std::uint8_t>;
inline void integer(Packet& out,std::uint32_t value,unsigned bytes){for(unsigned i=0;i<bytes;++i)out.push_back(static_cast<std::uint8_t>(value>>(i*8)));}
struct Cell {std::uint32_t x,y;};
struct Recovery {
    std::filesystem::path preserved_path;
    std::string diagnostic;
};
inline std::optional<Cell> locate(std::uint32_t map,const std::array<float,4>& bounds,float x,float y){
    if(map>=exploration_rects_2022.size()||!std::isfinite(x)||!std::isfinite(y)||
       !std::all_of(bounds.begin(),bounds.end(),[](float value){return std::isfinite(value);}))return {};
    const auto& rect=exploration_rects_2022[map];const auto dims=exploration_dimensions_2022(map);
    if(rect[2]<=rect[0]||rect[3]<=rect[1]||bounds[2]<=bounds[0]||bounds[3]<=bounds[1])return {};
    const double cx=(double(rect[0])+(double(x)-bounds[0])/96.0)/32.0;
    const double cy=(double(rect[1])+(double(bounds[3])-y)/96.0)/32.0;
    if(cx<0||cy<0||cx>=dims.width||cy>=dims.height)return {};
    return Cell{static_cast<std::uint32_t>(cx),static_cast<std::uint32_t>(cy)};
}
class Progress {
    ExplorationDimensions dims_{};
    std::vector<std::uint32_t> words_;
    static inline std::mutex storage_mutex_;
    static inline std::uint64_t recovery_sequence_=0;
    std::vector<std::uint32_t> read_file(const std::filesystem::path& path,std::optional<Recovery>& recovery)const{
        std::error_code error;
        const bool exists=std::filesystem::exists(path,error);
        if(error)throw std::runtime_error("exploration save status failed: "+error.message());
        if(!exists)return std::vector<std::uint32_t>(words_.size());
        if(!std::filesystem::is_regular_file(path,error)||error)
            throw std::runtime_error("exploration save is not an accessible regular file");
        errno=0;std::ifstream in(path,std::ios::binary);
        if(!in)throw std::runtime_error("exploration save open failed (errno="+std::to_string(errno)+")");
        auto malformed=[&](const char* diagnostic){
            if(in.bad())throw std::runtime_error("exploration save read failed");
            in.close();
            FILETIME time{};GetSystemTimeAsFileTime(&time);
            const auto stamp=(std::uint64_t(time.dwHighDateTime)<<32)|time.dwLowDateTime;
            for(unsigned attempt=0;attempt<64;++attempt){
                auto preserved=path;
                preserved+=L".corrupt-"+std::to_wstring(stamp)+L"-"+std::to_wstring(GetCurrentProcessId())+L"-"+
                    std::to_wstring(++recovery_sequence_);
                // Do not replace another recovery file; preserve the original
                // bytes before the caller can write a new, valid save.
                if(MoveFileExW(path.c_str(),preserved.c_str(),MOVEFILE_WRITE_THROUGH)){
                    recovery=Recovery{std::move(preserved),diagnostic};
                    return std::vector<std::uint32_t>(words_.size());
                }
                const auto move_error=GetLastError();
                if(move_error!=ERROR_FILE_EXISTS&&move_error!=ERROR_ALREADY_EXISTS)
                    throw std::runtime_error("exploration save recovery failed (Win32="+std::to_string(move_error)+
                        "): "+diagnostic);
            }
            throw std::runtime_error("exploration save recovery filename collision");
        };
        std::array<std::uint32_t,3> header{};
        if(!in.read(reinterpret_cast<char*>(header.data()),sizeof header))return malformed("truncated exploration save header");
        if(header[0]!=0x31505845)return malformed("invalid exploration save magic");
        if(header[1]!=dims_.width||header[2]!=dims_.height)return malformed("exploration save dimensions mismatch");
        std::vector<std::uint32_t> result(words_.size());
        if(!in.read(reinterpret_cast<char*>(result.data()),result.size()*4))return malformed("truncated exploration save data");
        const auto trailing=in.peek();
        if(in.bad()||(in.fail()&&!in.eof()))throw std::runtime_error("exploration save read failed");
        if(trailing!=EOF)return malformed("trailing exploration save data");
        return result;
    }
public:
    explicit Progress(ExplorationDimensions dims):dims_(dims),words_(std::uint64_t(dims.width)*dims.height/32){
        if(!dims.width||!dims.height||dims.width%32||dims.height%16||dims.width>2048||dims.height>2048)throw std::invalid_argument("invalid exploration dimensions");
    }
    const auto& words()const{return words_;}
    bool reveal(Cell cell){
        if(cell.x>=dims_.width||cell.y>=dims_.height)throw std::invalid_argument("exploration cell out of bounds");
        bool changed=false;
        for(std::uint32_t y=cell.y?cell.y-1:0;y<=std::min(cell.y+1,dims_.height-1);++y)
            for(std::uint32_t x=cell.x?cell.x-1:0;x<=std::min(cell.x+1,dims_.width-1);++x){
                const auto index=y*dims_.width+x;auto& word=words_[index/32];const auto mask=1u<<(index%32);
                changed|=!(word&mask);word|=mask;
            }
        return changed;
    }
    static Packet mark(Cell cell){Packet out;integer(out,139,2);integer(out,cell.x,4);integer(out,cell.y,4);return out;}
    std::vector<Packet> restore_packets()const{
        // Client decoder consumes length-prefixed 16-row bands of alternating
        // zero/one run lengths. 255 continues a run; even exact multiples end in 0.
        Packet compressed;
        for(std::uint32_t band=0;band<dims_.height/16;++band){
            Packet runs;bool value=false,first_run=true;unsigned count=0;
            // FUN_00799430 resets its run accumulator to 1 after each
            // transition: subsequent runs therefore encode length minus one.
            auto emit=[&]{unsigned encoded=count-(first_run?0u:1u);first_run=false;
                while(encoded>=255){runs.push_back(255);encoded-=255;}runs.push_back(static_cast<std::uint8_t>(encoded));};
            const auto first=band*dims_.width*16,last=first+dims_.width*16;
            for(auto index=first;index<last;++index){const bool bit=(words_[index/32]>>(index%32))&1;
                if(bit!=value){emit();count=0;value=bit;}++count;}
            emit();if(runs.size()>65535)throw std::runtime_error("exploration band too large");
            integer(compressed,static_cast<std::uint32_t>(runs.size()),2);compressed.insert(compressed.end(),runs.begin(),runs.end());
        }
        Packet init;integer(init,138,2);integer(init,dims_.width,4);integer(init,dims_.height,4);integer(init,static_cast<std::uint32_t>(compressed.size()),4);
        std::vector<Packet> packets{std::move(init)};
        for(size_t at=0;at<compressed.size();at+=256){
            const auto bytes=std::min(size_t(256),compressed.size()-at);const auto count=(bytes+3)/4;
            Packet packet;integer(packet,137,2);integer(packet,static_cast<std::uint32_t>(count),2);
            packet.insert(packet.end(),compressed.begin()+at,compressed.begin()+at+bytes);packet.resize(4+count*4);
            packets.push_back(std::move(packet));
        }
        return packets;
    }
    std::optional<Recovery> load(const std::filesystem::path& path){
        std::lock_guard lock(storage_mutex_);std::optional<Recovery> recovery;
        words_=read_file(path,recovery);return recovery;
    }
    std::optional<Recovery> save(const std::filesystem::path& path){
        std::lock_guard lock(storage_mutex_);std::optional<Recovery> recovery;
        auto merged=read_file(path,recovery);
        // Disk progress may include another session's reveals. Keep those out
        // of this session's mask so a later visit still sends its client a mark.
        for(size_t i=0;i<words_.size();++i)merged[i]|=words_[i];
        auto failure=[&](std::string diagnostic){
            // A failed replacement must still report any recovery performed
            // before the write, since the optional result cannot be returned.
            if(recovery)diagnostic+="; recovered "+recovery->diagnostic+"; preserved_file="+
                recovery->preserved_path.filename().string();
            throw std::runtime_error(diagnostic);
        };
        std::error_code directory_error;std::filesystem::create_directories(path.parent_path(),directory_error);
        if(directory_error)failure("exploration save directory creation failed: "+directory_error.message());
        auto temporary=path;temporary+=".tmp";
        std::ofstream out(temporary,std::ios::binary|std::ios::trunc);const std::uint32_t magic=0x31505845;
        out.write(reinterpret_cast<const char*>(&magic),4);out.write(reinterpret_cast<const char*>(&dims_.width),4);out.write(reinterpret_cast<const char*>(&dims_.height),4);
        out.write(reinterpret_cast<const char*>(merged.data()),merged.size()*4);out.close();
        if(!out)failure("exploration save write failed");
        // Windows filesystem rename cannot replace an existing destination.
        if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            failure("exploration save replace failed (Win32="+std::to_string(GetLastError())+")");
        return recovery;
    }
};
}

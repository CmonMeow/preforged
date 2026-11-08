#pragma once
#include <fstream>
#include <iostream>
#include <mutex>
#include <streambuf>
class RuntimeLog {
    struct Tee : std::streambuf {
        std::streambuf* console; std::ofstream& file; std::mutex& mutex;
        Tee(std::streambuf* c,std::ofstream& f,std::mutex& m):console(c),file(f),mutex(m){}
        std::streamsize xsputn(const char* s,std::streamsize n) override {
            // Persist before touching the console: a paused/disconnected console
            // must not hide the diagnostic that identifies the blocked phase.
            std::lock_guard lock(mutex);file.write(s,n);file.flush();console->sputn(s,n);return n;
        }
        int overflow(int ch) override { if(ch==traits_type::eof())return traits_type::not_eof(ch);char c=static_cast<char>(ch);return xsputn(&c,1)==1?ch:traits_type::eof(); }
        int sync() override {std::lock_guard lock(mutex);file.flush();return console->pubsync();}
    };
    std::ofstream file_;std::mutex mutex_;Tee out_,err_;
public:
    explicit RuntimeLog(const char* path):file_(path,std::ios::trunc),out_(std::cout.rdbuf(),file_,mutex_),err_(std::cerr.rdbuf(),file_,mutex_) {
        if(!file_)throw std::runtime_error("Cannot open runtime log");std::cout.rdbuf(&out_);std::cerr.rdbuf(&err_);
    }
    ~RuntimeLog(){std::cout.flush();std::cerr.flush();std::cout.rdbuf(out_.console);std::cerr.rdbuf(err_.console);}
};

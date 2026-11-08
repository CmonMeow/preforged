#pragma once
#include <cstdint>
#include <string>
#include <memory>
namespace fw::tcp {
class Socket {
    uintptr_t value_ = ~uintptr_t(0);
public:
    explicit Socket(uintptr_t value=~uintptr_t(0)):value_(value){}
    ~Socket();
    Socket(const Socket&)=delete;
    Socket& operator=(const Socket&)=delete;
    Socket(Socket&& other) noexcept:value_(other.release()){}
    uintptr_t release(){auto value=value_;value_=~uintptr_t(0);return value;}
    uintptr_t get() const{return value_;}
    explicit operator bool() const{return value_!=~uintptr_t(0);}
};
class Runtime {
public:
    Runtime(); ~Runtime();
    Runtime(const Runtime&)=delete;
    Runtime& operator=(const Runtime&)=delete;
};
class Listener {
    Socket socket_;
    uint16_t port_=0;
public:
    Listener(const std::string& address,uint16_t port);
    Socket accept(std::string& address,uint32_t wait_ms=100);
    uint16_t port() const{return port_;}
};
// DNS resolution precedes the socket deadline; call from a bounded worker.
Socket connect(const std::string& host,uint16_t port,uint32_t timeout_ms=3000);
}

#include "transport.h"
#include <windows.h>
#include <sodium.h>
namespace fw {
uint64_t now_ms() { return GetTickCount64(); }
uint64_t random_id() { uint64_t value=0; do { randombytes_buf(&value,sizeof(value)); } while(!value); return value; }
}

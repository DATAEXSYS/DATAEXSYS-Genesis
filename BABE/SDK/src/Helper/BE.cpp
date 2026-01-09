#include "Helper/BE.h"

bool isbigEndian(){
    uint16_t test = 0xABCD;
    uint8_t *ptr = reinterpret_cast<uint8_t*>(&test);
    return *ptr == 0xAB;
}

void BE16(uint16_t& val){
    if(!isbigEndian()){ // only convert if host is Little-Endian
        val = htons(val);
    }
}

void BE32(uint32_t &val){
    if(!isbigEndian()){
        val = htonl(val);
    }
}

// htonll is not standard; implement yourself
uint64_t htonll(uint64_t val) {
    if(isbigEndian()) return val; // BE system → nothing to do
    uint64_t hi = htonl(static_cast<uint32_t>(val >> 32));
    uint64_t lo = htonl(static_cast<uint32_t>(val & 0xFFFFFFFF));
    return (lo << 32) | hi;
}

void BE64(uint64_t &val){
    val = htonll(val);
}

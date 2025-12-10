#pragma once
#include <iostream>
#include <openssl/rand.h>
#include <cstdint>

using namespace std;

inline uint32_t idGen(){
    uint32_t id;
    if(RAND_bytes((unsigned char *)&id, sizeof(id)) != 1){
        cout << "Error generating ID" << endl;
        exit(1);
    }
    return id;
}

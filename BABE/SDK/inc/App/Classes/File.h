#pragma once 
#include <string>
#include <cstdint>
struct File{
    std::string filename;
    std::string extension;
    size_t size;
    uint8_t* data;
};

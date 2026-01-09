#pragma once
#include <arpa/inet.h>
#include "App/Classes/File.h"



inline auto toRaw(const File& file){
    //Format: [Length of Filename (32 bits to represent size)][Filename][Length of Extension (32 bits)][Extension][FileSize 64 bits][File]
    uint16_t fileNameSize = htons(file.filename.size());
    uint16_t extensionSize = htons(file.extension.size());
    uint64_t fileSize = file.size;
    int totalBytes = 16*2 + 64 + fileNameSize + extensionSize + file.size;
    uint8_t* data = new uint8_t[totalBytes];
    
}
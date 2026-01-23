#pragma once
#include "App/Classes/File.h"
#include "Helper/BE.h"
#include <cstdint>
#include <cstring>

struct RawFile{
    uint8_t* content;
    uint64_t size;
};

inline auto toRaw(const File& file){
    uint16_t filenameSize = file.filename.size();
    uint16_t extensionSize = file.extension.size();
    uint64_t fileSize = file.size;

    // Convert to big endian
    BE16(filenameSize);
    BE16(extensionSize);
    BE64(fileSize);

    uint64_t totalBytes = 2 + file.filename.size() + 2 + file.extension.size() + 8 + file.size;
    uint8_t* serialized = new uint8_t[totalBytes];
    uint8_t* ptr = serialized;

    memcpy(ptr, &filenameSize, 2);
    ptr += 2;

    memcpy(ptr, file.filename.c_str(), file.filename.size());
    ptr += file.filename.size();

    memcpy(ptr, &extensionSize, 2);
    ptr += 2;

    memcpy(ptr, file.extension.c_str(), file.extension.size());
    ptr += file.extension.size();

    memcpy(ptr, &fileSize, 8);
    ptr += 8;

    memcpy(ptr, file.data, file.size);

    RawFile* serial = new RawFile();
    serial->content = serialized;
    serial->size = totalBytes;


    return serial;
}

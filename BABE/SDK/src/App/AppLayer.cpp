#include "App/AppLayer.h"
#include "App/Adapters/FileToRaw.h"
MessageChunksList* sendData(File& file, uint8_t senderID, uint8_t receiverID){
    auto rawFile = toRaw(file);
}
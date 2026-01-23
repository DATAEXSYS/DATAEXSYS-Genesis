#pragma once
#include <cstdint>
#include <stddef.h>
#include "Classes/MessageChunksList.h"
#include "Classes/File.h"

MessageChunksList* sendData(File& file, uint8_t senderID, uint8_t receiverID);



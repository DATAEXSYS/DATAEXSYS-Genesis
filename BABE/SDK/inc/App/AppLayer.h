#pragma once
#include <cstdint>
#include <stddef.h>
#include "Classes/MessageChunksList.h"

MessageChunksList* sendData(void* data, uint64_t size, uint8_t senderID, uint8_t receiverID);



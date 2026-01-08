#pragma once
#include "Message.h"

struct MessageChunkNode{
Message* data;
MessageChunkNode* next;
};
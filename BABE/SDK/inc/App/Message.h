#pragma once

#include <cstdint>
#include <stddef.h>

struct Message{
uint16_t SeqNum;
size_t size;
void* data;
};
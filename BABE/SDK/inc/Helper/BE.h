#pragma once
#include <arpa/inet.h>
#include <cstdint>

bool isbigEndian();

void BE16(uint16_t& val);
void BE32(uint32_t& val);
void BE64(uint64_t& val);    
#pragma once
#include<cstdint>
enum class PacketType: uint8_t{
    RREQ = 0x00,
    RREP = 0x01,
    DATA = 0x02,
    RERR = 0x03,
    HELLO = 0x04,
    PKCertBlock = 0x05,
    RouteLogBlock = 0x06
};
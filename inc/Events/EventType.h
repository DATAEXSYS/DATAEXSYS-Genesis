#pragma once
enum class EventType {
    None,
    PACKET_INCOMING,
    PACKET_OUTGOING,
    ROUTE_CACHE_UPDATE
};
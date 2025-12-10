#pragma once
#include <functional>
#include "Events/EventType.h"

struct Event {
    EventType type;
    std::function<void()> callback;

    Event(){
        type = EventType::None;
        callback = nullptr;
    }
    Event(EventType t, std::function<void()> cb)
        : type(t), callback(std::move(cb)) {}
};
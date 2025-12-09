#include <functional>
#include "Events/EventType.h"

struct Event {
    EventType type;
    std::function<void()> callback;

    Event(EventType t, std::function<void()> cb)
        : type(t), callback(std::move(cb)) {}
};
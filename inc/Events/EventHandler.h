#include <queue>
#include "Events/Event.h"


class EventHandler {
public:
    void push(Event e) {
        events.push(std::move(e));
    }

    bool empty() const {
        return events.empty();
    }

    Event pop() {
        if (events.empty()) {
            return Event();
        }
        Event e = std::move(events.front());
        events.pop();
        return e;
    }

private:
    std::queue<Event> events;
};
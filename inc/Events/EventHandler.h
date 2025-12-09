#pragma once
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

    /**
     * @brief Safely pops an event from the front of the queue.
     * 
     * If the queue is not empty, this method retrieves the front event,
     * removes it from the queue, and returns true. If the queue is empty,
     * it does nothing and returns false.
     * 
     * @param out_event A reference to an Event object to store the result.
     * @return true if an event was successfully popped, false otherwise.
     */
    bool try_pop(Event& out_event) {
        if (events.empty()) {
            return false;
        }
        out_event = std::move(events.front());
        events.pop();
        return true;
    }

private:
    std::queue<Event> events;
};
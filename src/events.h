#ifndef HVH_WC_EVENTS_H
#define HVH_WC_EVENTS_H

#include <functional>
#include "tools/hashtable.hpp"

// Event class
// Easily create events which can be subscribed to and unsubscribed from.
// Template arguments are the types that will be passed to the execution function.
template <typename... Args>
class Event {
public:
	Event() = default;
	~Event() = default;

	// Subscribe to the event.
	// Returns an ID which can be used to unsubscribe from the event.
	int Subscribe(std::function<void(Args...)> func) {
		int my_id = next_id++;
		subscribers.insert(my_id, func);
		return my_id;
	}

	// Unsubscribe from the event.
	// Uses the ID returned by 'Subscribe'.
	void Unsubscribe(int id) {
		subscribers.erase(id);
	}

	// Execute the event.
	// Calls each subscribed function.
	void Execute(const Args&... args) {
		auto* funcs = subscribers.data<1>();
		for (size_t i = 0; i < subscribers.size(); ++i) {
			funcs[i](args...);
		}
	}

private:
	hvh::hashtable<int, std::function<void(Args...)>> subscribers;
	int next_id = 0;
};

// This 'class' acts as a namespace container for default global events.
class events {
public:
	// Executed during the logical update.
	// Runs before the normal logical update functions.
	static Event<>& earlyLogicalUpdate() { static Event<>* event = new Event<>(); return *event; }

	// Executed during the logical update.
	// Runs normal logical update functions.
	static Event<>& onLogicalUpdate() { static Event<>* event = new Event<>(); return *event; }

	// Executed during the logical update.
	// Runs after the normal logical update functions.
	static Event<>& lateLogicalUpdate() { static Event<>* event = new Event<>(); return *event; }

	// Executed during the display update, before the frame is drawn.
	// Runs before normal display update functions.
	// Args:	float interpolation; how far between logical updates this display update occurs.
	static Event<float>& earlyDisplayUpdate() { static Event <float> * event = new Event<float>(); return *event; }

	// Executed during the display update, before the frame is drawn.
	// Runs normal display update functions.
	// Args:	float interpolation; how far between logical updates this display update occurs.
	static Event<float>& onDisplayUpdate() { static Event <float>* event = new Event<float>(); return *event; }

	// Executed during the display update, before the frame is drawn.
	// Runs after normal display update functions.
	// Args:	float interpolation; how far between logical updates this display update occurs.
	static Event<float>& lateDisplayUpdate() { static Event <float>* event = new Event<float>(); return *event; }

	// Executed during the display update, after the frame is drawn.
	// Runs before normal post display update functions.
	// Args:	float interpolation; how far between logical updates this display update occurs.
	static Event<float>& earlyPostDisplayUpdate() { static Event <float>* event = new Event<float>(); return *event; }

	// Executed during the display update, after the frame is drawn.
	// Runs normal post display update functions.
	// Args:	float interpolation; how far between logical updates this display update occurs.
	static Event<float>& onPostDisplayUpdate() { static Event <float>* event = new Event<float>(); return *event; }

	// Executed during the display update, after the frame is drawn.
	// Runs after normal post display update functions.
	// Args:	float interpolation; how far between logical updates this display update occurs.
	static Event<float>& latePostDisplayUpdate() { static Event <float>* event = new Event<float>(); return *event; }
};

#endif // HVH_WC_EVENTS_H
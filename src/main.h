#ifndef HVH_WC_MAIN_H
#define HVH_WC_MAIN_H

namespace wc {

	// stop()
	// Stops the main loop and shuts down subsystems properly.
	void stop();

	// mainloop()
	// Runs a single iteration of the main loop.
	// This should not be called explicitly unless something is blocking the window message pump.
	// Lookin' at you, win32!
	// handleWindowMessages: whether or not this loop iteration should handle window messages.
	void mainloop(bool handleWindowMessages);

}; // namespace wc

#endif // HVH_WC_MAIN_H
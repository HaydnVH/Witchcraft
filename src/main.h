#ifndef HVH_WC_MAIN_H
#define HVH_WC_MAIN_H

namespace wc {

	// The logical framerate is 30fps.
	// Testing shows no noticable input lag at this frequency, so we allow ourselves the extra time for script/physics processing.
	constexpr const int LOGICAL_FRAMES_PER_SECOND = 30;
	constexpr const double LOGICAL_SECONDS_PER_FRAME = 1.0 / (double)LOGICAL_FRAMES_PER_SECOND;

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
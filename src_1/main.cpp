#include "main.h"

#include <iostream>
#include <chrono>
using namespace std;

#include "appconfig.h"
#include "userconfig.h"
#include "debug.h"
#include "filesys/vfs.h"
#include "lua/luasystem.h"
#include "window.h"
#include "graphics/renderer.h"
#include "events.h"
#include "ecs/entity.h"

#include "flecs.h"
#include "filesys/archive.h"
#include "tools/dxmathhelper.h"

struct Transform {
	float3 pos;
	quat rot;
	float3 scl;
	mat4 world_matrix;
};

struct InterpolatedTransform {
	float3 pos;
	quat rot;
	float3 scl;
};

void FlipTransforms(flecs::world& world) {
	world.each([](InterpolatedTransform& prev_transform, Transform& current_transform) {
		prev_transform.pos = current_transform.pos;
		prev_transform.rot = current_transform.rot;
		prev_transform.scl = current_transform.scl;
	});
}
/*
void CreateMatrices(flecs::world& world, float interpolation) {
	world.each([](InterpolatedTransform& prev_transform, Transform& current_transform) {
		current_transform.world_matrix =
			mat4_scale(lerp(prev_transform.scl, current_transform.scl, interpolation)) *
			mat4_rotation(quat_slerp(prev_transform.rot, current_transform.rot, interpolation)) *
			mat4_translation(lerp(prev_transform.pos, current_transform.pos, interpolation));
	});

	auto f = world.filter_builder<>()
		.term<Transform>()
		.term<InterpolatedTransform>().oper(flecs::Not)
		.build();

}
*/

namespace wc {

	namespace {

		flecs::world world;

		uint32_t logical_frame_counter = 0;
		uint32_t display_frame_counter = 0;
		double logical_time = 0.0;
		double display_time = 0.0;
		double logical_time_budget = 0.0;

		auto nowtime = std::chrono::high_resolution_clock::now();
		auto prevtime = nowtime;

		// Initialize subsystems.
		int startup() {
			appconfig::init();
			userconfig::init();
			debug::init(appconfig::getUserDir().c_str());
			lua::init();
			if (!vfs::init()) return 10;
			if (!window::init()) return 20;
			if (!gfx::init()) return 30;

			entity::initLua();
			
			Archive archive;
			archive.open((std::filesystem::current_path() / "src_1.wca").string().c_str());
			archive.pack("src", Archive::REPLACE_IF_NEWER, Archive::COMPRESS_FAST);
			archive.close();
			archive.open((std::filesystem::current_path() / "src_1.wca").string().c_str());
			archive.unpack("src_1");
			archive.close();




			return 0;
		}

		// Shut down subsystems.
		void shutdown() {
			gfx::shutdown();
			window::shutdown();
			vfs::shutdown();
			debug::showCrashReports(); debug::shutdown();
			userconfig::shutdown();
		}

		bool running = true;

	} // namespace <anon>

	void stop() {
		running = false;
	}

	void mainloop(bool handle_window_messages) {

		// Get the amount of time passed since the previous call to mainLoop().
		prevtime = nowtime;
		nowtime = std::chrono::high_resolution_clock::now();
		double delta_time = std::chrono::duration<double>(nowtime - prevtime).count();

		// Check here for a very large delta_time.
		// We don't want to simulate too many frames at once.
		// This value might need to change depending on behaviour and performance.
		// In a multiplayer environment, this would trigger a state resync.
		if (delta_time > 1.0) { delta_time = 1.0; }

		// Add delta_time to the time budget, and run the appropriate number of logical updates.
		logical_time_budget += delta_time;
		while (logical_time_budget >= LOGICAL_SECONDS_PER_FRAME) {

			// Handle window messages.
			if (handle_window_messages) {
				if (!window::handleMessages()) {
					running = false;
					return;
				}
			}

			// Handle terminal input.
			std::string terminal_input;
			while (debug::popInput(terminal_input)) {
				debug::user(terminal_input, "\n");
				lua::runString(terminal_input.c_str(), "CONSOLE", "@CLI");
			}

			// Run onLogicalUpdate events.
			events::earlyLogicalUpdate().Execute();
			events::onLogicalUpdate().Execute();
			events::lateLogicalUpdate().Execute();

			++logical_frame_counter;
			logical_time += LOGICAL_SECONDS_PER_FRAME;
			logical_time_budget -= LOGICAL_SECONDS_PER_FRAME;
		}

		// Run onDisplayUpdate events.
		// Draw the frame.
	}


}; // namespace wc


///////////////////////////////////////////////////////////////////////////////
// Main entry point into the program.
///////////////////////////////////////////////////////////////////////////////



int main(int argc, char* argv[]) {
	// Initialize services and subsystems.
	int result = wc::startup();

	// Enter the main loop.
	if (result == 0) {
		while (wc::running) {
			wc::mainloop(true);
	}	}

	// Shutdown services and subsystems.
	wc::shutdown();
	return result;
}


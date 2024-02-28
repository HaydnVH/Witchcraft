# Witchcraft

This is a game engine written in modern C++.  Vulkan is used for rendering, and Lua is used for scripting.  This is the latest version of the engine, which is not as feature-complete as older versions.  Any future updates will happen here.

## Building

Clone this repository, open it in Visual Studio as a cmake project, and build away!  The project is built and tested using Visual Studio 2022 on Windows, and GCC 13 and CMake 3.26 on Linux.  CMake 3.19 or newer is required to process CMakePresets.json, and GCC 13 or newer is required for certain features in C++20.

## Dependencies

The Vulkan SDK MUST be installed.  The newest version of the SDK as of the time of this writing is 1.3.268.0.  On Windows, simply download and run the installer provided by LunarG here: https://vulkan.lunarg.com/. On Linux, instructions for installing the SDK on Ubuntu using `apt` can be found here: https://vulkan.lunarg.com/doc/view/latest/linux/getting_started_ubuntu.html.  When selecting components, everything except for SDL and 32-bit support should be selected.

Git submodules are used to manage most third-party dependencies.  When you clone the repository, make sure to include the `--recursive` flag.  CMake handles the building of dependencies.

## License

Copyright ©2024 Haydn V. Harach.  More details coming soon!  Third-party dependencies used by Witchcraft are provided under a permissive license that allows redistribution, modification without redistribution, and commercial use (MIT, BSD, ZLIB etc.)

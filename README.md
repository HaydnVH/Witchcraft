# Witchcraft

This is a game engine written in modern C++.  Vulkan is used for rendering, and Lua is used for scripting.  This is the latest version of the engine, which is not as feature-complete as older versions.  Any future updates will happen here.

## Building

Download this repository, open it in Visual Studio as a cmake project, and build away!  The project is built and tested using Visual Studio 2022 on Windows, and GCC 12 and CMake 3.26 on Linux.  CMake 3.19 or newer is required to process CMakePresets.json, and GCC 11 or newer is required for certain features in C++20.

## Dependencies

The Witchcraft project depends on a number of other projects.  The 'dependencies' directory of this repository contains public headers and pre-built binary libraries.  If a dependency has a compatible license, its source repository will be included in the WithcraftDependencies repo and built from source to produce the binaries used here.  A library with an incompatible license whch provides pre-built binary libraries will be included in the 'dependencies' folder of this repo, but not in the WitchcraftDependencies repo.  Ideally, this repository should contain everything needed to build and run Witchcraft without needing to directly build any dependencies.

## License

Copyright ©2023 Haydn V. Harach.  More details coming soon!  Dependencies used by Witchcraft are provided under a permissive license that allows redistribution, modification without redistribution, and commercial use (MIT, BSD, ZLIB etc.); check the 'licenses' folder for more information.

# Summary
Limnova Engine is an ongoing hobby project/learning exercise with the intention of providing game-oriented* orbital mechanics in a game engine, including an editor with intuitive GUI-based controls, and teaching as much as possible about game engine architecture, orbital physics simulation, and modern C++ development along the way.

In its current state, the repository is very nearly a carbon copy of The Cherno's Hazel Engine (https://github.com/TheCherno/Hazel). However, key exceptions exist in areas relating to the engine's orbital focus - physics, scenes, and rendering - aside from which it uses CMake rather than Premake for a build system, and has been written almost entirely by hand so there is some (superficial) variation to be found in every source file. The interesting (maybe even exciting!) stuff can be found in any directory/file with "Orbital" in its name, particularly `Limnova/src/Orbital/OrbitalPhysics.h` which is supposed to eventually become a standalone header-only orbital physics engine, as well as the `LimnovaEditor` subproject which is presently the only usage example for the most-current orbital features.

*Utilising an abstracted physical model for staying within the performance standards and hardware requirements that can be reasonably expected of a modern game. Part and parcel of this abstracted model are restrictions on the types of scenarios which can be simulated.

# Building with Visual Studio
Open the root directory (contains this README and the top CMakeLists file) in Visual Studio.
Open a Terminal window in this directory and run:

`cmake -S . -B build/`.

This will create the output directory structure under a new subdirectory called "build", to contain all build objects (including executables). It is likely to be hidden in the Solution Explorer by the .gitignore - select "Show All Files" to view.

Select Build/Build All to compile the LimnovaEngine project and its executables - LimnovaEditor, PlayApp, and Orbital.

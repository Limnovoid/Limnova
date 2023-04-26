'*' = see definition

# Summary
Limnova Engine is an ongoing hobby project/learning exercise with the intention of providing game-oriented orbital mechanics* in a game engine, including an editor with intuitive GUI-based controls, and teaching as much as possible about game engine architecture, orbital physics simulation, and modern C++ development along the way.

In its current state, the source is largely carbon copied from The Cherno's Hazel Engine (https://github.com/TheCherno/Hazel). However, key exceptions exist in areas relating to the engine's orbital focus - physics, scenes, and rendering - aside from which it uses CMake rather than Premake for a build system, and has been written almost entirely by hand so there is some (superficial) variation to be found in every source file. The exciting stuff can be found in any directory/file with "Orbital" in its name, particularly `Limnova/src/Orbital/OrbitalPhysics.h` which is intended to eventually become a standalone header-only orbital physics library, as well as the `LimnovaEditor` subproject which is presently the only usage example for the most-current orbital features.

### Definitions
**Game-oriented orbital mechanics** = utilising an abstracted physical model for staying within the performance standards and hardware requirements that can be reasonably expected of a modern game. Baked into this abstracted model are restrictions on the types of scenarios which can be simulated. Most importantly:
1. All orbits must be 'primary-secondary' 2-body systems, in which one of the two masses has a significantly smaller mass such that its gravitational influence on the larger mass is negligible.
2. An object is only acted on by a single source of gravity at a time, and spheres of influence are used to determine which object is the primary source of gravity at any point in the simulated space.

These restrictions imply certain others, like objects with similar influencing* masses not being allowed to enter each other's spheres of influence. Systems like these can be used to describe many common scenarios such as star-planet systems, planet-moon systems, and any system comprising an influencing body orbited by a non-influencing body. Restrictions are dynamic, meaning they are determined on a per-system basis using the values associated with the objects, as opposed to, say, using predefined value ranges; in the editor, placing an orbital object in the scene or setting its physical attributes such that it violates any number of restrictions will mark that object as invalid, causing it to be excluded from the orbital simulation.

**Influencing mass** = sufficient to have a noticeable gravitational influence on other massive objects in a reasonable timeframe. This distinction is useful because it allows us to say that we are only interested in realtime interactions so our simulation can exclude the influences of relatively small objects like spacecraft (e.g, ships, stations, missiles) and debris - things that the developer of a space game might want to have interact with each other without violating the above-mentioned 'implied' restriction.

# Building with Visual Studio
Open the root directory (contains this README and the top CMakeLists file) in Visual Studio.
Open a Terminal window in this directory and run:

`cmake -S . -B build/`.

This will create the output directory structure under a new subdirectory called "build", to contain all build objects (including executables). It is likely to be hidden in the Solution Explorer by the .gitignore - select "Show All Files" to view.

Select Build/Build All to compile the LimnovaEngine project and its executables - LimnovaEditor, PlayApp, and Orbital.

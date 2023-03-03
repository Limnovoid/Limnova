# Building with Visual Studio
Open the root directory (contains this README and the top CMakeLists file) in Visual Studio.
Open a Terminal window in this directory and run:

`cmake -S . -B build/`.

This will create the output directory structure under a new subdirectory called "build", to contain all build objects (including executables). It is likely to be hidden in the Solution Explorer by the .gitignore - select "Show All Files" to view.

Select Build/Build All to compile the LimnovaEditor and PlayApp executables.

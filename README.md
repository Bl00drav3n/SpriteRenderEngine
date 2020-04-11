# SpriteRenderEngine
A 2D game engine with (almost) no dependencies.

## Architecture
The code is seperated in two layers: the platform layer and the game layer. Both layers can be compiled seperately, with the platform layer calling the game layer's GameUpdateAndRender function each frame.

The platform layer defines a set of platform specific API callbacks that can be used by the game layer for tasks like loading files from disk or allocate more memory. These callbacks are accessible from within the game layer as the global variable **Platform** (see game_platform.h).
**NOTE:** Memory provided by the platform layer is usually expected to be 0-initialized.

The game layer uses an abstract, command buffer based renderer, with the actual commands implemented using OpenGL. This makes it easier to implement a different backend like DirectX, if one wants to deal with that. Furthermore it provides the game logic, as well as a record based debug system with builtin profiler.

### Memory management
Memory is supplied by the platform layer and is seperated into persistent and transient memory. Memory that has to be valid over several frames (or indefinitly, like the GameState) is placed in persistent memory, while transient memory gets reset after every frame (i.e. by a call to GameUpdateAndRender).

Both chunks of memory are implemented as memory arenas, where blocks of memory get pushed on preallocated areas provided by the platform layer. This is a constant time operation if enough memory is left in the arena - the platform layer can decide to allocate another block of virtual memory in the case where more space is needed (which can cause some amount of memory fragmentation), or return 0 instead.
After an arena was reset, all memory belonging to this arena's memory space should be considered invalid.

In case that a scratch buffer is needed, game_memory.h provides a convenient implementation of temporary memory, implemented on top of the memory arena concept.

### Renderer
The renderer operates on a concept called **Render Groups**, which define a sequence of abstract render commands to be executed by the GPU. Another benefit is the weak coupling between game logic and rendering, so both operations can be neatly seperated in code.

### Audio
The platform layer will request the game layer to fill out a sample buffer of a specific size for each frame. The game layer tracks all running sound sources and mixes them before filling the audio buffer.

### Math
The game library defines a collection of useful math concepts like linear algebra functions, as well as a wrapper for C-standard library math functions.

### Debug system
To enable the debug system, GAME_DEBUG has to be defined in game_config.h.

The debug system keeps records of user defined profiling events, as well as several diagnostics like frame time, wav audio display, a console and memory usage information. 
The profiler stores events for each frame and keeps updating a circular buffer, so it's possible to go back to earlier frames for inspection.

To use the profiler, the code needs to be annotated with calls to TIMED_BLOCK to profile a scoped block, or TIMED_FUNCTION to time a whole function scope. Both are implemented as macros and are only compiled if GAME_DEBUG was defined in game_config.h.

### Windows platform layer
Features of the windows platform layer are:
- DLL hotloading: the application will check if the DLL has changed, and will reload it during execution (no memory fixup is happening).
- Memory blocks can be dynamically added or removed if required (growing arenas).
- Save and Restore: Ctrl+S saves the game state to a temporary file, Ctrl+L will restore this state.
- Interfacing with Windows CMD: WIP

### Assets
The engine loads assets from the data directory corresponding to the given FileType. Filenames should exclude the extension (for example "test" instead of "test.png").

## Compile and run
Because there are virtually no dependencies, everything should just compile and run out of the box without any effort. 

### Dependencies
- Platform: Win32
	- WinSDK (everything from WinXP onwards should work)
	- C-Runtime (math functions and some minor utility, as well as Visual Studio compatibility)
	- DirectSound
	- OpenGL 4

### Visual Studio
Almost any version of Visual Studio >= 2010 has been tested and should just work. The solution is provided inside the vc directory.
For VS versions implementing the universal platform, the C standard lib has been seperated out into multiple files, so the project files have to be adjusted.
Instead of only libcmt.lib the following libraries have to be included additionally:
- libucrt.lib
- vcruntime.lib

### Clang
Using gnu-make was the simplest way to get clang going besides using a simple batch file on Windows, so I suggest using that.
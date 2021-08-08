# cserver
Another Minecraft Classic server in C. Not well written, but it may be useful for someone. The server MAY have many security holes. Use carefully!
![atom_2021-08-08_08-14-18](https://user-images.githubusercontent.com/40758030/128621626-65725e58-561f-4cee-bbdb-fdf69c17b9a2.png)


## Features
* Classic Protocol Extension
* Multiplatform (Windows/Linux)
* Multithreaded clients processing
* Up to 256 worlds can be loaded
* Plugins support
* WebSocket client support ([More info](https://www.classicube.net/server/list/))
* RCON server (Implemented in base plugin)
* Own world generator (Written by [scaled](https://github.com/scaledteam) for [LuaClassic](https://github.com/igor725/LuaClassic), later ported to C by me)
* Heartbeat support
* Easy configurable

## Dependencies

### On Linux
1. zlib
2. pthread
3. libcrypto
4. curl

### On Windows
1. zlib
2. ws2_32
3. wininet
4. advapi32

## Building

### On Linux
``./build [args ...]``

NOTE: This script uses gcc, but you can change it to another by setting CC environment variable (``CC=clang ./build [args ...]``).

### On Windows
``.\build.bat [args ...]``

NOTE: This script must be runned in the Visual Studio Developer Command Prompt

### Build script arguments
* ``cls`` - Clear console window before compilation;
* ``pb`` - Build a plugin (next argument must be a plugin name, without the "cs-" prefix);
* ``pbu`` - Pull plugin repository and build it (This argument is used instead of ``pb``);
* ``dbg`` - Build with debug symbols;
* ``wall`` - Enable all possible warnings;
* ``wx`` - Treat warnings as errors;
* ``w0`` - Disable all warnings;
* ``od`` - Disable compiler optimizations;
* ``run`` - Start the server after compilation;
* ``runsame`` - Start the server after compilation in the same console window;
* ``install`` - Copy plugin binary to the ``plugins`` directory after compilation (Can be used only with ``pb``).


### Example
* ``./build`` - Build the server release binary
* ``./build dbg wall`` - Build the server with all warnings and debug symbols
* ``./build dbg wall pb base install`` - Build the base plugin with all warnings and debug symbols, then copy binary to the plugins directory

### Notes
* My main OS - Windows 10. It means the Linux part of the server not well tested.
* By default the server doesn't have any chat commands, build the cs-base plugin to add them.
* Your directory should have the following structure in order to compile plugins:
```
  [root_folder]/cserver - Main server repository
  [root_folder]/cs-base - Base server functionality
  [root_folder]/cs-survival - Survival plugin
  [root_folder]/cs-test - Test plugin
```

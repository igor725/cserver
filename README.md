# cserver
Another Minecraft Classic server written in C. Very buggy and dirty, but it may be useful for someone.

## Dependencies

### On linux
1. pthread
2. libcrypto
3. curl
4. zlib

### On windows
2. zlib
3. ws2_32
4. wininet
5. advapi32

## Building

### On linux
``./build [flags ...]``

NOTE: This script uses gcc, but you can change this to any gcc-like compiler by setting CC environment variable (``CC=clang ./build [flags ...]``).

### On Windows
``.\build.bat [flags ...]``

NOTE: This script uses Microsoft Visual Studio to compile the project

### Build script flags
* ``cls`` - Clear console window before compilation
* ``pb`` - Build plugin (next argument must be a plugin name, without the "cs-" prefix)'
* ``dbg`` - Build with debug symbols
* ``wall`` - Enable all possible warnings
* ``install`` - Copy plugin binary to the ``plugins`` directory after compilation (Can be used only with ``pb``)

### Notes
* By default the server doesn't have any chat commands, build the cs-base plugin to add them.
* Your directory should have the following structure in order to compile plugins
```
  [root_folder]/cserver - Main server repository
  [root_folder]/cs-base - Base server functionality
  [root_folder]/cs-survival - Survival plugin
  [root_folder]/cs-test - Test plugin
```

<p align='center'>
    <a href='https://github.com/igor725/cserver/actions/workflows/build.yml'>
        <img src='https://github.com/igor725/cserver/actions/workflows/build.yml/badge.svg' />
    </a>
    <a href='https://github.com/igor725/cserver/pulse'>
        <img src='https://img.shields.io/github/commit-activity/m/igor725/cserver' />
    </a>
</p>

# cserver
Another Minecraft Classic server in C.
The server is still under development (see [Projects](https://github.com/igor725/cserver/projects) tab)!

The goal of this project is to create a stable, customizable and future-rich multiplatform Minecraft Classic server with a minimum dependencies.

## Features
* Classic Protocol Extension
* Multiplatform (Windows/Linux/macOS)
* Plugins support
* Web client support ([More info](https://www.classicube.net/api/docs/server#footer))
* Lua scripting (Implemented in the [Lua plugin](https://github.com/igor725/cs-lua))
* RCON server (Implemented in the [Base plugin](https://github.com/igor725/cs-base))
* Own world generator (Written by [scaled](https://github.com/scaledteam) for [LuaClassic](https://github.com/igor725/LuaClassic), later ported to C by me)
* Heartbeat API (ClassiCube heartbeat implemented in the [Base plugin](https://github.com/igor725/cs-base))
* Easy configurable

## Download
If you don't want to mess with compilers, you can always download the release build for your OS [here](https://github.com/igor725/cserver/releases).
You can also get the latest unstable build [here](https://github.com/igor725/cserver/actions/workflows/build.yml).

## Dependencies

### On Linux/macOS
1. zlib
2. pthread
3. libcurl, libcrypto (will be loaded on demand)
4. libreadline (will be loaded if available)

### On Windows
1. zlib (will be automatically cloned and compiled during the building process)
2. Several std libs (such as Kernel32, DbgHelp, WS2_32)
3. WinInet, Advapi32 (will be loaded on demand)

### NOTES
* Some libraries (such as libcurl, libcrypto) are multiplatform. You can use them both on Windows, Linux and macOS.
* You can use zlib-ng in compatibility mode instead of zlib.

### Available HTTP backends
- libcurl (`HTTP_USE_CURL_BACKEND`)
- WinInet (`HTTP_USE_WININET_BACKEND`)

### Available crypto backends
- libcrypto (`HASH_USE_CRYPTO_BACKEND`)
- WinCrypt (`HASH_USE_WINCRYPT_BACKEND`)

Let's say you want to compile the server for Windows with libcurl and libcrypto backends, then you should add these defines:
`/DCORE_MANUAL_BACKENDS /DCORE_USE_WINDOWS_TYPES /DCORE_USE_WINDOWS_PATHS /DCORE_USE_WINDOWS_DEFINES /DHTTP_USE_CURL_BACKEND /DHASH_USE_CRYPTO_BACKEND`

It can be done by creating a file called `vars.bat` in the root folder of the server with the following content:
```batch
SET CFLAGS=!CFLAGS! /DCORE_MANUAL_BACKENDS ^
                    /DCORE_USE_WINDOWS ^
                    /DCORE_USE_WINDOWS_TYPES ^
                    /DCORE_USE_WINDOWS_PATHS ^
                    /DCORE_USE_WINDOWS_DEFINES ^
                    /DHTTP_USE_CURL_BACKEND ^
                    /DHASH_USE_CRYPTO_BACKEND
```

## Building

### On Linux/macOS
``./build [args ...]``

Single command builder for Linux: `curl -sL https://igvx.ru/singlecommand | bash` (server + base + lua)

NOTE: This script uses gcc, but you can change it to another compiler by setting CC environment variable (``CC=clang ./build [args ...]``).

### On Windows
``.\build.bat [args ...]``

NOTE: You must open a Visual Studio Command Prompt to run this script.

### Build script arguments

| Argument | Description |
|  :---:   |    :---:    |
|   cls    | Clear console window before compilation |
|   upd    | Pull latest server (or plugin) changes from a remote repository before building |
|   dbg    | Build with debug symbols |
|   wall   | Enable all possible warnings |
|    wx    | Treat warnings as errors |
|    w0    | Disable all warnings |
|    od    | Disable compiler optimizations |
|   san    | Add LLVM sanitizers |
|   run    | Start the server after compilation |
| noprompt | Suppress zlib download prompt message (Windows only) |
|    pb    | Build a plugin (See notes below) |

#### Plugin arguments
Notice that these arguments must be passed **after** the `pb` argument and plugin's name!

| Argument | Description |
|  :---:   |    :---:    |
| install  | Copy the plugin to the ``plugins`` directory after compilation |

#### Notes
* Each uncompiled plugin is a folder with a `cs-` prefix in its name. Inside this folder there should be a `src` folder with atleast one *.c file.
* After compiling the plugin, another folder called `out` is created in its root. Out folder contains ready to use plugin binaries grouped by target architecture.
* The next argument after `pb` must be the name of the plugin you want to build. The name **must not** include the `cs-` prefix.
* Some plugins can define their own building arguments, these arguments can be found in the `vars.sh`/`vars.bat` file in the plugin's root folder.

### Example
* ``./build`` - Build the server release binary
* ``./build dbg wall upd`` - Pull latest changes from this repository, then build the server with all warnings and debug symbols
* ``./build dbg wall pb base install`` - Build the base plugin with all warnings and debug symbols, then copy binary to the plugins directory
* ``./build dbg wall upd pb base install`` - Pull latest changes from cs-base repository, then build the base plugin and copy binary to the plugins directory

## Notes
* Use this software carefully! The server **may** have many security holes.
* At this point, it is strongly recommended to recompile **all plugins** every time you update the server, otherwise your server may crash due to API incompatibility.
* My main OS is Windows 10, this means the Linux thing are not well tested.
* By default the server doesn't have any useful chat commands, build the [cs-base](https://github.com/igor725/cs-base) plugin for an expanded command set.
* Here is the [example plugin](https://github.com/igor725/cs-test) for this server software.
* Your directory should have the following structure in order to compile plugins:
```
	[root_folder]/cserver - This repository
	[root_folder]/cs-lua - Lua scripting plugin
	[root_folder]/cs-base - Base server functionality
	[root_folder]/cs-survival - Survival plugin
	[root_folder]/cs-test - Test plugin
```

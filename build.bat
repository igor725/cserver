@echo off
cls
cl *.c /I.\zlib-1.2.11 ws2_32.lib .\zlib-1.2.11\contrib\vstudio\vc14\x86\ZlibDllRelease\zlibwapi.lib /Feserver
server

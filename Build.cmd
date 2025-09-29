@echo off
cd "%~dp0\src"

rd /s /q "bin"
md "bin"

gcc.exe -shared -static "Library.Proxy.c" "Library.Hooks.c" -lminhook -lkernel32 -lshlwapi -ldxgi -o "bin\MinecraftWindowsBeta.Bootstrapper.dll" 
gcc.exe -mwindows -s "Program.Bootstrapper.c" -lkernel32 -lshell32 -lole32 -lshlwapi -o "bin\MinecraftWindowsBeta.Bootstrapper.exe"
@echo off
cd "%~dp0\src"

rd /s /q "bin"
md "bin"

gcc.exe -nostdlib -Oz -s -Wl,--gc-sections,--exclude-all-symbols,--wrap=memcpy,--wrap=memset -shared -static "Library.Proxy.c" "Library.Hooks.c" -lminhook -lkernel32 -lshlwapi -ldxgi -luser32 -o "bin\MinecraftWindowsBeta.Bootstrapper.dll" 
gcc.exe -nostdlib -Oz -s -Wl,--gc-sections,--exclude-all-symbols -mwindows "Program.Bootstrapper.c" -lkernel32 -lshell32 -lole32 -lshlwapi -o "bin\MinecraftWindowsBeta.Bootstrapper.exe"
# Stonecutter GDK
A port of Stonecutter for GDK Builds of Minecraft: Bedrock Edition.

## Build
1. Install & update [MSYS2](https://www.msys2.org):

    ```bash
    pacman -Syu --noconfirm
    ```

3. Install [GCC](https://gcc.gnu.org) & [MinHook](https://github.com/TsudaKageyu/minhook):

    ```bash
    pacman -Syu mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-MinHook --noconfirm
    ```

3. Start MSYS2's `UCRT64` environment & run `Build.cmd`.
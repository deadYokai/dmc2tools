# Unpacker tool for DMC2

Only PC version of game (PS2 textures not supported)

- --
## Usage:
- #### Extract asset:
   ```shell
   repack[.exe] <filename>
   ```

- #### Pack asset:
    ```shell
    repack[.exe] <dirpath> -p
    ```

- --
## Supported assets:
Supported assets for now:

    tm2
    ptx
    bin
    ipu

Also compressed supported (like: `.biz`, `.ptz`)

- --
## Build:
Build is simple:
- Using GCC
    ```shell
    g++ -o repack repack.cpp
    ```
- Using Clang
    ```shell
    clang++ -o repack repack.cpp
    ```
- --


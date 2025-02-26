# Unpacker tool for DMC2

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
    ipu (extract only fn)

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


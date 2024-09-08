# file2png - transfer binary file data to PNG image
Sometimes a few platforms provide free cloud picture storage services, in order to utilize them to store normal files I created this utility.

## Build
CMake is supported. Need libpng, and zlib if using static libraries. On some platforms you also need to link libm manually. Since the UNIX-like command line interface, MinGW is required to build on Windows.
```shell
git clone https://github.com/tongpeifu-yh/file2png.git
cd file2png
# Now change the library path in CMakeLists.txt to build on your system
# After that,
cmake -B./build -S./ -DCMAKE_C_COMPILER=gcc -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build ./build
```

## Usage
Use `--help` for usage information.
```
$ file2png.exe --help
Usage: file2png.exe [options]
Options:
  -h, --help                    Print this help message
  -v, --version                 Print version information
  -i, --input <filename>        Input file to convert
  -o, --output <filename>       Output file
  -f, --forwards                Convert file to PNG
  -b, --backwards               Convert PNG to file
  -c, --compression <level>     Specify compression level
  -s, --stego[=<lsb1|lsb2>]     Using steganography
  -C, --cover <filename>        Specify cover image for steganography
```
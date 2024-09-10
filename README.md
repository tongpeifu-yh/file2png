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
Of course you can also build using one command, since there are only 3 source files:
```shell
gcc main.c process.c process_stego.c -o file2png -I<LIBPNG_INCLUDE_PATH> -L<LIBPNG_LIB_PATH> -l<lib>
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
  -f, --forwards                Convert file to PNG (default)
  -b, --backwards               Convert PNG to file
  -c, --compression <level>     Specify compression level
  -s, --stego[=<lsb1|lsb2>]     Using steganography (lsb2 by default)
  -C, --cover <filename>        Specify cover image for steganography
```
Examples:
```shell
# 1. convert file "data.dat" to a PNG
file2png -i data.dat -o data.png
# 2. recover file "data.dat" from "data.png"
file2png -i data.png -o data.dat -b
# 3. convert file "data.dat" to a PNG using lsb1 steganography
file2png -i data.dat -o data.png -C cover.png -slsb1
# 4. convert file "data.dat" to a PNG using lsb2 steganography
file2png -i data.dat -o data.png -C cover.png -s
# 5. recover file "data.dat" from "data.png" using lsb1 steganography
file2png -i data.png -o data.dat -b --stego=lsb1
```
# file2png - transfer binary file data to PNG image
Sometimes a few platforms provide free cloud picture storage services, in order to utilize them to store normal files I created this utility.

## Build
CMake is supported. Since the GNU-like command line interface, MinGW is required to build on Windows.
```shell
git clone https://github.com/tongpeifu-yh/file2png.git
cd file2png
cmake -B ./build -G Ninja -DCMAKE_C_COMPILER=gcc -S ./
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
  -f, --forwords                Convert file to PNG
  -b, --backwords               Convert PNG to file

```
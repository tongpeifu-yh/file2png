cmake_minimum_required(VERSION 3.13)

# file2png - using libpng to transform files to PNG images, or to convert images back to files.
# How to build:
# 1. Configure the libpng library by editing the following lines:
set(LIBPNG_INCLUDE_DIR "C:\\msys64\\usr\\local\\include")
set(LIBPNG_LIBS libpng.a libzlibstatic.a )
set(LIBPNG_LIB_DIR "C:\\msys64\\usr\\local\\lib")
# 2. Set installation directory using `-DCMAKE_INSTALL_PREFIX=` in the command line.

project(file2png ASM C)
add_executable(file2png main.c process.c process_stego.c)

target_include_directories(file2png PUBLIC ${LIBPNG_INCLUDE_DIR})
target_link_directories(file2png PUBLIC ${LIBPNG_LIB_DIR})
target_link_libraries(file2png PUBLIC ${LIBPNG_LIBS})
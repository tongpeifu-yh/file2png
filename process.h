#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <getopt.h>
#include <stdint.h>
#include <png.h>

//signs
#define FILE2PNG_FORWORDS 1 // file to png
#define FILE2PNG_BACKWORDS 2 // png back to file

typedef struct _file2png_ctx{
    uint8_t sign;
    const char *in_filename;
    const char *out_filename;

}file2png_ctx;

// typedef struct _file_info{
//     char name[MAX_PATH_LENGTH];
// }file_info;

typedef struct _line_buf{
    int cursor;
    int size;
    int rw_times;
    png_bytep buffer;
}line_buf;

void process_args(file2png_ctx *ctx, int argc, char * const *argv);
int process_image(file2png_ctx *ctx);
void print_usage(const char *program);

int file2png(const char *filename, const char * pngname);
int png2file(const char *pngname, const char * filename);

void png_write_byte(png_structp png_ptr, line_buf *buf, png_byte value);
void png_write_bytes(png_structp png_ptr, line_buf *buf, png_bytep src, size_t size);
void png_write_byte_flush(png_structp png_ptr, line_buf *buf);

void png_read_byte(png_structp png_ptr, line_buf *buf, png_bytep value);
void png_read_bytes(png_structp png_ptr, line_buf *buf, png_bytep src, size_t size);
void png_read_byte_flush(png_structp png_ptr, line_buf *buf);
#endif
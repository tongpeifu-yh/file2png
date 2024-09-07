#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <getopt.h>
#include <stdint.h>
#include <png.h>

//signs
#define FILE2PNG_FORWARDS 1 // file to png
#define FILE2PNG_BACKWARDS 2 // png back to file

typedef enum _stego_t{
    STEGO_NONE = 0,
    STEGO_LSB1,
    STEGO_LSB2,
    STEGO_DEFAULT = STEGO_LSB2,
} stego_t;

typedef struct _file2png_ctx{
    uint8_t sign;
    uint8_t compression_level;
    const char *in_filename;
    const char *out_filename;
    stego_t stego;
    const char * cover_name;
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

char *strlwr_custom(char *str);

int file2png(const char *filename, const char * pngname, uint8_t compression_level);
int png2file(const char *pngname, const char * filename);

void png_write_byte(png_structp png_ptr, line_buf *buf, png_byte value);
void png_write_bytes(png_structp png_ptr, line_buf *buf, png_bytep src, size_t size);
void png_write_byte_flush(png_structp png_ptr, line_buf *buf);

void png_read_byte(png_structp png_ptr, line_buf *buf, png_bytep value);
void png_read_bytes(png_structp png_ptr, line_buf *buf, png_bytep src, size_t size);
void png_read_byte_flush(png_structp png_ptr, line_buf *buf);

int stego_hide(const char * file_name, const char * cover_name, const char * output_name, stego_t mode);
int stego_recover(const char * stego_name, const char * file_name, stego_t mode);

void stego_write_bytes_lsb2(png_structp cover, line_buf *cbuf, int bytes_per_pixel_cover, png_structp output, line_buf *obuf, const uint8_t* bytes, size_t size);
void stego_write_finish(png_structp cover, line_buf *cbuf, int bytes_per_pixel_cover, png_structp output, line_buf *obuf, uint32_t height);

void stego_read_bytes_lsb2(png_structp png, line_buf *buf, uint8_t *bytes, size_t size);

void stego_write_bytes_lsb1(png_structp cover, line_buf *cbuf, int bytes_per_pixel_cover, png_structp output, line_buf *obuf, const uint8_t* bytes, size_t size);
void stego_read_bytes_lsb1(png_structp png, line_buf *buf, uint8_t *bytes, size_t size);

int get_pixels_per_byte(stego_t stego);// how many pixels needed to store one byte, according to stego mode

typedef void stego_write_bytes_f(png_structp cover, line_buf *cbuf, int bytes_per_pixel_cover, png_structp output, line_buf *obuf, const uint8_t* bytes, size_t size);
typedef void stego_read_bytes_f(png_structp png, line_buf *buf, uint8_t *bytes, size_t size);

#define get_new_file_name(file_name, existing_name, new_file_name, suffix) \
    if(!file_name){\
        size_t len = strlen(existing_name) + strlen(suffix) + 1;\
        char *name_tmp = malloc(len);\
        new_file_name = name_tmp;\
        if(!name_tmp){\
            fprintf(stderr, "Error: Out of memory.\n");\
            goto ERROR;\
        }\
        if(snprintf(name_tmp, len, "%s%s", existing_name, suffix)>=len){\
            fprintf(stderr, "Error: Failed to format output filename.\n");\
            goto ERROR;\
        }\
    }\
    else\
        new_file_name = file_name;


#endif
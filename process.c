#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "process.h"
#include "public.h"


void process_args(file2png_ctx *ctx, int argc, char * const *argv)
{
    static const struct option long_options[] ={
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"input", required_argument, NULL, 'i'},
        {"output", required_argument, NULL, 'o'},
        {"forwards", no_argument, NULL, 'f'},
        {"backwards", no_argument, NULL, 'b'},
        {0, 0, 0, 0}
    };
    int opt,opt_index=0;
    while((opt = getopt_long(argc, argv,"hvi:o:fb", long_options, &opt_index)) != -1)
    {
        switch(opt)
        {
            case 'h':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'v':
                printf("file2png version %s.\n", FILE2PNG_VERSION);
                exit(EXIT_SUCCESS);
            case 'i':
                ctx->in_filename = optarg;
                break;
            case 'o':
                ctx->out_filename = optarg;
                break;
            case 'f':
                ctx->sign = FILE2PNG_FORWARDS;
                break;
            case 'b':
                ctx->sign = FILE2PNG_BACKWARDS;
                break;
            default:
                fprintf(stderr, "Invalid option: '-%c' or --%s\n", opt, argv[optind-1]);
                goto ERROR;
        }
    }
    if(!ctx->in_filename){
        fprintf(stderr, "Error: Input file not specified.\n");
        goto ERROR;
    }
    if(!ctx->out_filename){
        fprintf(stderr, "Warning: Output file not specified. <InputFileName>.png will be used for forwards and <InputFileName>.file will be used for backwards.\n");
        //goto ERROR;
    }
    return;
ERROR:
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
}


void print_usage(const char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("Options:\n");
    printf("  -h, --help                    Print this help message\n");
    printf("  -v, --version                 Print version information\n");
    printf("  -i, --input <filename>        Input file to convert\n");
    printf("  -o, --output <filename>       Output file\n");
    printf("  -f, --forwards                Convert file to PNG\n");
    printf("  -b, --backwards               Convert PNG to file\n");
}


int process_image(file2png_ctx *ctx)
{
    if(!ctx) 
        return EXIT_FAILURE;
    if(ctx->sign == FILE2PNG_FORWARDS){
        return file2png(ctx->in_filename, ctx->out_filename);
    }
    else if (ctx->sign == FILE2PNG_BACKWARDS){
        return png2file(ctx->in_filename, ctx->out_filename);
    }
    else{
        fprintf(stderr, "Error: Invalid conversion sign.\n");
        return EXIT_FAILURE;
    }
    
}

int file2png(const char *filename, const char *pngname)
{
    const char *pngname_new = NULL;
    FILE *ifp = NULL;
    FILE *ofp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytep *row_pointers = NULL;
    uint32_t width, height;
    line_buf buf={0,0,0, NULL};

    if(!pngname){
        size_t len = strlen(filename) + strlen(".png") + 1;
        char *pngname_tmp = malloc(len);
        pngname_new = pngname_tmp;
        if(!pngname_tmp){
            fprintf(stderr, "Error: Out of memory.\n");
            goto ERROR;
        }
        if(snprintf(pngname_tmp, len, "%s.png", filename)>=len){
            fprintf(stderr, "Error: Failed to format output filename.\n");
            goto ERROR;
        }
    }
    else
        pngname_new = pngname;

    ifp = fopen(filename, "rb");
    if(!ifp){
        fprintf(stderr, "Error: Failed to open input file '%s'.\n", filename);
        goto ERROR;
    }

    ofp = fopen(pngname_new, "wb");
    if(!ofp){
        fprintf(stderr, "Error: Failed to open output file '%s'.\n", pngname_new);
        goto ERROR;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr){
        fprintf(stderr, "Error: Failed to create png struct.\n");
        goto ERROR;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr){
        fprintf(stderr, "Error: Failed to create info struct.\n");
        goto ERROR;
    }

    if(setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error: during libpng processing.\n");
        goto ERROR;
    }

    png_init_io(png_ptr, ofp);

    //compute length of ifp
    fseek(ifp, 0, SEEK_END);
    uint64_t file_size = ftell(ifp);
    rewind(ifp);

    //compute header:
    //file name length(sizeof(uint16_t)), file name, file length(sizeof(uint64_t))
    uint16_t filename_length = strlen(filename);

    uint64_t total_length = sizeof(filename_length) + filename_length + sizeof(file_size) + file_size;
    uint8_t bytes_per_pixel = 3;//3 bytes per pixel, using RGB
    uint64_t final_length = (total_length % bytes_per_pixel == 0) 
        ? total_length 
        : total_length + bytes_per_pixel - total_length % bytes_per_pixel;// with padding
    uint64_t pixel_num = final_length / bytes_per_pixel;
    width = height = ceil(sqrt(pixel_num));
    if(width == 0){
        width = height = 1;
    } 
    while(width * height * bytes_per_pixel < final_length)
        width++;
    pixel_num = width * height;

    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, 
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    
    png_write_info(png_ptr, info_ptr);

    buf.buffer = (png_bytep) png_malloc(png_ptr, width * bytes_per_pixel * sizeof(png_byte));
    if(!buf.buffer){
        fprintf(stderr, "Error: Failed to allocate memory for image.\n");
        goto ERROR;
    }

    buf.size = width * bytes_per_pixel;

    png_write_bytes(png_ptr, &buf, (png_bytep)&filename_length, sizeof(filename_length));
    png_write_bytes(png_ptr, &buf, (png_bytep)filename, strlen(filename));
    png_write_bytes(png_ptr, &buf, (png_bytep)&file_size, sizeof(file_size));
    //png_write_bytes(png_ptr, &buf, &filename_length, sizeof(filename_length));
    png_byte tmp[4096];
    size_t read_len;
    while((read_len = fread(tmp, sizeof(png_byte), sizeof(tmp), ifp)) > 0){
        png_write_bytes(png_ptr, &buf, tmp, read_len);
    }
    png_write_byte_flush(png_ptr, &buf);
    memset(buf.buffer, 0, buf.size * sizeof(png_byte));
    while(buf.rw_times < height){
        png_write_row(png_ptr, buf.buffer);
        buf.rw_times ++;
    }
    png_write_end(png_ptr, info_ptr);

    // Cleanup
    png_free(png_ptr, buf.buffer);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    if(pngname_new && !pngname)
        free((char *)pngname_new);
    fclose(ifp);
    fclose(ofp);
    return EXIT_SUCCESS;

ERROR:
    if(pngname_new && !pngname)
        free((char *)pngname_new);
    if(ifp)
        fclose(ifp);
    if(ofp)
        fclose(ofp);
    if(buf.buffer && png_ptr)
        png_free(png_ptr, buf.buffer);
    if(png_ptr && info_ptr)
        png_destroy_write_struct(&png_ptr, &info_ptr);
    else if(png_ptr)
        png_destroy_write_struct(&png_ptr, NULL);
    
    return EXIT_FAILURE;
}


void png_write_byte(png_structp png_ptr,line_buf *buf, png_byte value)
{
    buf->buffer[buf->cursor++] = value;
    if(buf->cursor >= buf->size){
        png_write_row(png_ptr, buf->buffer);
        buf->rw_times ++;
        buf->cursor = 0;
    }
}

void png_write_bytes(png_structp png_ptr, line_buf *buf, png_bytep src, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        png_write_byte(png_ptr, buf, src[i]);
    }

}

void png_write_byte_flush(png_structp png_ptr, line_buf *buf)
{
    if(buf->cursor > 0){
        png_write_row(png_ptr, buf->buffer);
        buf->rw_times ++;
        buf->cursor = 0;
    }
}


int png2file(const char *pngname, const char *filename)
{
    const char *filename_new = NULL;
    FILE *ifp = NULL;
    FILE *ofp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    uint32_t width, height;
    line_buf buf={0,0,0,NULL};

    if(!filename){
        size_t len = strlen(pngname) + strlen(".file") + 1;
        char *filename_tmp = malloc(len);
        filename_new = filename_tmp;
        if(!filename_tmp){
            fprintf(stderr, "Error: Out of memory.\n");
            goto ERROR;
        }
        if(snprintf(filename_tmp, len, "%s.file", pngname)>=len){
            fprintf(stderr, "Error: Failed to format output filename.\n");
            goto ERROR;
        }
    }
    else
        filename_new = filename;

    ifp = fopen(pngname, "rb");
    if(!ifp){
        fprintf(stderr, "Error: Failed to open input file '%s'.\n", pngname);
        goto ERROR;
    }

    ofp = fopen(filename_new, "wb");
    if(!ofp){
        fprintf(stderr, "Error: Failed to open output file '%s'.\n", filename_new);
        goto ERROR;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr){
        fprintf(stderr, "Error: Failed to create png struct.\n");
        goto ERROR;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr){
        fprintf(stderr, "Error: Failed to create info struct.\n");
        goto ERROR;
    }

    if(setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error: during libpng processing.\n");
        goto ERROR;
    }

    png_init_io(png_ptr, ifp);

    png_read_info(png_ptr, info_ptr);
    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);

    if(png_get_bit_depth(png_ptr, info_ptr) != 8){
        fprintf(stderr, "Error: Input image must have 8-bit depth.\n");
        goto ERROR;
    }
    if(png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGB){
        fprintf(stderr, "Error: Input image must have RGB color type.\n");
        goto ERROR;
    }

    uint8_t bytes_per_pixel = 3;//3 bytes per pixel, using RGB
    buf.size = width * bytes_per_pixel;
    buf.buffer = (png_bytep)png_malloc(png_ptr, buf.size * sizeof(png_byte));
    if(!buf.buffer){
        fprintf(stderr, "Error: Failed to allocate memory for image.\n");
        goto ERROR;
    }

    uint64_t file_size;
    uint16_t filename_length;
    uint64_t pixel_num = width * height;
    png_read_bytes(png_ptr, &buf, (png_bytep)&filename_length, sizeof(filename_length));
    if(filename_length > pixel_num * bytes_per_pixel){
        fprintf(stderr, "Error: File name length is too long.\n");
        goto ERROR;
    }
    png_bytep initial_filename = (png_bytep)png_malloc(png_ptr, filename_length + 1);
    if(!initial_filename){
        fprintf(stderr, "Error: Failed to allocate memory for file name.\n");
        goto ERROR;
    }
    png_read_bytes(png_ptr, &buf, initial_filename, filename_length);
    initial_filename[filename_length] = '\0';
    fprintf(stdout, "Initial file name: %s\n", initial_filename);
    png_read_bytes(png_ptr, &buf, (png_bytep)&file_size, sizeof(file_size));
    if(file_size > pixel_num * bytes_per_pixel){
        fprintf(stderr, "Error: File size is too long.\n");
        goto ERROR;
    }
    png_byte tmp[4096];
    size_t write_len = 0;
    
    // while((write_len = fread(tmp, sizeof(png_byte), sizeof(tmp), ifp)) > 0){
    //     png_write_bytes(png_ptr, &buf, tmp, read_len);
    // }
    while(write_len + sizeof(tmp) < file_size){
        png_read_bytes(png_ptr, &buf, tmp, sizeof(tmp));
        fwrite(tmp, sizeof(png_byte), sizeof(tmp), ofp);
        write_len += sizeof(tmp);
    }
    png_read_bytes(png_ptr, &buf, tmp, file_size - write_len);
    fwrite(tmp, sizeof(png_byte), file_size - write_len, ofp);

    if(filename_new && !filename)
        free((char *)filename_new);
    png_free(png_ptr, buf.buffer);
    png_free(png_ptr, initial_filename);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(ifp);
    fclose(ofp);
    return EXIT_SUCCESS;

ERROR:
    if(filename_new && !filename)
        free((char *)filename_new);
    if(ifp)
        fclose(ifp);
    if(ofp)
        fclose(ofp);
    if(buf.buffer && png_ptr)
        png_free(png_ptr, buf.buffer);
    if(initial_filename && png_ptr){
        png_free(png_ptr, initial_filename);
    }
    if(png_ptr && info_ptr){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    }
    else if (png_ptr){
        png_destroy_read_struct(&png_ptr, NULL, NULL);
    }
    
    return EXIT_FAILURE;
}

void png_read_byte(png_structp png_ptr, line_buf *buf, png_bytep value)
{
    if(buf->rw_times == 0){
        png_read_row(png_ptr, buf->buffer, NULL);// read a row first
        buf->rw_times++;
    }
    *value = buf->buffer[buf->cursor++];
    if(buf->cursor >= buf->size)
    {
        png_read_row(png_ptr, buf->buffer, NULL);
        buf->rw_times++;
        buf->cursor = 0;
    }
}

void png_read_bytes(png_structp png_ptr, line_buf *buf, png_bytep src, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        png_read_byte(png_ptr, buf, &src[i]);
    }
}

void png_read_byte_flush(png_structp png_ptr, line_buf *buf)
{
    if(buf->cursor > 0){
        png_read_row(png_ptr, buf->buffer, NULL);
        buf->rw_times ++;
        buf->cursor = 0;
    }
}

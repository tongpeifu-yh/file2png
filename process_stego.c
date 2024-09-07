#include "process.h"
#include "public.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int stego_hide(const char *input_name, const char *cover_name, const char *output_name, stego_t stego)
{
    FILE *ifp = NULL, *ofp = NULL, *cfp = NULL;
    uint64_t file_size = 0;
    uint16_t input_name_length = strlen(input_name);
    png_structp cover = NULL, output = NULL;
    png_infop cover_info = NULL, output_info = NULL;
    uint32_t width = 0, height = 0;
    const char *output_name_new = NULL;
    line_buf cbuf = {0, 0, 0, NULL};
    line_buf obuf = {0, 0, 0, NULL};
    stego_write_bytes_f *stego_write_bytes = NULL;

    if (stego == STEGO_LSB1) {
        stego_write_bytes = &stego_write_bytes_lsb1;
    }
    else if (stego == STEGO_LSB2) {
        stego_write_bytes = &stego_write_bytes_lsb2;
    }
    else {
        fprintf(stderr, "Error: Unsupported stego mode.\n");
        return EXIT_FAILURE;
    }

    get_new_file_name(output_name, input_name, output_name_new, ".png");

    ifp = fopen(input_name, "rb");
    if (!ifp) {
        fprintf(stderr, "Error: Failed to open input file '%s'.\n", input_name);
        goto ERROR;
    }

    cfp = fopen(cover_name, "rb");
    if (!ifp) {
        fprintf(stderr, "Error: Failed to open cover file '%s'.\n", cover_name);
        goto ERROR;
    }

    ofp = fopen(output_name_new, "wb");
    if (!ofp) {
        fprintf(stderr, "Error: Failed to open output file '%s'.\n", output_name_new);
        goto ERROR;
    }

    cover = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    cover_info = png_create_info_struct(cover);

    output = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    output_info = png_create_info_struct(output);
    if (!cover || !cover_info || !output || !output_info) {
        fprintf(stderr, "Error: Failed to create PNG structures.\n");
        goto ERROR;
    }

    if (setjmp(png_jmpbuf(cover)) || setjmp(png_jmpbuf(output))) {
        fprintf(stderr, "Error: during libpng processing.\n");
        fprintf(stderr, "Info: cbuf.rw_times = %d, cbuf.cursor = %d\n", cbuf.rw_times, cbuf.cursor);
        goto ERROR;
    }

    png_init_io(cover, cfp);
    png_init_io(output, ofp);

    png_read_info(cover, cover_info);
    width = png_get_image_width(cover, cover_info);
    height = png_get_image_height(cover, cover_info);
    uint64_t pixel_num = width * height;

    fseek(ifp, 0, SEEK_END);
    file_size = ftell(ifp);
    rewind(ifp);

    // Now consider the size of the payload and the number of pixels in the
    // cover. We have header information (program name and version number),
    // inital information (file name and size)
    // and the data of file.
    uint64_t total_size = sizeof(file2png_name) + sizeof(file2png_version) + sizeof(input_name_length) +
                          input_name_length + sizeof(file_size) + file_size;
    if (total_size * get_pixels_per_byte(stego) > height * width) {
        fprintf(stderr, "Error: Payload is too large for the cover.\n");
        goto ERROR;
    }

    int bytes_per_pixel_cover, bytes_per_pixel_output = 4;
    if (png_get_color_type(cover, cover_info) == PNG_COLOR_TYPE_RGB)
        bytes_per_pixel_cover = 3;
    else if (png_get_color_type(cover, cover_info) == PNG_COLOR_TYPE_RGBA)
        bytes_per_pixel_cover = 4;
    else {
        fprintf(stderr, "Error: Unsupported color type in cover file.\n");
        goto ERROR;
    }

    png_set_IHDR(output, output_info, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(output, output_info);

    cbuf.buffer = (png_bytep)png_malloc(cover, bytes_per_pixel_cover * width * sizeof(png_byte));
    cbuf.size = bytes_per_pixel_cover * width;
    if (!cbuf.buffer) {
        fprintf(stderr, "Error: Failed to allocate memory for line buffer.\n");
        goto ERROR;
    }
    obuf.buffer = (png_bytep)png_malloc(output, bytes_per_pixel_output * width * sizeof(png_byte));
    obuf.size = bytes_per_pixel_output * width;
    if (!obuf.buffer) {
        fprintf(stderr, "Error: Failed to allocate memory for line buffer.\n");
        goto ERROR;
    }

    stego_write_bytes(cover, &cbuf, bytes_per_pixel_cover, output, &obuf, (const uint8_t *)file2png_name,
                      sizeof(file2png_name));
    stego_write_bytes(cover, &cbuf, bytes_per_pixel_cover, output, &obuf, (const uint8_t *)file2png_version,
                      sizeof(file2png_version));
    stego_write_bytes(cover, &cbuf, bytes_per_pixel_cover, output, &obuf, (const uint8_t *)&input_name_length,
                      sizeof(input_name_length));
    stego_write_bytes(cover, &cbuf, bytes_per_pixel_cover, output, &obuf, input_name, input_name_length);
    stego_write_bytes(cover, &cbuf, bytes_per_pixel_cover, output, &obuf, (const uint8_t *)&file_size,
                      sizeof(file_size));

    png_byte tmp[8192];
    size_t read_len;
    while ((read_len = fread(tmp, 1, sizeof(tmp), ifp)) > 0) {
        stego_write_bytes(cover, &cbuf, bytes_per_pixel_cover, output, &obuf, tmp, read_len);
    }
    stego_write_finish(cover, &cbuf, bytes_per_pixel_cover, output, &obuf, height);
    png_write_end(output, output_info);

    png_free(output, obuf.buffer);
    png_free(cover, cbuf.buffer);
    png_destroy_read_struct(&cover, &cover_info, NULL);
    png_destroy_write_struct(&output, &output_info);
    fclose(ifp);
    fclose(cfp);
    fclose(ofp);
    if (!output_name && output_name_new)
        free((char *)output_name_new);
    return EXIT_SUCCESS;
ERROR:
    if (cbuf.buffer && cover) {
        png_free(cover, cbuf.buffer);
        cbuf.buffer = NULL;
    }
    if (obuf.buffer && output) {
        png_free(output, obuf.buffer);
        obuf.buffer = NULL;
    }
    if (cover && cover_info)
        png_destroy_read_struct(&cover, &cover_info, NULL);
    else if (cover)
        png_destroy_read_struct(&cover, NULL, NULL);
    if (output && output_info)
        png_destroy_write_struct(&output, &output_info);
    else if (output)
        png_destroy_write_struct(&output, NULL);

    if (output_name_new && !output_name)
        free((char *)output_name_new);
    if (ifp)
        fclose(ifp);
    if (cfp)
        fclose(cfp);
    if (ofp)
        fclose(ofp);
    return EXIT_FAILURE;
}

int stego_recover(const char *input_name, const char *output_name, stego_t stego)
{
    FILE *ifp = NULL, *ofp = NULL;
    uint64_t file_size = 0;
    uint16_t file_name_length = 0;
    png_structp input = NULL;
    png_infop input_info = NULL;
    uint32_t width = 0, height = 0;
    const char *output_name_new = NULL;
    line_buf buf = {0, 0, 0, NULL};
    char *initial_filename = NULL;
    stego_read_bytes_f *stego_read_bytes;

    if (stego == STEGO_LSB1) {
        stego_read_bytes = &stego_read_bytes_lsb1;
    }
    else if (stego == STEGO_LSB2) {
        stego_read_bytes = &stego_read_bytes_lsb2;
    }
    else {
        fprintf(stderr, "Error: Unsupported stego mode.\n");
        return EXIT_FAILURE;
    }

    get_new_file_name(output_name, input_name, output_name_new, ".file");

    ifp = fopen(input_name, "rb");
    if (!ifp) {
        fprintf(stderr, "Error: Failed to open input file '%s'.\n", input_name);
        goto ERROR;
    }

    ofp = fopen(output_name_new, "wb");
    if (!ofp) {
        fprintf(stderr, "Error: Failed to open output file '%s'.\n", output_name);
        goto ERROR;
    }

    input = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    input_info = png_create_info_struct(input);
    if (!input || !input_info) {
        fprintf(stderr, "Error: Failed to create PNG structures.\n");
        goto ERROR;
    }

    if (setjmp(png_jmpbuf(input))) {
        fprintf(stderr, "Error: during libpng processing.\n");
        goto ERROR;
    }

    png_init_io(input, ifp);

    png_read_info(input, input_info);
    width = png_get_image_width(input, input_info);
    height = png_get_image_height(input, input_info);
    uint64_t pixel_num = width * height;

    int bytes_per_pixel = 4;
    buf.size = bytes_per_pixel * width;
    buf.buffer = (png_bytep)png_malloc(input, buf.size * sizeof(png_byte));
    if (!buf.buffer) {
        fprintf(stderr, "Error: Failed to allocate memory for line buffer.\n");
        goto ERROR;
    }

    if (pixel_num < get_pixels_per_byte(stego) * (sizeof(file2png_name) + sizeof(file2png_version) +
                                                  sizeof(file_name_length) + sizeof(file_size))) {
        fprintf(stderr, "Error: Image size is too small for this program.\n");
        goto ERROR;
    }

    // read header information
    char header_name[sizeof(file2png_name)];
    uint16_t header_version[sizeof(file2png_version) / sizeof(file2png_version[0])];
    stego_read_bytes(input, &buf, (uint8_t *)header_name, sizeof(header_name));
    // check program name
    if (strcmp(file2png_name, header_name)) {
        fprintf(stderr, "Error: Invalid file format.\n");
        goto ERROR;
    }

    stego_read_bytes(input, &buf, (uint8_t *)header_version, sizeof(header_version));
    // check program version
    if (header_version[0] > file2png_version[0] // file.xx > my.xx
    ) {
        fprintf(stderr, "Error: Unsupported file format version: %u.%u.%u\n", header_version[0],
                header_version[1], header_version[2]);
        goto ERROR;
    }
    else {
        fprintf(stdout, "File format version: %u.%u.%u\n", header_version[0], header_version[1],
                header_version[2]);
    }

    // get length of initial file name
    stego_read_bytes(input, &buf, (uint8_t *)&file_name_length, sizeof(file_name_length));
    // check picture size
    if (pixel_num <
        get_pixels_per_byte(stego) * (sizeof(file2png_name) + sizeof(file2png_version) +
                                      sizeof(file_name_length) + sizeof(file_size) + file_name_length)) {
        fprintf(stderr, "Error: Initial file name length is too long.\n");
        goto ERROR;
    }
    initial_filename = (char *)malloc(file_name_length + 1);
    if (!initial_filename) {
        fprintf(stderr, "Error: Failed to allocate memory for file name.\n");
        goto ERROR;
    }
    // get initial file name
    stego_read_bytes(input, &buf, (uint8_t *)initial_filename, file_name_length);
    initial_filename[file_name_length] = '\0';
    fprintf(stdout, "Initial file name: %s\n", initial_filename);

    // get file size
    stego_read_bytes(input, &buf, (uint8_t *)&file_size, sizeof(file_size));
    // check picture size
    if (pixel_num < get_pixels_per_byte(stego) *
                        (sizeof(file2png_name) + sizeof(file2png_version) + sizeof(file_name_length) +
                         sizeof(file_size) + file_name_length + file_size)) {
        fprintf(stderr, "Error: File size is too long.\n");
        goto ERROR;
    }

    uint8_t tmp[8192];
    size_t write_len = 0;

    // read data from png and write to file
    while (write_len + sizeof(tmp) < file_size) {
        stego_read_bytes(input, &buf, tmp, sizeof(tmp));
        fwrite(tmp, sizeof(uint8_t), sizeof(tmp), ofp);
        write_len += sizeof(tmp);
    }
    stego_read_bytes(input, &buf, tmp, file_size - write_len);
    fwrite(tmp, sizeof(uint8_t), file_size - write_len, ofp);

    // release memory
    free(initial_filename);
    png_free(input, buf.buffer);
    png_destroy_read_struct(&input, &input_info, NULL);
    fclose(ofp);
    fclose(ifp);
    if (output_name_new && !output_name)
        free((char *)output_name_new);

    return EXIT_SUCCESS;
ERROR:
    if (initial_filename)
        free(initial_filename);
    if (buf.buffer && input)
        png_free(input, buf.buffer);
    if (input && input_info)
        png_destroy_read_struct(&input, &input_info, NULL);
    else if (input)
        png_destroy_read_struct(&input, NULL, NULL);
    if (ofp)
        fclose(ofp);
    if (ifp)
        fclose(ifp);
    if (output_name_new && !output_name)
        free((char *)output_name_new);
    return EXIT_FAILURE;
}

void stego_write_bytes_lsb2(png_structp cover, line_buf *cbuf, int bytes_per_pixel_cover, png_structp output,
                            line_buf *obuf, const uint8_t *bytes, size_t size)
{
    uint8_t pixel[4] = {0, 0, 0, 255};
    for (size_t i = 0; i < size; i++) {
        png_read_bytes(cover, cbuf, pixel, bytes_per_pixel_cover);
        for (size_t j = 0; j < 4; j++)
            pixel[j] = (pixel[j] & ~0x03) | ((bytes[i] >> (j * 2)) & 0x03);
        png_write_bytes(output, obuf, pixel, 4);
    }
}

void stego_write_finish(png_structp cover, line_buf *cbuf, int bytes_per_pixel_cover, png_structp output,
                        line_buf *obuf, uint32_t height)
{
    uint8_t pixel[4] = {0, 0, 0, 255};
    while (obuf->rw_times < height) {
        png_read_bytes(cover, cbuf, pixel, bytes_per_pixel_cover);
        png_write_bytes(output, obuf, pixel, 4);
    }
}

void stego_read_bytes_lsb2(png_structp png, line_buf *buf, uint8_t *bytes, size_t size)
{
    uint8_t pixel[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < size; i++) {
        png_read_bytes(png, buf, pixel, 4);
        bytes[i] = (pixel[0] & 0x03) | ((pixel[1] & 0x03) << 2) | ((pixel[2] & 0x03) << 4) |
                   ((pixel[3] & 0x03) << 6);
    }
}

void stego_write_bytes_lsb1(png_structp cover, line_buf *cbuf, int bytes_per_pixel_cover, png_structp output,
                            line_buf *obuf, const uint8_t *bytes, size_t size)
{
    uint8_t pixel[4] = {0, 0, 0, 255};
    for (size_t i = 0; i < size; i++) {
        png_read_bytes(cover, cbuf, pixel, bytes_per_pixel_cover);
        for (size_t j = 0; j < 4; j++)
            pixel[j] = (pixel[j] & ~0x01) | ((bytes[i] >> j) & 0x01);
        png_write_bytes(output, obuf, pixel, 4);
        png_read_bytes(cover, cbuf, pixel, bytes_per_pixel_cover);
        for (size_t j = 0; j < 4; j++)
            pixel[j] = (pixel[j] & ~0x01) | ((bytes[i] >> (j + 4)) & 0x01);
        png_write_bytes(output, obuf, pixel, 4);
    }
}

void stego_read_bytes_lsb1(png_structp png, line_buf *buf, uint8_t *bytes, size_t size)
{
    uint8_t pixel[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < size; i++) {
        png_read_bytes(png, buf, pixel, 4);
        bytes[i] = (pixel[0] & 0x01) | ((pixel[1] & 0x01) << 1) | ((pixel[2] & 0x01) << 2) |
                   ((pixel[3] & 0x01) << 3);
        png_read_bytes(png, buf, pixel, 4);
        bytes[i] |= ((pixel[0] & 0x01) << 4) | ((pixel[1] & 0x01) << 5) | ((pixel[2] & 0x01) << 6) |
                    ((pixel[3] & 0x01) << 7);
    }
}

int get_pixels_per_byte(stego_t stego)
{
    switch (stego) {
    case STEGO_LSB1:
        return 2;
    case STEGO_LSB2:
        return 1;
    default:
        return 0;
    }
}

#include <stdio.h>

#include <png.h>

#include "process.h"
int main(int argc, char * const *argv)
{
    file2png_ctx ctx = {FILE2PNG_FORWORDS, NULL, NULL};
    process_args(&ctx, argc, argv);
    
    return process_image(&ctx);
}
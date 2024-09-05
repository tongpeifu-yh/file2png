#include "process.h"
#include "public.h"


int main(int argc, char * const *argv)
{
    file2png_ctx ctx = {FILE2PNG_FORWARDS, 6, NULL, NULL};
    process_args(&ctx, argc, argv);
    
    return process_image(&ctx);
}
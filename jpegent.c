/*
 * gcc -O3 -o jpegent jpegent.c -ljpeg
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <jerror.h>

int main(int argc, char **argv) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *infile;
    jvirt_barray_ptr* coeff_arrays;
    JBLOCKARRAY buffer;
    JCOEFPTR blockptr;
    unsigned int x, y, i, j, c;
    int v;
    unsigned int threshold = 0, max = 0;
    const unsigned char revzigzag[64] = {
        63, 62, 55, 47, 54, 61, 60, 53,
        46, 39, 31, 38, 45, 52, 59, 58,
        51, 44, 37, 30, 23, 15, 22, 29,
        36, 43, 50, 57, 56, 49, 42, 35,
        28, 21, 14,  7,  6, 13, 20, 27,
        34, 41, 48, 40, 33, 26, 19, 12,
        5,  4,  11, 18, 25, 32, 24, 17,
        10,  3,  2,  9, 16, 8,  1,  0
    };
    char *out;
    char chunk[8];

    if (argc == 3) {
        threshold = atoi(argv[2]);
    } else if (argc == 1 || argc > 3) {
        fprintf(stderr, "Usage: %s file.jpg [threshold]\n", argv[0]);
        return 1;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    if ((infile = fopen(argv[1], "rb")) == NULL) {
        fprintf(stderr, "Can't open %s\n", argv[1]);
        return 1;
    }

    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    coeff_arrays = jpeg_read_coefficients(&cinfo);

    out = (char *) calloc(1, cinfo.comp_info->width_in_blocks * cinfo.comp_info->height_in_blocks * 3 + cinfo.comp_info->height_in_blocks);

    for (y = 0; y < cinfo.comp_info->height_in_blocks; y++) {
        buffer = (cinfo.mem->access_virt_barray) ((j_common_ptr) &cinfo, coeff_arrays[0], y, (JDIMENSION) 1, FALSE);
        for (x = 0; x < cinfo.comp_info->width_in_blocks; x++) {
            blockptr = buffer[0][x];
            for (i = 0; i < sizeof(revzigzag); i++) {
                j = revzigzag[i];
                v = (int) blockptr[j] * (int) cinfo.comp_info->quant_table->quantval[j];
                if (abs(v) > threshold)
                    break;
            }

            c = (int) sizeof(revzigzag) - i;
            if (max < c)
                max = c;

            sprintf(chunk, "%2d ", c);
            strcat(out, chunk);
        }
        strcat(out, "\n");
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    printf("P2\n%d\n%d\n%d\n", cinfo.comp_info->width_in_blocks, cinfo.comp_info->height_in_blocks, max);
    puts(out);
    free(out);

    return 0;
}
/*
 * gcc -O3 -o jpegent jpegent.c -ljpeg
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <jerror.h>
#include <math.h>

#define COEF_BITS   11
#define COEF_RANGE  2048

int main(int argc, char **argv) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *infile;

    jvirt_barray_ptr *coeff_arrays;
    JBLOCKARRAY buffer;
    JCOEFPTR blockptr;

    unsigned int x, y, w, h, i;
    int v;

    int freq[COEF_RANGE];
    double sum;
    unsigned int ent, max = 0, avg = 0;

    char *out, *p;
    unsigned int outlen, len;
    char chunk[5]; // " %3d" + '\0'

    if (argc == 1 || argc > 2) {
        fprintf(stderr, "Usage: %s file.jpg\n", argv[0]);
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
    w = cinfo.comp_info->width_in_blocks;
    h = cinfo.comp_info->height_in_blocks;

    outlen = sizeof(char) * (w * h * (sizeof(chunk) - 1) + h + 1);
    out = (char *) malloc(outlen);
    p = out;

    for (y = 0; y < h; y++) {
        *(p++) = '\n';

        buffer = (cinfo.mem->access_virt_barray)
            ((j_common_ptr) &cinfo, coeff_arrays[0], y, (JDIMENSION) 1, FALSE);

        for (x = 0; x < w; x++) {
            blockptr = buffer[0][x];

            for (i = 0; i < COEF_RANGE; i++)
                freq[i] = 0;

            for (i = 0; i < DCTSIZE2; i++) {
                v = blockptr[i] * cinfo.comp_info->quant_table->quantval[i];
                freq[v + COEF_RANGE / 2]++;
            }
            /*
                fprintf(stderr, " %4d", v);
                if ((i + 1) % DCTSIZE == 0)
                    fprintf(stderr, "\n");
            }
            fprintf(stderr, "\n");
            */

            sum = 0;
            for (i = 0; i < COEF_RANGE; i++)
                if (freq[i])
                    sum += freq[i] * log2(freq[i]);

            ent = 256 * ((log2(DCTSIZE2) - sum / DCTSIZE2) / COEF_BITS);
            if (max < ent)
                max = ent;
            avg += ent;

            snprintf(chunk, sizeof(chunk), " %3d", ent);
            len = strlen(chunk);
            if ((p + len) - out < outlen) {
                memcpy(p, chunk, len);
                p += len;
            }
        }
    }

    printf("P2\n%d\n%d\n%d # %d", w, h, max, avg / (w * h));
    puts(out);

    free(out);
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return 0;
}

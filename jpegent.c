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
    int hipass = 1;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *infile;

    jvirt_barray_ptr *coeff_arrays;
    JBLOCKARRAY buffer;
    JCOEFPTR blockptr;

    unsigned int x, y, w, h, i, j, l;
    int v;
    static const unsigned int jpeg_zigzag_order[DCTSIZE2] = {
       0,  1,  5,  6, 14, 15, 27, 28,
       2,  4,  7, 13, 16, 26, 29, 42,
       3,  8, 12, 17, 25, 30, 41, 43,
       9, 11, 18, 24, 31, 40, 44, 53,
      10, 19, 23, 32, 39, 45, 52, 54,
      20, 22, 33, 38, 46, 51, 55, 60,
      21, 34, 37, 47, 50, 56, 59, 61,
      35, 36, 48, 49, 57, 58, 62, 63
    };

    int freq[COEF_RANGE];
    double sum;
    unsigned int ent, max = 0, avg = 0;

    unsigned int *zx, *zy, xsum = 0, zxsum = 0, ysum = 0, zysum = 0;

    char *out, *p;
    unsigned int outlen, len;
    char chunk[5]; // " %3d" + '\0'

    if (argc == 3) {
        hipass = atoi(argv[2]);
        if (hipass < 0 || hipass >= DCTSIZE2 - 1) {
            fprintf(stderr, "0 <= hipass < %d!\n", DCTSIZE2 - 1);
            return 1;
        }
    } else if (argc == 1 || argc > 3) {
        fprintf(stderr, "Usage: %s file.jpg [hipass=1]\n", argv[0]);
        return 1;
    }
    l = DCTSIZE2 - hipass;

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

    zx = calloc(sizeof(zx), w);
    zy = calloc(sizeof(zy), h);

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
                j = jpeg_zigzag_order[i];
                if (j >= hipass) {
                    v = blockptr[j] * cinfo.comp_info->quant_table->quantval[j];
                    freq[v + COEF_RANGE / 2]++;
                }
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

            ent = 256 * ((log2(l) - sum / l) / COEF_BITS);
            if (max < ent)
                max = ent;
            avg += ent;

            zx[x] += ent;
            zy[y] += ent;

            snprintf(chunk, sizeof(chunk), " %3d", ent);
            len = strlen(chunk);
            if ((p + len) - out < outlen) {
                memcpy(p, chunk, len);
                p += len;
            }
        }
    }

    for (i = 0; i < w; i++) {
        zxsum += zx[i];
        xsum += i * zx[i];
    }

    for (i = 0; i < h; i++) {
        zysum += zy[i];
        ysum += i * zy[i];
    }

    printf("P2\n%d\n%d\n%d\n", w, h, max);
    printf("# %d %d %d", avg / (w * h), xsum / zxsum, ysum / zysum);
    puts(out);

    free(out);
    free(zx);
    free(zy);

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return 0;
}

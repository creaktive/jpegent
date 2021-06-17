#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jerror.h>
#include <jpeglib.h>

#include <math.h>

#define COEF_BITS   11
#define COEF_RANGE  2048
#define HEADER_LEN  40

void jpeg_entropy(const unsigned char *jpeg_data, size_t jpeg_size, int hipass) {
    static const int jpeg_zigzag_order[DCTSIZE2] = {
       0,  1,  5,  6, 14, 15, 27, 28,
       2,  4,  7, 13, 16, 26, 29, 42,
       3,  8, 12, 17, 25, 30, 41, 43,
       9, 11, 18, 24, 31, 40, 44, 53,
      10, 19, 23, 32, 39, 45, 52, 54,
      20, 22, 33, 38, 46, 51, 55, 60,
      21, 34, 37, 47, 50, 56, 59, 61,
      35, 36, 48, 49, 57, 58, 62, 63
    };

    int max = 0, avg = 0;
    int xsum = 0, zxsum = 0, ysum = 0, zysum = 0;

    char chunk[5]; // " %3d" + '\0'

    int l = DCTSIZE2 - hipass;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, jpeg_data, jpeg_size);
    jpeg_read_header(&cinfo, TRUE);

    jvirt_barray_ptr *coeff_arrays = jpeg_read_coefficients(&cinfo);
    int w = cinfo.comp_info->width_in_blocks;
    int h = cinfo.comp_info->height_in_blocks;

    int *zx = calloc(sizeof(zx), w);
    int *zy = calloc(sizeof(zy), h);

    int outlen = sizeof(char) * (w * h * (sizeof(chunk) - 1) + h + 1);
    char *out = (char *) malloc(outlen);
    char *p = out;

    for (int y = 0; y < h; y++) {
        *(p++) = '\n';

        JBLOCKARRAY buffer = (cinfo.mem->access_virt_barray)
            ((j_common_ptr) &cinfo, coeff_arrays[0], y, (JDIMENSION) 1, FALSE);

        for (int x = 0; x < w; x++) {
            JCOEFPTR blockptr = buffer[0][x];

            int freq[COEF_RANGE];
            for (int i = 0; i < COEF_RANGE; i++)
                freq[i] = 0;

            for (int i = 0; i < DCTSIZE2; i++) {
                int j = jpeg_zigzag_order[i];
                if (j >= hipass) {
                    int v = blockptr[j] * cinfo.comp_info->quant_table->quantval[j];
                    freq[v + COEF_RANGE / 2]++;
                }
            }

            double sum = 0;
            for (int i = 0; i < COEF_RANGE; i++)
                if (freq[i])
                    sum += freq[i] * log2(freq[i]);

            int ent = 256 * ((log2(l) - sum / l) / COEF_BITS);
            if (max < ent)
                max = ent;
            avg += ent;

            zx[x] += ent;
            zy[y] += ent;

            snprintf(chunk, sizeof(chunk), " %3d", ent);
            int len = strlen(chunk);
            if ((p + len) - out < outlen) {
                memcpy(p, chunk, len);
                p += len;
            }
        }
    }

    for (int i = 0; i < w; i++) {
        zxsum += zx[i];
        xsum += i * zx[i];
    }

    for (int i = 0; i < h; i++) {
        zysum += zy[i];
        ysum += i * zy[i];
    }

    printf("P2\n%d\n%d\n%d\n", w, h, max);
    printf("# %d %d %d", avg / (w * h), xsum / zxsum, ysum / zysum);
    puts(out);

    free(out);
    free(zx);
    free(zy);

    jpeg_abort_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
}

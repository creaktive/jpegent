#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <jpeglib.h>

#include "jpegent.h"

int main(int argc, char **argv) {
    int hipass = 1;
    if (argc == 3) {
        hipass = atoi(argv[2]);
        if (hipass < 0 || hipass >= DCTSIZE2 - 1) {
            fprintf(stderr, "0 <= hipass < %d!\n", DCTSIZE2 - 1);
            return 1;
        }
    } else if (argc == 1 || argc > 3) {
        fprintf(stderr, "Usage: %s file.jpg [hipass=1]\n", argv[0]);
        return 2;
    }

    int fd;
    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        fprintf(stderr, "Can't open() %s\n", argv[1]);
        return 3;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        fprintf(stderr, "Can't fstat() %s\n", argv[1]);
        return 4;
    }

    unsigned char *jpeg_data;
    jpeg_data = (unsigned char *) malloc(sb.st_size);
    if (read(fd, jpeg_data, sb.st_size) != sb.st_size) {
        fprintf(stderr, "Can't fread() %s\n", argv[1]);
        return 5;
    }

    char *result = jpeg_entropy(jpeg_data, sb.st_size, hipass);

    puts(result);

    free(jpeg_data);
    free(result);

    return 0;
}

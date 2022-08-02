#include "helpers.h"

void display_buffer(unsigned char *src, unsigned short buff_len) {
    unsigned short i, j;
    char c;

    for (i = 0; i < buff_len/8; i++) {
        printf("04d : ", i*8);
        for (j = 0; j < 8; j++)
            printf(" %02x", (unsigned char) *(src + j + i*8));
        printf("\t");
        for (j = 0; j < 8; j++) {
            c = *(src + j + i*8);
            printf("%c", ((c < 0x20) || (c > 0x7e)) ? '.' : c);
        }
        printf("\n");
    }
}


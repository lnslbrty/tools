#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')


void print_binary_hex(uint8_t *bin, size_t siz)
{
    size_t i;

    for (i = 0; i < siz; ++i) {
        printf("%8.02X ", bin[i]);
    }
    printf("\n");
    for (i = 0; i < siz; ++i) {
        printf("%8.d ", bin[i]);
    }
    printf("\n");
    for (i = 0; i < siz; ++i) {
        printf(BYTE_TO_BINARY_PATTERN " ", BYTE_TO_BINARY(bin[i]));
    }
    printf("\n");
}

ssize_t textify(uint8_t *bin, size_t siz, char *out, size_t outsiz)
{
    size_t i, j;

    if (!bin || !out)
        return -1;

    for (i = 0, j = 0; i < siz && j < outsiz / 3; ++i, j += 3) {
        uint8_t tmp0 = 32, tmp1 = 64, tmp2 = 64;

        if (bin[i] < 32)
            tmp1 = bin[i] + 32;
        else if (bin[i] >= 127) {
            if (bin[i] < 190)
                tmp1 = bin[i] - 63;
            else
                tmp2 = bin[i] - 63*2;
        } else {
            tmp0 = bin[i];
        }

        out[j+0] = tmp0;
        out[j+1] = tmp1;
        out[j+2] = tmp2;
    }

    return j;
}

ssize_t untextify(uint8_t *text, size_t siz, char *out, size_t outsiz)
{
    size_t i, j;

    if (!text || !out)
        return -1;

    for (i = 0, j = 0; i < siz / 3 && j < outsiz; i += 3, ++j) {
        if (text[i+1] >= 32 && text[i+1] < 64) {
            out[j] = text[i+1] - 32;
        } else if (text[i+1] >= 64 && text[i+1] < 96) {
            out[j] = text[i+1] + 63;
        } else if (text[i+2] >= 96 && text[i+1] < 127) {
            out[j] = text[i+2] + 63*2;
        }
    }

    return j;
}

int main(int argc, char **argv)
{
    ssize_t ret;
    char outbuf[BUFSIZ] = {0};
    char orgbuf[BUFSIZ] = {0};

    if (argc != 2) {
        printf("usage: %s [DATA]\n", argv[0]);
        return 1;
    }

    print_binary_hex((uint8_t *)argv[1], strnlen(argv[1], BUFSIZ));
    printf("\n");

    ret = textify((uint8_t *)argv[1], strnlen(argv[1], sizeof outbuf / 3),
                  outbuf, sizeof outbuf);
    if (ret < 0)
    {
        printf("%s: textify failed\n", argv[0]);
        return 1;
    }

    print_binary_hex((uint8_t *)outbuf, ret);
    outbuf[BUFSIZ-1] = 0;
    printf("%s result: '%s'\n\n", argv[0], outbuf);

    ret = untextify((uint8_t *)outbuf, ret, orgbuf, sizeof orgbuf);
    if (ret < 0)
    {
        printf("%s: untextify failed\n", outbuf);
        return 1;
    }

    print_binary_hex((uint8_t *) orgbuf, ret);
    orgbuf[BUFSIZ-1] = 0;
    printf("%s result: '%s'\n", argv[0], orgbuf);

    return 0;
}

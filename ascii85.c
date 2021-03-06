/** @file ascii85.c
 *
 * @brief Ascii85 encoder and decoder
 *
 * @par
 * @copyright Copyright © 2017 Doug Currie, Londonderry, NH, USA. All rights reserved.
 * 
 * @par
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
 * and associated documentation files (the "Software"), to deal in the Software without 
 * restriction, including without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or 
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**/

/* from: https://github.com/dcurrie/ascii85 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

enum ascii85_errs_e
{
    ascii85_err_out_buf_too_small = -255,
    ascii85_err_in_buf_too_large,
    ascii85_err_bad_decode_char,
    ascii85_err_decode_overflow
};

int32_t encode_ascii85 (const uint8_t *inp, int32_t in_length, char *outp, int32_t out_max_length);

int32_t decode_ascii85 (const char *inp, int32_t in_length, uint8_t *outp, int32_t out_max_length);

// From Wikipedia re: Ascii85 length...
// Adobe adopted the basic btoa encoding, but with slight changes, and gave it the name Ascii85.
// The characters used are the ASCII characters 33 (!) through 117 (u) inclusive (to represent
// the base-85 digits 0 through 84), together with the letter z (as a special case to represent
// a 32-bit 0 value), and white space is ignored. Adobe uses the delimiter "~>" to mark the end
// of an Ascii85-encoded string, and represents the length by truncating the final group: If the
// last block of source bytes contains fewer than 4 bytes, the block is padded with up to three
// null bytes before encoding. After encoding, as many bytes as were added as padding are
// removed from the end of the output.
// The reverse is applied when decoding: The last block is padded to 5 bytes with the Ascii85
// character "u", and as many bytes as were added as padding are omitted from the end of the
// output (see example).
// NOTE: The padding is not arbitrary. Converting from binary to base 64 only regroups bits and
// does not change them or their order (a high bit in binary does not affect the low bits in the
// base64 representation). In converting a binary number to base85 (85 is not a power of two)
// high bits do affect the low order base85 digits and conversely. Padding the binary low (with
// zero bits) while encoding and padding the base85 value high (with 'u's) in decoding assures
// that the high order bits are preserved (the zero padding in the binary gives enough room so
// that a small addition is trapped and there is no "carry" to the high bits).

// NOTE: ths implementation does not ignore white space!
//
// The motivation for this implementation is as a binary message wrapper for serial
// communication; in that application, white space is used for message framing.

static const uint8_t base_char = 33u; // '!' -- note that (85 + 33) < 128

static const int32_t ascii85_in_length_max = 65536;
static const bool ascii85_check_decode_chars = true;
#define ENDECODE_NUL_AS_Z 1

#if 0
static inline bool ascii85_char_ok (uint8_t c)
{
    return ((c >= 33u) && (c <= 117u));
}
#endif

static inline bool ascii85_char_ng (uint8_t c)
{
    return ((c < 33u) || (c > 117u));
}

/*!
 * @brief encode_ascii85: encode binary input into Ascii85
 * @param[in] inp pointer to a buffer of unsigned bytes 
 * @param[in] in_length the number of bytes at inp to encode
 * @param[in] outp pointer to a buffer for the encoded data as c-string
 * @param[in] out_max_length available space at outp in bytes; must be >= 5 * ceiling(in_length/4)
 * @return number of bytes in the encoded value at outp if non-negative; error code from
 * ascii85_errs_e if negative
 * @par Possible errors include: ascii85_err_in_buf_too_large, ascii85_err_out_buf_too_small
 */
int32_t encode_ascii85 (const uint8_t *inp, int32_t in_length, char *outp, int32_t out_max_length)
{
    // Note that (in_length + 3) below may overflow, but this is inconsequental
    // since ascii85_in_length_max is < (INT32_MAX - 3), and we check in_length before
    // using the calculated out_length.
    //
    int32_t out_length = (((in_length + 3) / 4) * 5); // ceiling

    if (in_length > ascii85_in_length_max)
    {
        out_length = (int32_t )ascii85_err_in_buf_too_large;
    }
    else if (out_length > out_max_length)
    {
        out_length = (int32_t )ascii85_err_out_buf_too_small;
    }
    else
    {
        int32_t in_rover = 0;

        out_length = 0; // we know we can increment by 5 * ceiling(in_length/4)

        while (in_rover < in_length)
        {
            uint32_t chunk;
            int32_t chunk_len = in_length - in_rover;

            if (chunk_len >= 4)
            {
                chunk  = (((uint32_t )inp[in_rover++]) << 24u);
                chunk |= (((uint32_t )inp[in_rover++]) << 16u);
                chunk |= (((uint32_t )inp[in_rover++]) <<  8u);
                chunk |= (((uint32_t )inp[in_rover++])       );
            }
            else
            {
                chunk  =                           (((uint32_t )inp[in_rover++]) << 24u);
                chunk |= ((in_rover < in_length) ? (((uint32_t )inp[in_rover++]) << 16u) : 0u);
                chunk |= ((in_rover < in_length) ? (((uint32_t )inp[in_rover++]) <<  8u) : 0u);
                chunk |= ((in_rover < in_length) ? (((uint32_t )inp[in_rover++])       ) : 0u);
            }

#ifdef ENDECODE_NUL_AS_Z
            if (/*lint -e{506} -e{774}*/ (0u == chunk) && (chunk_len >= 4))
            {
                outp[out_length++] = (uint8_t )'z';
            }
            else
#endif
            {
                outp[out_length + 4] = (chunk % 85u) + base_char;
                chunk /= 85u;
                outp[out_length + 3] = (chunk % 85u) + base_char;
                chunk /= 85u;
                outp[out_length + 2] = (chunk % 85u) + base_char;
                chunk /= 85u;
                outp[out_length + 1] = (chunk % 85u) + base_char;
                chunk /= 85u;
                outp[out_length    ] = (uint8_t )chunk + base_char;
                // we don't need (chunk % 85u) on the last line since (((((2^32 - 1) / 85) / 85) / 85) / 85) = 82.278

                if (chunk_len >= 4)
                {
                    out_length += 5;
                }
                else
                {
                    out_length += (chunk_len + 1); // see note above re: Ascii85 length
                }
            }
        }
    }

    return out_length;
}

/*!
 * @brief decode_ascii85: decode Ascii85 input to binary output
 * @param[in] inp pointer to a buffer of Ascii85 encoded c-string
 * @param[in] in_length the number of bytes at inp to decode
 * @param[in] outp pointer to a buffer for the decoded data
 * @param[in] out_max_length available space at outp in bytes; must be >= 4 * ceiling(in_length/5)
 * @return number of bytes in the decoded value at outp if non-negative; error code from
 * ascii85_errs_e if negative
 * @par Possible errors include: ascii85_err_in_buf_too_large, ascii85_err_out_buf_too_small, 
 * ascii85_err_bad_decode_char, ascii85_err_decode_overflow
 */
int32_t decode_ascii85 (const char *inp, int32_t in_length, uint8_t *outp, int32_t out_max_length)
{
    // Note that (in_length + 4) below may overflow, but this is inconsequental
    // since ascii85_in_length_max is < (INT32_MAX - 4), and we check in_length before
    // using the calculated out_length.
    //
    int32_t out_length = (((in_length + 4) / 5) * 4); // ceiling

    if (in_length > ascii85_in_length_max)
    {
        out_length = (int32_t )ascii85_err_in_buf_too_large;
    }
    else if (out_length > out_max_length)
    {
        out_length = (int32_t )ascii85_err_out_buf_too_small;
    }
    else
    {
        int32_t in_rover = 0;

        out_length = 0; // we know we can increment by 4 * ceiling(in_length/5)

        while (in_rover < in_length)
        {
            uint32_t chunk;
            int32_t chunk_len = in_length - in_rover;

#ifdef ENDECODE_NUL_AS_Z
            if (/*lint -e{506} -e{774}*/ ((uint8_t )'z' == inp[in_rover]))
            {
                in_rover += 1;
                chunk = 0u;
                chunk_len = 5; // to make out_length increment correct
            }
            else
#endif
            if (/*lint -e{506} -e{774}*/ascii85_check_decode_chars
                    && (                       ascii85_char_ng(inp[in_rover    ])
                        || ((chunk_len > 1) && ascii85_char_ng(inp[in_rover + 1]))
                        || ((chunk_len > 2) && ascii85_char_ng(inp[in_rover + 2]))
                        || ((chunk_len > 3) && ascii85_char_ng(inp[in_rover + 3]))
                        || ((chunk_len > 4) && ascii85_char_ng(inp[in_rover + 4]))))
            {
                out_length = (int32_t )ascii85_err_bad_decode_char;
                break; // leave while loop early to report error
            }
            else if (chunk_len >= 5)
            {
                chunk  = inp[in_rover++] - base_char;
                chunk *= 85u; // max: 84 * 85 = 7,140
                chunk += inp[in_rover++] - base_char;
                chunk *= 85u; // max: (84 * 85 + 84) * 85 = 614,040
                chunk += inp[in_rover++] - base_char;
                chunk *= 85u; // max: (((84 * 85 + 84) * 85) + 84) * 85 = 52,200,540
                chunk += inp[in_rover++] - base_char;
                // max: (((((84 * 85 + 84) * 85) + 84) * 85) + 84) * 85 = 4,437,053,040 oops! 0x108780E70
                if (chunk > (UINT32_MAX / 85u))
                {
                    // multiply would overflow
                    out_length = (int32_t )ascii85_err_decode_overflow; // bad input
                    break; // leave while loop early to report error
                }
                else
                {
                    uint8_t addend = inp[in_rover++] - base_char;

                    chunk *= 85u; // multiply will not overflow due to test above

                    if (chunk > (UINT32_MAX - addend))
                    {
                        /// add would overflow
                        out_length = (int32_t )ascii85_err_decode_overflow; // bad input
                        break; // leave while loop early to report error
                    }
                    else
                    {
                        chunk += addend;
                    }
                }
            }
            else
            {
                chunk  = inp[in_rover++] - base_char;
                chunk *= 85u; // max: 84 * 85 = 7,140
                chunk += ((in_rover < in_length) ? (inp[in_rover++] - base_char) : 84u);
                chunk *= 85u; // max: (84 * 85 + 84) * 85 = 614,040
                chunk += ((in_rover < in_length) ? (inp[in_rover++] - base_char) : 84u);
                chunk *= 85u; // max: (((84 * 85 + 84) * 85) + 84) * 85 = 52,200,540
                chunk += ((in_rover < in_length) ? (inp[in_rover++] - base_char) : 84u);
                // max: (((((84 * 85 + 84) * 85) + 84) * 85) + 84) * 85 = 4,437,053,040 oops! 0x108780E70
                if (chunk > (UINT32_MAX / 85u))
                {
                    // multiply would overflow
                    out_length = (int32_t )ascii85_err_decode_overflow; // bad input
                    break; // leave while loop early to report error
                }
                else
                {
                    uint8_t addend = (uint8_t )((in_rover < in_length) ? (inp[in_rover++] - base_char) : 84u);

                    chunk *= 85u; // multiply will not overflow due to test above

                    if (chunk > (UINT32_MAX - addend))
                    {
                        /// add would overflow
                        out_length = (int32_t )ascii85_err_decode_overflow; // bad input
                        break; // leave while loop early to report error
                    }
                    else
                    {
                        chunk += addend;
                    }
                }
            }

            outp[out_length + 3] = (chunk % 256u);
            chunk /= 256u;
            outp[out_length + 2] = (chunk % 256u);
            chunk /= 256u;
            outp[out_length + 1] = (chunk % 256u);
            chunk /= 256u;
            outp[out_length    ] = (uint8_t )chunk;
            // we don't need (chunk % 256u) on the last line since ((((2^32 - 1) / 256u) / 256u) / 256u) = 255

            if (chunk_len >= 5)
            {
                out_length += 4;
            }
            else
            {
                out_length += (chunk_len - 1); // see note above re: Ascii85 length
            }
        }
    }

    return out_length;
}

int main(int argc, char **argv) {
    char out_enc[BUFSIZ];
    uint8_t out_dec[BUFSIZ];
    size_t buflen;
    int32_t siz_enc, siz_dec;

#if ENDECODE_NUL_AS_Z
    printf("Encoding NUL characters won't work from cmdline!\n");
#endif
    if (argc != 2) {
        printf("usage: %s [BINARY-DATA]\n", (argc > 0 ? argv[0] : "null"));
        exit(1);
    }

    memset(out_enc, 0, sizeof out_enc);
    memset(out_dec, 0, sizeof out_dec);

    buflen = strlen(argv[1]);
    siz_enc = encode_ascii85((uint8_t *) argv[1], buflen, out_enc, sizeof out_enc - 1);
    printf("Encoded: \"%.*s\"\n", siz_enc, (char *) out_enc);
    siz_dec = decode_ascii85(out_enc, siz_enc, out_dec, sizeof out_dec - 1);
    printf("Decoded: \"%.*s\"\n", siz_dec, (char *) out_dec);

    if ((int32_t) buflen == siz_dec &&
        memcmp(out_dec, argv[1], buflen) == 0)
    {
        printf("%s: Ok!\n", argv[0]);
    } else printf("%s: FAIL!\n", argv[0]);

    return 0;
}

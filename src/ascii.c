#ifndef ASCII_C
#define ASCII_C

#define ASCII_TS 0x01
#define ASCII_BS 0x02
#define ASCII_LS 0x04
#define ASCII_RS 0x08
#define ASCII_TD 0x10
#define ASCII_BD 0x20
#define ASCII_LD 0x40
#define ASCII_RD 0x80

#define ASCII_SIGNLE 0
#define ASCII_DOUBLE 1
#define ASCII_NONE   2
#define ASCII_ANY    3

#define ASCII_EDGE_CATEGORY(t, b, l, r) ((t) + (b) * 4 + (l) * 16 + (r) * 64)


typedef enum
{
    /* edges */

    // { line 'L' | corner 'C' | t junction 'T' | cross 'X' }
    // .
    // { single 'S' | double 'D' | horizontal double 'H' | vertical double 'V' }
    // .
    // { horizontal 'H' | vertical 'V' | '' }
    // .
    // { top 'T' | bottom 'B' | '' }
    // .
    // { left 'L' | right 'R' | '' }

    AsciiLSH = 192 + 14,
    AsciiLSV,
    AsciiCSTL,
    AsciiCSTR,
    AsciiCSBL,
    AsciiCSBR,
    AsciiTSL,
    AsciiTSR,
    AsciiTST,
    AsciiTSB,
    AsciiXS,
    AsciiLDH,
    AsciiLDV,
    AsciiCHTL,
    AsciiCVTL,
    AsciiCDTL,
    AsciiCHTR,
    AsciiCVTR,
    AsciiCDTR,
    AsciiCHBL,
    AsciiCVBL,
    AsciiCDBL,
    AsciiCHBR,
    AsciiCVBR,
    AsciiCDBR,
    AsciiTHL,
    AsciiTVL,
    AsciiTDL,
    AsciiTHR,
    AsciiTVR,
    AsciiTDR,
    AsciiTHT,
    AsciiTVT,
    AsciiTDT,
    AsciiTHB,
    AsciiTVB,
    AsciiTDB,
    AsciiXH,
    AsciiXV,
    AsciiXD,

    AsciiGR1 = 251,
    AsciiGR2,
    AsciiGR3,
} ascii_t;

#define ASCII_EDGES_BEGIN AsciiLSH
#define ASCII_EDGES_END   (AsciiXD + 1)


const unsigned ascii_edge_connections[] = {
    /* connections are just complements of name specifiers */
    [AsciiLSH]  = ASCII_LS | ASCII_RS,
    [AsciiLSV]  = ASCII_TS | ASCII_BS,
    [AsciiCSTL] = ASCII_RS | ASCII_BS,
    [AsciiCSTR] = ASCII_LS | ASCII_BS,
    [AsciiCSBL] = ASCII_RS | ASCII_TS,
    [AsciiCSBR] = ASCII_LS | ASCII_TS,
    [AsciiTSL]  = ASCII_RS | ASCII_TS | ASCII_BS,
    [AsciiTSR]  = ASCII_LS | ASCII_TS | ASCII_BS,
    [AsciiTST]  = ASCII_RS | ASCII_LS | ASCII_BS,
    [AsciiTSB]  = ASCII_RS | ASCII_LS | ASCII_TS,
    [AsciiXS]   = ASCII_RS | ASCII_LS | ASCII_TS | ASCII_BS,
    [AsciiLDH]  = ASCII_LD | ASCII_RD,
    [AsciiLDV]  = ASCII_TD | ASCII_BD,
    [AsciiCHTL] = ASCII_RD | ASCII_BS,
    [AsciiCVTL] = ASCII_RS | ASCII_BD,
    [AsciiCDTL] = ASCII_RD | ASCII_BD,
    [AsciiCHTR] = ASCII_LD | ASCII_BS,
    [AsciiCVTR] = ASCII_LS | ASCII_BD,
    [AsciiCDTR] = ASCII_LD | ASCII_BD,
    [AsciiCHBL] = ASCII_RD | ASCII_TS,
    [AsciiCVBL] = ASCII_RS | ASCII_TD,
    [AsciiCDBL] = ASCII_RD | ASCII_TD,
    [AsciiCHBR] = ASCII_LD | ASCII_TS,
    [AsciiCVBR] = ASCII_LS | ASCII_TD,
    [AsciiCDBR] = ASCII_LD | ASCII_TD,
    [AsciiTHL]  = ASCII_RD | ASCII_TS | ASCII_BS,
    [AsciiTVL]  = ASCII_RS | ASCII_TD | ASCII_BD,
    [AsciiTDL]  = ASCII_RD | ASCII_TD | ASCII_BD,
    [AsciiTHR]  = ASCII_LD | ASCII_TS | ASCII_BS,
    [AsciiTVR]  = ASCII_LS | ASCII_TD | ASCII_BD,
    [AsciiTDR]  = ASCII_LD | ASCII_TD | ASCII_BD,
    [AsciiTHT]  = ASCII_LD | ASCII_RD | ASCII_BS,
    [AsciiTVT]  = ASCII_LS | ASCII_RS | ASCII_BD,
    [AsciiTDT]  = ASCII_LD | ASCII_RD | ASCII_BD,
    [AsciiTHB]  = ASCII_LD | ASCII_RD | ASCII_TS,
    [AsciiTVB]  = ASCII_LS | ASCII_RS | ASCII_TD,
    [AsciiTDB]  = ASCII_LD | ASCII_RD | ASCII_TD,
    [AsciiXH]   = ASCII_RD | ASCII_LD | ASCII_TS | ASCII_BS,
    [AsciiXV]   = ASCII_RS | ASCII_LS | ASCII_TD | ASCII_BD,
    [AsciiXD]   = ASCII_RD | ASCII_LD | ASCII_TD | ASCII_BD,
};


#ifdef META

#include "meta.h"
#include "utils.h"

#define META_FILE_NAME "ascii.meta.h"

int main(void)
{
    FILE *file = fopen("src/"META_FILE_NAME, "w");
    file_check(file, "src/"META_FILE_NAME);

    meta_insert_header(file);

    fclose(file);

    return 0;
}

#else
#include "ascii.meta.h"
#endif // META


#endif // ASCII_C

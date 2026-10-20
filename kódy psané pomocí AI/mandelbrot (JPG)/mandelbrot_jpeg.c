/*
 * Mandelbrot -> JPEG (baseline, 4:4:4), BEZ EXTERNÍCH KNIHOVEN.
 * Počítá se na všech jádrech (OpenMP) a JPEG se zapisuje průběžně,
 * takže se celý obrázek nikdy nedrží v paměti.
 *
 * Kompilace (gcc):  gcc -O3 -march=native -fopenmp mandelbrot_jpeg.c -o mandelbrot_jpeg.exe
 * Kompilace (MSVC): cl /O2 /openmp mandelbrot_jpeg.c
 */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#ifdef _OPENMP
#include <omp.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

/* --- KONFIGURACE --- */
#define MAX_ITER 100
#define BATCH_BR 16      /* kolik pruhů po 8 řádcích se zpracuje najednou */

static const double X_MIN = -2.0, X_MAX = 0.5;
static const double Y_MIN = -1.25, Y_MAX = 1.25;

/* úroveň komprese 1..9 -> JPEG kvalita (1 = nejmenší komprese, 9 = největší) */
static const int LEVEL_TO_QUALITY[10] = { 0, 95, 88, 80, 70, 60, 50, 35, 20, 10 };

/* ------------------------------------------------------------------ */
/*  JPEG tabulky (ITU-T T.81, příloha K)                               */
/* ------------------------------------------------------------------ */
static const uint8_t Q_LUM[64] = {
    16,11,10,16,24,40,51,61, 12,12,14,19,26,58,60,55,
    14,13,16,24,40,57,69,56, 14,17,22,29,51,87,80,62,
    18,22,37,56,68,109,103,77, 24,35,55,64,81,104,113,92,
    49,64,78,87,103,121,120,101, 72,92,95,98,112,100,103,99 };
static const uint8_t Q_CHR[64] = {
    17,18,24,47,99,99,99,99, 18,21,26,66,99,99,99,99,
    24,26,56,99,99,99,99,99, 47,66,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99, 99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99, 99,99,99,99,99,99,99,99 };

static const uint8_t ZIGZAG[64] = {
    0, 1, 8,16, 9, 2, 3,10, 17,24,32,25,18,11, 4, 5,
    12,19,26,33,40,48,41,34, 27,20,13, 6, 7,14,21,28,
    35,42,49,56,57,50,43,36, 29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46, 53,60,61,54,47,55,62,63 };

static const uint8_t DC_BITS[16] = { 0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
static const uint8_t DC_VALS[12] = { 0,1,2,3,4,5,6,7,8,9,10,11 };

static const uint8_t AC_BITS[16] = { 0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };
static const uint8_t AC_VALS[162] = {
    0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,
    0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,
    0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
    0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
    0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
    0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
    0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
    0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,
    0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
    0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
    0xf9,0xfa };

/* ------------------------------------------------------------------ */
/*  Globální (po inicializaci jen pro čtení) data                       */
/* ------------------------------------------------------------------ */
static uint8_t  qtab[2][64];           /* kvantizační tabulky v zig-zag pořadí */
static float    qrec[2][64];           /* 1/q v přirozeném pořadí */
static float    dctC[8][8];            /* DCT matice */
static uint16_t dc_code[16], ac_code[256];
static uint8_t  dc_size[16], ac_size[256];

static void build_huff(const uint8_t *bits, const uint8_t *vals, uint16_t *code, uint8_t *size)
{
    unsigned c = 0; int k = 0;
    for (int len = 1; len <= 16; len++) {
        for (int i = 0; i < bits[len - 1]; i++, k++) {
            code[vals[k]] = (uint16_t)c;
            size[vals[k]] = (uint8_t)len;
            c++;
        }
        c <<= 1;
    }
}

static void init_tables(int quality)
{
    int scale = quality < 50 ? 5000 / quality : 200 - 2 * quality;
    for (int t = 0; t < 2; t++) {
        const uint8_t *base = t ? Q_CHR : Q_LUM;
        for (int i = 0; i < 64; i++) {
            int v = (base[i] * scale + 50) / 100;
            if (v < 1) v = 1;
            if (v > 255) v = 255;
            qtab[t][i] = (uint8_t)v;                 /* přirozené pořadí, zig-zag až při zápisu DQT */
            qrec[t][i] = 1.0f / (float)v;
        }
    }
    for (int u = 0; u < 8; u++)
        for (int x = 0; x < 8; x++)
            dctC[u][x] = (float)((u == 0 ? sqrt(0.125) : 0.5) * cos((2 * x + 1) * u * 3.14159265358979323846 / 16.0));

    memset(dc_size, 0, sizeof dc_size);
    memset(ac_size, 0, sizeof ac_size);
    build_huff(DC_BITS, DC_VALS, dc_code, dc_size);
    build_huff(AC_BITS, AC_VALS, ac_code, ac_size);
}

/* ------------------------------------------------------------------ */
/*  Zápis bitů do souboru                                               */
/* ------------------------------------------------------------------ */
typedef struct {
    FILE    *f;
    uint8_t *buf;
    size_t   pos, cap;
    uint64_t acc;
    int      nbits;
} BitWriter;

static void bw_flush(BitWriter *bw)
{
    if (bw->pos) { fwrite(bw->buf, 1, bw->pos, bw->f); bw->pos = 0; }
}

static inline void bw_byte(BitWriter *bw, uint8_t b)
{
    if (bw->pos + 2 > bw->cap) bw_flush(bw);
    bw->buf[bw->pos++] = b;
}

static inline void bw_put(BitWriter *bw, unsigned code, int size)
{
    if (!size) return;
    bw->acc = (bw->acc << size) | (code & ((1u << size) - 1));
    bw->nbits += size;
    while (bw->nbits >= 8) {
        uint8_t b = (uint8_t)(bw->acc >> (bw->nbits - 8));
        bw_byte(bw, b);
        if (b == 0xFF) bw_byte(bw, 0x00);       /* byte stuffing */
        bw->nbits -= 8;
    }
    bw->acc &= ((uint64_t)1 << bw->nbits) - 1;
}

static inline int category(int v)
{
    if (v < 0) v = -v;
    int s = 0;
    while (v) { s++; v >>= 1; }
    return s;
}

static void encode_block(BitWriter *bw, const int16_t *c, int *prev_dc)
{
    int diff = c[0] - *prev_dc;
    *prev_dc = c[0];

    int s = category(diff);
    bw_put(bw, dc_code[s], dc_size[s]);
    if (s) bw_put(bw, (unsigned)(diff < 0 ? diff - 1 : diff), s);

    int run = 0;
    for (int k = 1; k < 64; k++) {
        if (c[k] == 0) { run++; continue; }
        while (run > 15) { bw_put(bw, ac_code[0xF0], ac_size[0xF0]); run -= 16; }
        int v = c[k];
        s = category(v);
        int sym = (run << 4) | s;
        bw_put(bw, ac_code[sym], ac_size[sym]);
        bw_put(bw, (unsigned)(v < 0 ? v - 1 : v), s);
        run = 0;
    }
    if (run) bw_put(bw, ac_code[0x00], ac_size[0x00]);   /* EOB */
}

/* ------------------------------------------------------------------ */
/*  Hlavičky JPEG                                                       */
/* ------------------------------------------------------------------ */
static void put8(FILE *f, int v)  { fputc(v & 0xFF, f); }
static void put16(FILE *f, int v) { fputc((v >> 8) & 0xFF, f); fputc(v & 0xFF, f); }

static void write_headers(FILE *f, int w, int h)
{
    /* SOI + JFIF APP0 */
    put16(f, 0xFFD8);
    put16(f, 0xFFE0); put16(f, 16);
    fwrite("JFIF", 1, 5, f);
    put8(f, 1); put8(f, 1);            /* verze 1.01 */
    put8(f, 0);                        /* bez jednotek */
    put16(f, 1); put16(f, 1);
    put8(f, 0); put8(f, 0);

    /* DQT */
    for (int t = 0; t < 2; t++) {
        put16(f, 0xFFDB); put16(f, 67);
        put8(f, t);
        for (int i = 0; i < 64; i++) put8(f, qtab[t][ZIGZAG[i]]);
    }

    /* SOF0: 8 bitů, 3 složky, bez podvzorkování */
    put16(f, 0xFFC0); put16(f, 17);
    put8(f, 8);
    put16(f, h); put16(f, w);
    put8(f, 3);
    put8(f, 1); put8(f, 0x11); put8(f, 0);
    put8(f, 2); put8(f, 0x11); put8(f, 1);
    put8(f, 3); put8(f, 0x11); put8(f, 1);

    /* DHT: DC (třída 0) a AC (třída 1), společné pro všechny složky */
    put16(f, 0xFFC4); put16(f, 2 + 1 + 16 + 12);
    put8(f, 0x00); fwrite(DC_BITS, 1, 16, f); fwrite(DC_VALS, 1, 12, f);
    put16(f, 0xFFC4); put16(f, 2 + 1 + 16 + 162);
    put8(f, 0x10); fwrite(AC_BITS, 1, 16, f); fwrite(AC_VALS, 1, 162, f);

    /* SOS */
    put16(f, 0xFFDA); put16(f, 12);
    put8(f, 3);
    put8(f, 1); put8(f, 0x00);
    put8(f, 2); put8(f, 0x00);
    put8(f, 3); put8(f, 0x00);
    put8(f, 0); put8(f, 63); put8(f, 0);
}

/* ------------------------------------------------------------------ */
/*  Výpočet jednoho 8x8 bloku: Mandelbrot -> YCbCr -> DCT -> kvantizace */
/* ------------------------------------------------------------------ */
static void compute_block(int bx, int by, int W, int H, int16_t out[3][64])
{
    float s[3][8][8];

    for (int j = 0; j < 8; j++) {
        int py = by * 8 + j; if (py >= H) py = H - 1;      /* okraj se doplní kopií */
        double im = Y_MIN + ((double)py / H) * (Y_MAX - Y_MIN);
        for (int i = 0; i < 8; i++) {
            int px = bx * 8 + i; if (px >= W) px = W - 1;
            double re = X_MIN + ((double)px / W) * (X_MAX - X_MIN);

            double zr = 0.0, zi = 0.0;
            int it = 0;
            while (zr * zr + zi * zi <= 4.0 && it < MAX_ITER) {
                double nr = zr * zr - zi * zi + re;
                double ni = 2.0 * zr * zi + im;
                zr = nr; zi = ni;
                it++;
            }

            float r = 0, g = 0, b = 0;
            if (it != MAX_ITER) {
                int ri = it * 2, gi = it * 3, bi = it * 8;
                r = (float)(ri > 255 ? 255 : ri);
                g = (float)(gi > 255 ? 255 : gi);
                b = (float)(bi > 255 ? 255 : bi);
            }
            s[0][j][i] =  0.299f    * r + 0.587f    * g + 0.114f    * b - 128.0f;
            s[1][j][i] = -0.168736f * r - 0.331264f * g + 0.5f      * b;
            s[2][j][i] =  0.5f      * r - 0.418688f * g - 0.081312f * b;
        }
    }

    for (int c = 0; c < 3; c++) {
        float tmp[8][8], d[64];
        /* tmp = s * C^T (po řádcích) */
        for (int j = 0; j < 8; j++)
            for (int u = 0; u < 8; u++) {
                float a = 0;
                for (int i = 0; i < 8; i++) a += s[c][j][i] * dctC[u][i];
                tmp[j][u] = a;
            }
        /* d = C * tmp (po sloupcích) */
        for (int v = 0; v < 8; v++)
            for (int u = 0; u < 8; u++) {
                float a = 0;
                for (int j = 0; j < 8; j++) a += dctC[v][j] * tmp[j][u];
                d[v * 8 + u] = a;
            }
        const float *qr = qrec[c ? 1 : 0];
        for (int k = 0; k < 64; k++) {
            int idx = ZIGZAG[k];
            out[c][k] = (int16_t)lrintf(d[idx] * qr[idx]);
        }
    }
}

/* ------------------------------------------------------------------ */
int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);   /* aby se správně zobrazila čeština */
#endif

    int size = 0, level = 0;

    printf("Mandelbrot -> JPEG\n");
    printf("Strana čtvercového obrázku v pixelech (8 - 65535): ");
    if (scanf("%d", &size) != 1 || size < 8 || size > 65535) {
        printf("Neplatná velikost. JPEG podporuje strany 8 až 65535 px.\n");
        return 1;
    }
    printf("Úroveň komprese 1-9 (1 = nejmenší komprese / nejlepší kvalita, 9 = největší komprese): ");
    if (scanf("%d", &level) != 1 || level < 1 || level > 9) {
        printf("Neplatná úroveň, zadej číslo 1 až 9.\n");
        return 1;
    }

    int quality = LEVEL_TO_QUALITY[level];
    init_tables(quality);

    char filename[64];
    snprintf(filename, sizeof filename, "mandelbrot_%dpx_komprese%d.jpg", size, level);

    FILE *f = fopen(filename, "wb");
    if (!f) { perror("fopen"); return 1; }
    static char iobuf[1 << 16];
    setvbuf(f, iobuf, _IOFBF, sizeof iobuf);

    const int W = size, H = size;
    const int bw = (W + 7) / 8;              /* bloků na řádek */
    const int bh = (H + 7) / 8;              /* pruhů po 8 řádcích */

#ifdef _OPENMP
    printf("Počítám na %d vláknech...\n", omp_get_max_threads());
#else
    printf("Počítám na 1 vlákně (bez OpenMP)...\n");
#endif
    printf("Soubor: %s (kvalita JPEG %d)\n", filename, quality);

    write_headers(f, W, H);

    BitWriter bwr = { f, malloc(1 << 20), 0, 1 << 20, 0, 0 };
    int16_t (*coef)[3][64] = malloc((size_t)BATCH_BR * bw * sizeof *coef);
    if (!bwr.buf || !coef) { fprintf(stderr, "Nedostatek paměti\n"); fclose(f); return 1; }

    int prev_dc[3] = { 0, 0, 0 };

    for (int br0 = 0; br0 < bh; br0 += BATCH_BR) {
        int nrows = (bh - br0 < BATCH_BR) ? (bh - br0) : BATCH_BR;
        long total = (long)nrows * bw;

        /* paralelně: výpočet a DCT všech bloků v dávce */
        #pragma omp parallel for schedule(dynamic, 16)
        for (long n = 0; n < total; n++) {
            int by = br0 + (int)(n / bw);
            int bx = (int)(n % bw);
            compute_block(bx, by, W, H, coef[n]);
        }

        /* sekvenčně: Huffmanovo kódování (závisí na předchozích blocích) */
        for (long n = 0; n < total; n++)
            for (int c = 0; c < 3; c++)
                encode_block(&bwr, coef[n][c], &prev_dc[c]);

        printf("Pracuji... %.1f %%\r", 100.0 * (br0 + nrows) / bh);
        fflush(stdout);
    }

    /* doplnění posledního bajtu jedničkovými bity a EOI */
    if (bwr.nbits > 0) bw_put(&bwr, (1u << (8 - bwr.nbits)) - 1, 8 - bwr.nbits);
    bw_flush(&bwr);
    put16(f, 0xFFD9);

    free(coef);
    free(bwr.buf);
    fclose(f);

    printf("\n100.0 %% - Hotovo! Soubor je připraven k otevření.\n");
    return 0;
}

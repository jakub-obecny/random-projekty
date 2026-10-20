/mandelbrot (BMP)/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* --- KONFIGURACE --- */
#define WIDTH    23000
#define HEIGHT   23000
#define MAX_ITER 100
#define FILENAME "mandelbrot.bmp"
#define BATCH    64   /* kolik řádků se počítá paralelně najednou */

static const double X_MIN = -2.0, X_MAX = 0.5;
static const double Y_MIN = -1.25, Y_MAX = 1.25;

static void w16(FILE *f, uint16_t v) {
    uint8_t b[2] = { v & 0xFF, v >> 8 };
    fwrite(b, 1, 2, f);
}

static void w32(FILE *f, uint32_t v) {
    uint8_t b[4] = { v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, v >> 24 };
    fwrite(b, 1, 4, f);
}

static void write_bmp_header(FILE *f, uint32_t w, uint32_t h, uint32_t row_size) {
    uint32_t data_size = row_size * h;

    fwrite("BM", 1, 2, f);
    w32(f, 54 + data_size);   /* velikost souboru */
    w32(f, 0);                /* rezervováno */
    w32(f, 54);               /* offset k datům */

    w32(f, 40);               /* velikost DIB hlavičky */
    w32(f, w);                /* šířka */
    w32(f, h);                /* výška (kladná = odspodu nahoru) */
    w16(f, 1);                /* roviny */
    w16(f, 24);               /* bitů na pixel */
    w32(f, 0);                /* komprese */
    w32(f, data_size);        /* velikost dat */
    w32(f, 2835);             /* horizontální rozlišení */
    w32(f, 2835);             /* vertikální rozlišení */
    w32(f, 0);                /* barvy v paletě */
    w32(f, 0);                /* důležité barvy */
}

static inline int min255(int v) { return v > 255 ? 255 : v; }

int main(void) {
    /* každý řádek v BMP musí být násobek 4 bajtů */
    const size_t row_size = ((size_t)WIDTH * 3 + 3) & ~(size_t)3;

    printf("Vytvářím soubor: %s\n", FILENAME);

    FILE *f = fopen(FILENAME, "wb");
    if (!f) { perror("fopen"); return 1; }

    uint8_t *buf = calloc((size_t)BATCH, row_size);
    if (!buf) { fprintf(stderr, "Nedostatek paměti\n"); fclose(f); return 1; }

    write_bmp_header(f, WIDTH, HEIGHT, (uint32_t)row_size);

    for (long k0 = 0; k0 < HEIGHT; k0 += BATCH) {
        long n = (HEIGHT - k0 < BATCH) ? (HEIGHT - k0) : BATCH;

        #pragma omp parallel for schedule(dynamic)
        for (long i = 0; i < n; i++) {
            long y = HEIGHT - 1 - (k0 + i);   /* odspodu nahoru */
            double im = Y_MIN + ((double)y / HEIGHT) * (Y_MAX - Y_MIN);
            uint8_t *p = buf + (size_t)i * row_size;

            for (long x = 0; x < WIDTH; x++) {
                double re = X_MIN + ((double)x / WIDTH) * (X_MAX - X_MIN);
                double zr = 0.0, zi = 0.0;
                int it = 0;
                while (zr * zr + zi * zi <= 4.0 && it < MAX_ITER) {
                    double nr = zr * zr - zi * zi + re;
                    double ni = 2.0 * zr * zi + im;
                    zr = nr; zi = ni;
                    it++;
                }

                if (it == MAX_ITER) {
                    p[0] = p[1] = p[2] = 0;
                } else {
                    p[0] = (uint8_t)min255(it * 8);  /* B */
                    p[1] = (uint8_t)min255(it * 3);  /* G */
                    p[2] = (uint8_t)min255(it * 2);  /* R */
                }
                p += 3;
            }
            /* padding na konci řádku zůstává 0 (calloc) */
        }

        if (fwrite(buf, row_size, (size_t)n, f) != (size_t)n) {
            perror("fwrite");
            free(buf); fclose(f);
            return 1;
        }

        printf("Pracuji... %.1f %%\r", 100.0 * (k0 + n) / HEIGHT);
        fflush(stdout);
    }

    free(buf);
    fclose(f);
    printf("\n100.0 %% - Hotovo! Soubor je připraven k otevření.\n");
    return 0;
}

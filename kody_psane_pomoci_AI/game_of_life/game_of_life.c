/*Conway's Game Of Life*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define DENSITY 0.4
#define DOSAVE 1
#define FONT_CHARS "0123456789POLEGNZIVMSX:/ "

typedef struct {
    int w;
    int h;
    uint8_t *a;
    uint8_t *b;
    int *xl;
    int *xr;
    int *yu;
    int *yd;
} Grid;

static const unsigned char gl[175] = {
    /* 0 */ 14,17,19,21,25,17,14,
    /* 1 */ 4,12,4,4,4,4,14,
    /* 2 */ 14,17,1,2,4,8,31,
    /* 3 */ 31,2,4,2,1,17,14,
    /* 4 */ 2,6,10,18,31,2,2,
    /* 5 */ 31,16,30,1,1,17,14,
    /* 6 */ 6,8,16,30,17,17,14,
    /* 7 */ 31,1,2,4,8,8,8,
    /* 8 */ 14,17,17,14,17,17,14,
    /* 9 */ 14,17,17,15,1,2,12,
    /* P */ 30,17,17,30,16,16,16,
    /* O */ 14,17,17,17,17,17,14,
    /* L */ 16,16,16,16,16,16,31,
    /* E */ 31,16,16,30,16,16,31,
    /* G */ 14,17,16,23,17,17,15,
    /* N */ 17,17,25,21,19,17,17,
    /* Z */ 31,1,2,4,8,16,31,
    /* I */ 14,4,4,4,4,4,14,
    /* V */ 17,17,17,17,17,10,4,
    /* M */ 17,27,21,21,17,17,17,
    /* S */ 15,16,16,14,1,1,30,
    /* X */ 17,17,10,4,10,17,17,
    /* : */ 0,4,4,0,4,4,0,
    /* / */ 1,1,2,4,8,16,16,
    /* space */ 0,0,0,0,0,0,0
};

static void write_u16(FILE *f, uint16_t v) {
    fputc((v & 0xFF), f);
    fputc((v >> 8) & 0xFF, f);
}

static void write_u32(FILE *f, uint32_t v) {
    fputc(v & 0xFF, f);
    fputc((v >> 8) & 0xFF, f);
    fputc((v >> 16) & 0xFF, f);
    fputc((v >> 24) & 0xFF, f);
}

static int glyph_index_for_char(char ch) {
    char *p = strchr(FONT_CHARS, ch);
    if (!p) return 24;
    return (int)(p - FONT_CHARS);
}

static int read_int(const char *prompt, int min_value) {
    char buffer[128];
    char *end = NULL;
    long value = 0;

    while (1) {
        printf("%s", prompt);
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            fprintf(stderr, "Chyba cteni vstupu.\n");
            exit(1);
        }

        value = strtol(buffer, &end, 10);
        if (end == buffer || (*end != '\n' && *end != '\r' && *end != '\0')) {
            printf("Neplatny vstup. Zadej cele cislo.\n");
            continue;
        }

        if (value < min_value) {
            printf("Hodnota musi byt >= %d.\n", min_value);
            continue;
        }

        return (int)value;
    }
}

static void init_grid(Grid *g, int w, int h) {
    g->w = w;
    g->h = h;
    g->a = (uint8_t *)calloc((size_t)w * h, sizeof(uint8_t));
    g->b = (uint8_t *)calloc((size_t)w * h, sizeof(uint8_t));
    g->xl = (int *)malloc((size_t)w * sizeof(int));
    g->xr = (int *)malloc((size_t)w * sizeof(int));
    g->yu = (int *)malloc((size_t)h * sizeof(int));
    g->yd = (int *)malloc((size_t)h * sizeof(int));

    if (!g->a || !g->b || !g->xl || !g->xr || !g->yu || !g->yd) {
        fprintf(stderr, "Nedostatek pameti.\n");
        exit(1);
    }

    for (int x = 0; x < w; ++x) {
        g->xl[x] = (x - 1 + w) % w;
        g->xr[x] = (x + 1) % w;
    }

    for (int y = 0; y < h; ++y) {
        g->yu[y] = (y - 1 + h) % h;
        g->yd[y] = (y + 1) % h;
    }
}

static void free_grid(Grid *g) {
    free(g->a);
    free(g->b);
    free(g->xl);
    free(g->xr);
    free(g->yu);
    free(g->yd);
}

static int count_alive(const Grid *g) {
    int alive = 0;
    for (int i = 0; i < g->w * g->h; ++i) {
        alive += g->a[i];
    }
    return alive;
}

static void random_fill(Grid *g) {
    for (int y = 0; y < g->h; ++y) {
        for (int x = 0; x < g->w; ++x) {
            g->a[y * g->w + x] = (rand() / (double)RAND_MAX < DENSITY) ? 1 : 0;
            g->b[y * g->w + x] = 0;
        }
    }
}

static void step(Grid *g) {
    int w = g->w;
    int h = g->h;

    for (int y = 0; y < h; ++y) {
        int u = g->yu[y];
        int d = g->yd[y];
        for (int x = 0; x < w; ++x) {
            int l = g->xl[x];
            int r = g->xr[x];

            int nb =
                g->a[(l + u * w)] +
                g->a[(x + u * w)] +
                g->a[(r + u * w)] +
                g->a[(l + y * w)] +
                g->a[(r + y * w)] +
                g->a[(l + d * w)] +
                g->a[(x + d * w)] +
                g->a[(r + d * w)];

            int alive = g->a[y * w + x];
            int next = (alive) ? ((nb == 2 || nb == 3) ? 1 : 0)
                              : ((nb == 3) ? 1 : 0);

            g->b[y * w + x] = next;
        }
    }

    uint8_t *tmp = g->a;
    g->a = g->b;
    g->b = tmp;
}

static void set_bitmap_pixel(uint8_t *img, int row_stride, int width, int x, int y, int value) {
    if (x < 0 || x >= width || y < 0) return;
    size_t byte_index = (size_t)y * row_stride + (size_t)(x / 8);
    unsigned bit = 7u - (unsigned)(x % 8u);

    if (value) {
        img[byte_index] |= (uint8_t)(1u << bit);
    } else {
        img[byte_index] &= (uint8_t)~(1u << bit);
    }
}

static void draw_text_to_bitmap(uint8_t *img, int row_stride, int width, int height,
                               int x0, int y0, const char *txt) {
    int x = x0;
    int y = y0;

    for (const char *p = txt; *p; ++p) {
        int idx = glyph_index_for_char(*p);

        for (int row = 0; row < 7; ++row) {
            int pattern = gl[idx * 7 + row];
            for (int col = 0; col < 5; ++col) {
                if (pattern & (1 << (4 - col))) {
                    for (int yy = 0; yy < 3; ++yy) {
                        for (int xx = 0; xx < 3; ++xx) {
                            set_bitmap_pixel(img, row_stride, width,
                                             x + col * 3 + xx,
                                             y + row * 3 + yy,
                                             1);
                        }
                    }
                }
            }
        }

        x += 18;
    }
}

static void save_bmp(const char *filename, const Grid *g, int gen, const char *txt) {
    int cell_size = 8;
    int img_w = g->w * cell_size;
    int img_h = g->h * cell_size + 32;

    int row_stride = ((img_w + 31) / 32) * 4;
    size_t img_size = (size_t)row_stride * (size_t)img_h;
    uint8_t *pixels = (uint8_t *)calloc(1, img_size);

    if (!pixels) {
        fprintf(stderr, "Nelze alokovat pamet pro BMP.\n");
        exit(1);
    }

    for (int y = 0; y < g->h; ++y) {
        for (int x = 0; x < g->w; ++x) {
            if (g->a[y * g->w + x] == 0) continue;
            for (int yy = 0; yy < cell_size; ++yy) {
                for (int xx = 0; xx < cell_size; ++xx) {
                    set_bitmap_pixel(pixels, row_stride, img_w,
                                     x * cell_size + xx,
                                     y * cell_size + yy,
                                     1);
                }
            }
        }
    }

    draw_text_to_bitmap(pixels, row_stride, img_w, img_h, 8, img_h - 24, txt);

    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen");
        free(pixels);
        exit(1);
    }

    write_u16(f, 0x4D42);
    write_u32(f, 14u + 40u + 8u + (uint32_t)img_size);
    write_u16(f, 0);
    write_u16(f, 0);
    write_u32(f, 14u + 40u + 8u);

    write_u32(f, 40);
    write_u32(f, (uint32_t)img_w);
    write_u32(f, (uint32_t)img_h);
    write_u16(f, 1);
    write_u16(f, 1);
    write_u32(f, 0);
    write_u32(f, (uint32_t)img_size);
    write_u32(f, 2835);
    write_u32(f, 2835);
    write_u32(f, 2);
    write_u32(f, 2);

    write_u32(f, 0);
    write_u32(f, 0x00FFFFFF);

    for (int y = img_h - 1; y >= 0; --y) {
        size_t row_offset = (size_t)y * row_stride;
        fwrite(pixels + row_offset, 1, (size_t)row_stride, f);
    }

    fclose(f);
    free(pixels);
}

static void print_ascii(const Grid *g) {
    for (int y = 0; y < g->h; ++y) {
        for (int x = 0; x < g->w; ++x) {
            putchar(g->a[y * g->w + x] ? '#' : ' ');
        }
        putchar('\n');
    }
    putchar('\n');
}

int main(void) {
    srand((unsigned)time(NULL));

    int generations = read_int("Kolik chces generaci? ", 1);
    int w = read_int("Sirka pole: ", 1);
    int h = read_int("Vyska pole: ", 1);

    Grid g;
    init_grid(&g, w, h);
    random_fill(&g);

    if (DOSAVE) {
        char txt[128];
        snprintf(txt, sizeof(txt),
                 "POLE %dx%d GEN: 0 ZIVE: %d 0 MS/GEN",
                 g.w, g.h, count_alive(&g));
        save_bmp("gen(0).bmp", &g, 0, txt);
    }

    for (int gen = 1; gen <= generations; ++gen) {
        step(&g);

        int alive = count_alive(&g);

        if (DOSAVE) {
            char txt[128];
            char file_name[64];
            snprintf(file_name, sizeof(file_name), "gen(%d).bmp", gen);
            snprintf(txt, sizeof(txt),
                     "POLE %dx%d GEN: %d ZIVE: %d %d MS/GEN",
                     g.w, g.h, gen, alive, 0);
            save_bmp(file_name, &g, gen, txt);
        }

        printf("GEN %d: ZIVE %d\n", gen, alive);

        if (gen % 25 == 0 && (g.w * g.h <= 2500)) {
            print_ascii(&g);
        }
    }

    printf("Hotovo. Vytvoreno %d snapshotu BMP.\\n", generations + 1);
    free_grid(&g);
    return 0;
}

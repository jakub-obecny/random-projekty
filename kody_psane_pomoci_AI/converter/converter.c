#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT 1024

enum {
    TYPE_BIN = 1,
    TYPE_HEX = 2,
    TYPE_DEC = 3,
    TYPE_ASCII = 4
};

static void flush_stdin(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

static void remove_separators(char *dst, const char *src) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0'; ++i) {
        char c = src[i];
        if (c == ' ' || c == '_' || c == ',' || c == '\t') {
            continue;
        }
        dst[j++] = c;
    }
    dst[j] = '\0';
}

static int parse_binary_value(const char *src, int *out) {
    char cleaned[MAX_INPUT];
    remove_separators(cleaned, src);

    if (cleaned[0] == '\0') return 0;

    long value = 0;
    for (size_t i = 0; cleaned[i] != '\0'; ++i) {
        char c = cleaned[i];
        if (c != '0' && c != '1') return 0;
        value = value * 2 + (c - '0');
    }

    if (value < INT_MIN || value > INT_MAX) return 0;
    *out = (int)value;
    return 1;
}

static int parse_hex_value(const char *src, int *out) {
    char cleaned[MAX_INPUT];
    remove_separators(cleaned, src);

    if (cleaned[0] == '\0') return 0;

    if (cleaned[0] == '0' && (cleaned[1] == 'x' || cleaned[1] == 'X')) {
        memmove(cleaned, cleaned + 2, strlen(cleaned + 2) + 1);
    }

    long value = 0;
    for (size_t i = 0; cleaned[i] != '\0'; ++i) {
        char c = cleaned[i];
        int digit = 0;

        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else return 0;

        value = value * 16 + digit;
    }

    if (value < INT_MIN || value > INT_MAX) return 0;
    *out = (int)value;
    return 1;
}

static int parse_decimal_value(const char *src, int *out) {
    char cleaned[MAX_INPUT];
    remove_separators(cleaned, src);

    if (cleaned[0] == '\0') return 0;

    long value = 0;
    size_t i = 0;
    int negative = 0;

    if (cleaned[0] == '-') {
        negative = 1;
        i = 1;
    }

    if (cleaned[i] == '\0') return 0;

    for (; cleaned[i] != '\0'; ++i) {
        char c = cleaned[i];
        if (c < '0' || c > '9') return 0;
        value = value * 10 + (c - '0');
    }

    if (negative) value = -value;

    if (value < INT_MIN || value > INT_MAX) return 0;
    *out = (int)value;
    return 1;
}

static void to_bin8(int value, char *buffer, size_t size) {
    unsigned int u = (unsigned int)(value & 0xFF);
    for (int i = 7; i >= 0; --i) {
        buffer[7 - i] = (char)('0' + ((u >> i) & 1));
    }
    buffer[8] = '\0';
}

static void to_hex2(int value, char *buffer, size_t size) {
    const char *digits = "0123456789ABCDEF";
    unsigned int u = (unsigned int)(value & 0xFF);
    snprintf(buffer, size, "%c%c", digits[(u >> 4) & 0xF], digits[u & 0xF]);
}

static int binary_to_ascii(const char *input, char *output, size_t out_size) {
    char cleaned[MAX_INPUT];
    remove_separators(cleaned, input);

    if (strlen(cleaned) == 0) {
        snprintf(output, out_size, "ERROR: INVALID INPUT");
        return 0;
    }

    if (strlen(cleaned) % 8 != 0) {
        snprintf(output, out_size, "ERROR: USE 8 BIT GROUPS");
        return 0;
    }

    size_t written = 0;
    for (size_t i = 0; i < strlen(cleaned); i += 8) {
        char chunk[9];
        memcpy(chunk, cleaned + i, 8);
        chunk[8] = '\0';

        int value = 0;
        for (int bit = 0; bit < 8; ++bit) {
            if (chunk[bit] != '0' && chunk[bit] != '1') {
                snprintf(output, out_size, "ERROR: INVALID BINARY BYTE");
                return 0;
            }
            value = (value << 1) | (chunk[bit] - '0');
        }

        if (value < 0 || value > 255) {
            snprintf(output, out_size, "ERROR: INVALID BYTE");
            return 0;
        }

        output[written++] = (char)value;
        if (written + 1 >= out_size) {
            output[written] = '\0';
            return 1;
        }
    }

    output[written] = '\0';
    return 1;
}

static int hex_to_ascii(const char *input, char *output, size_t out_size) {
    char cleaned[MAX_INPUT];
    remove_separators(cleaned, input);

    if (cleaned[0] == '\0') {
        snprintf(output, out_size, "ERROR: INVALID INPUT");
        return 0;
    }

    if (strlen(cleaned) % 2 != 0) {
        snprintf(output, out_size, "ERROR: USE 2 DIGIT GROUPS");
        return 0;
    }

    size_t written = 0;
    for (size_t i = 0; i < strlen(cleaned); i += 2) {
        char pair[3];
        pair[0] = cleaned[i];
        pair[1] = cleaned[i + 1];
        pair[2] = '\0';

        int value = 0;
        if (!parse_hex_value(pair, &value)) {
            snprintf(output, out_size, "ERROR: INVALID HEX BYTE");
            return 0;
        }

        if (value < 0 || value > 255) {
            snprintf(output, out_size, "ERROR: INVALID BYTE");
            return 0;
        }

        output[written++] = (char)value;
        if (written + 1 >= out_size) {
            output[written] = '\0';
            return 1;
        }
    }

    output[written] = '\0';
    return 1;
}

static int decimal_to_ascii(const char *input, char *output, size_t out_size) {
    char cleaned[MAX_INPUT];
    remove_separators(cleaned, input);

    size_t written = 0;
    char *token = strtok(cleaned, " \t,");
    while (token != NULL) {
        int value = 0;
        if (!parse_decimal_value(token, &value)) {
            snprintf(output, out_size, "ERROR: VALUES 0-255");
            return 0;
        }

        if (value < 0 || value > 255) {
            snprintf(output, out_size, "ERROR: VALUES 0-255");
            return 0;
        }

        output[written++] = (char)value;
        if (written + 1 >= out_size) {
            output[written] = '\0';
            return 1;
        }

        token = strtok(NULL, " \t,");
    }

    if (written == 0) {
        snprintf(output, out_size, "ERROR: INVALID INPUT");
        return 0;
    }

    output[written] = '\0';
    return 1;
}

static void append_value(char *output, size_t out_size, size_t *len, const char *part) {
    size_t part_len = strlen(part);
    if (*len + part_len + 1 >= out_size) {
        return;
    }

    memcpy(output + *len, part, part_len);
    *len += part_len;

    if (*len + 1 < out_size) {
        output[(*len)++] = ' ';
    }

    output[*len] = '\0';
}

static int convert_text(const char *input, int in_type, int out_type, char *output, size_t out_size) {
    output[0] = '\0';

    if (out_type == TYPE_ASCII) {
        if (in_type == TYPE_BIN) {
            return binary_to_ascii(input, output, out_size);
        }
        if (in_type == TYPE_HEX) {
            return hex_to_ascii(input, output, out_size);
        }
        if (in_type == TYPE_DEC) {
            return decimal_to_ascii(input, output, out_size);
        }
        snprintf(output, out_size, "ERROR: INVALID INPUT TYPE");
        return 0;
    }

    if (in_type == TYPE_ASCII) {
        size_t len = 0;
        for (size_t i = 0; input[i] != '\0'; ++i) {
            unsigned char ch = (unsigned char)input[i];
            char tmp[64];

            if (out_type == TYPE_BIN) {
                to_bin8((int)ch, tmp, sizeof(tmp));
            } else if (out_type == TYPE_HEX) {
                to_hex2((int)ch, tmp, sizeof(tmp));
            } else if (out_type == TYPE_DEC) {
                snprintf(tmp, sizeof(tmp), "%d", (int)ch);
            } else {
                snprintf(output, out_size, "ERROR: INVALID OUTPUT TYPE");
                return 0;
            }

            append_value(output, out_size, &len, tmp);
        }

        if (len > 0) {
            if (len > 0 && output[len - 1] == ' ') {
                output[len - 1] = '\0';
            }
        }

        if (output[0] == '\0') {
            snprintf(output, out_size, "ERROR: INVALID INPUT");
            return 0;
        }

        return 1;
    }

    int value = 0;

    if (in_type == TYPE_BIN) {
        if (!parse_binary_value(input, &value)) {
            snprintf(output, out_size, "ERROR: INVALID INPUT");
            return 0;
        }
    } else if (in_type == TYPE_HEX) {
        if (!parse_hex_value(input, &value)) {
            snprintf(output, out_size, "ERROR: INVALID INPUT");
            return 0;
        }
    } else if (in_type == TYPE_DEC) {
        if (!parse_decimal_value(input, &value)) {
            snprintf(output, out_size, "ERROR: INVALID INPUT");
            return 0;
        }
    } else {
        snprintf(output, out_size, "ERROR: INVALID INPUT TYPE");
        return 0;
    }

    if (out_type == TYPE_BIN) {
        char tmp[64];
        to_bin8(value, tmp, sizeof(tmp));
        snprintf(output, out_size, "%s", tmp);
        return 1;
    }

    if (out_type == TYPE_HEX) {
        char tmp[16];
        to_hex2(value, tmp, sizeof(tmp));
        snprintf(output, out_size, "%s", tmp);
        return 1;
    }

    if (out_type == TYPE_DEC) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "%d", value);
        snprintf(output, out_size, "%s", tmp);
        return 1;
    }

    snprintf(output, out_size, "ERROR: INVALID OUTPUT TYPE");
    return 0;
}

static void read_line_input(char *buffer, size_t size) {
    size_t len = 0;
    int ch;

    printf("> ");
    fflush(stdout);

    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (ch == 8 || ch == 127) {
            if (len > 0) {
                len--;
                printf("\b \b");
            }
            continue;
        }

        if (ch < 32) continue;

        if (len + 1 >= size) continue;

        buffer[len++] = (char)ch;
        putchar(ch);
        fflush(stdout);
    }

    buffer[len] = '\0';
    printf("\n");
}

static void show_menu(void) {
    printf("\n==============================\n");
    printf("      BASE CONVERTER\n");
    printf("==============================\n\n");
    printf("1 BIN\n");
    printf("2 HEX\n");
    printf("3 DEC\n");
    printf("4 ASCII\n\n");
}

static int read_choice(const char *prompt) {
    int choice = 0;
    printf("%s", prompt);
    if (scanf("%d", &choice) != 1) {
        flush_stdin();
        return 0;
    }
    flush_stdin();
    return choice;
}

int main(void) {
    char input[MAX_INPUT];
    char output[MAX_INPUT];

    while (1) {
        show_menu();

        int im = read_choice("INPUT TYPE: ");
        if (im < 1 || im > 4) {
            printf("INVALID CHOICE\n");
            continue;
        }

        printf("\nENTER INPUT\n");
        printf("TAB or ENTER = DONE\n");
        printf("BACKSPACE = DELETE\n\n");

        read_line_input(input, sizeof(input));

        if (input[0] == '\0') {
            continue;
        }

        printf("\nINPUT: %s\n\n", input);

        printf("OUTPUT TYPE:\n");
        printf("1 BIN\n");
        printf("2 HEX\n");
        printf("3 DEC\n");
        printf("4 ASCII\n\n");

        int om = read_choice("OUTPUT: ");
        if (om < 1 || om > 4) {
            printf("INVALID CHOICE\n");
            continue;
        }

        if (!convert_text(input, im, om, output, sizeof(output))) {
            printf("\nERROR\n");
            printf("%s\n", output);
        } else {
            printf("\nRESULT\n");
            printf("==============================\n");
            printf("%s\n", output);
            printf("==============================\n");
        }

        printf("\nSAVE TO FILE? Y/N: ");
        char answer[8];
        if (scanf("%7s", answer) != 1) {
            flush_stdin();
            answer[0] = 'N';
            answer[1] = '\0';
        }
        flush_stdin();

        if (toupper((unsigned char)answer[0]) == 'Y') {
            char filename[MAX_INPUT];
            printf("FILENAME: ");
            if (scanf("%1023s", filename) != 1) {
                flush_stdin();
                printf("Invalid filename.\n");
                continue;
            }
            flush_stdin();

            FILE *f = fopen(filename, "w");
            if (!f) {
                printf("Could not open file: %s\n", filename);
                continue;
            }

            fprintf(f, "Converter\n");
            fprintf(f, "Input: %s\n", input);
            fprintf(f, "Result:\n");
            fprintf(f, "%s\n", output);
            fclose(f);

            printf("SAVED: %s\n", filename);
        }

        printf("\nPRESS ANY KEY\n");
        int ch = getchar();
        if (ch == EOF) break;
        flush_stdin();
    }

    return 0;
}

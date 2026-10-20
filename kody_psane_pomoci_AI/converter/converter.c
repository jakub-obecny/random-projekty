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

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
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

static int parse_binary(const char *src, int *out) {
    char cleaned[MAX_INPUT];
    remove_separators(cleaned, src);

    if (cleaned[0] == '\0') {
        return 0;
    }

    long value = 0;
    for (size_t i = 0; cleaned[i] != '\0'; ++i) {
        char c = cleaned[i];
        if (c != '0' && c != '1') {
            return 0;
        }
        value = value * 2 + (c - '0');
    }

    if (value < INT_MIN || value > INT_MAX) {
        return 0;
    }

    *out = (int)value;
    return 1;
}

static int parse_hex(const char *src, int *out) {
    char cleaned[MAX_INPUT];
    remove_separators(cleaned, src);

    if (cleaned[0] == '\0') {
        return 0;
    }

    if (cleaned[0] == '0' && (cleaned[1] == 'x' || cleaned[1] == 'X')) {
        memmove(cleaned, cleaned + 2, strlen(cleaned + 2) + 1);
    }

    long value = 0;
    for (size_t i = 0; cleaned[i] != '\0'; ++i) {
        char c = cleaned[i];
        int digit = 0;

        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            return 0;
        }

        value = value * 16 + digit;
    }

    if (value < INT_MIN || value > INT_MAX) {
        return 0;
    }

    *out = (int)value;
    return 1;
}

static int parse_decimal(const char *src, int *out) {
    char cleaned[MAX_INPUT];
    remove_separators(cleaned, src);

    if (cleaned[0] == '\0') {
        return 0;
    }

    long value = 0;
    size_t i = 0;
    int negative = 0;

    if (cleaned[0] == '-') {
        negative = 1;
        i = 1;
    }

    if (cleaned[i] == '\0') {
        return 0;
    }

    for (; cleaned[i] != '\0'; ++i) {
        char c = cleaned[i];
        if (c < '0' || c > '9') {
            return 0;
        }
        value = value * 10 + (c - '0');
    }

    if (negative) {
        value = -value;
    }

    if (value < INT_MIN || value > INT_MAX) {
        return 0;
    }

    *out = (int)value;
    return 1;
}

static void to_binary_string(int value, char *buffer, size_t size) {
    if (value == 0) {
        snprintf(buffer, size, "0");
        return;
    }

    char tmp[64];
    size_t len = 0;
    unsigned int u = (unsigned int)value;

    while (u > 0) {
        tmp[len++] = (char)('0' + (u & 1));
        u >>= 1;
    }

    for (size_t i = 0; i < len; ++i) {
        buffer[i] = tmp[len - 1 - i];
    }
    buffer[len] = '\0';
}

static void to_hex_string(int value, char *buffer, size_t size) {
    const char *digits = "0123456789ABCDEF";
    if (value == 0) {
        snprintf(buffer, size, "0");
        return;
    }

    char tmp[64];
    size_t len = 0;
    unsigned int u = (unsigned int)value;

    while (u > 0) {
        tmp[len++] = digits[u & 0xF];
        u >>= 4;
    }

    for (size_t i = 0; i < len; ++i) {
        buffer[i] = tmp[len - 1 - i];
    }
    buffer[len] = '\0';
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

static int ascii_to_other(const char *input, int out_type, char *output, size_t output_size) {
    if (out_type == TYPE_ASCII) {
        snprintf(output, output_size, "%s", input);
        return 1;
    }

    size_t written = 0;
    for (size_t i = 0; input[i] != '\0'; ++i) {
        unsigned char ch = (unsigned char)input[i];

        if (out_type == TYPE_BIN) {
            char tmp[16];
            to_bin8((int)ch, tmp, sizeof(tmp));
            int n = snprintf(output + written, output_size - written, "%s%s",
                             written == 0 ? "" : " ", tmp);
            if (n < 0 || (size_t)n >= output_size - written) {
                return 0;
            }
            written += (size_t)n;
        } else if (out_type == TYPE_HEX) {
            char tmp[16];
            to_hex2((int)ch, tmp, sizeof(tmp));
            int n = snprintf(output + written, output_size - written, "%s%s",
                             written == 0 ? "" : " ", tmp);
            if (n < 0 || (size_t)n >= output_size - written) {
                return 0;
            }
            written += (size_t)n;
        } else if (out_type == TYPE_DEC) {
            char tmp[16];
            snprintf(tmp, sizeof(tmp), "%u", (unsigned int)ch);
            int n = snprintf(output + written, output_size - written, "%s%s",
                             written == 0 ? "" : " ", tmp);
            if (n < 0 || (size_t)n >= output_size - written) {
                return 0;
            }
            written += (size_t)n;
        } else {
            return 0;
        }
    }

    return 1;
}

static int other_to_ascii(const char *input, int in_type, char *output, size_t output_size) {
    int value = 0;

    if (in_type == TYPE_BIN) {
        if (!parse_binary(input, &value)) {
            snprintf(output, output_size, "ERROR: INVALID INPUT");
            return 0;
        }
    } else if (in_type == TYPE_HEX) {
        if (!parse_hex(input, &value)) {
            snprintf(output, output_size, "ERROR: INVALID INPUT");
            return 0;
        }
    } else if (in_type == TYPE_DEC) {
        if (!parse_decimal(input, &value)) {
            snprintf(output, output_size, "ERROR: INVALID INPUT");
            return 0;
        }
    } else {
        snprintf(output, output_size, "ERROR: INVALID INPUT TYPE");
        return 0;
    }

    if (value < 0 || value > 255) {
        snprintf(output, output_size, "ERROR: VALUES 0-255");
        return 0;
    }

    output[0] = (char)value;
    output[1] = '\0';
    return 1;
}

static int convert_text(const char *input, int in_type, int out_type, char *output, size_t output_size) {
    output[0] = '\0';

    if (in_type == TYPE_ASCII) {
        return ascii_to_other(input, out_type, output, output_size);
    }

    if (out_type == TYPE_ASCII) {
        return other_to_ascii(input, in_type, output, output_size);
    }

    int value = 0;

    if (in_type == TYPE_BIN) {
        if (!parse_binary(input, &value)) {
            snprintf(output, output_size, "ERROR: INVALID INPUT");
            return 0;
        }
    } else if (in_type == TYPE_HEX) {
        if (!parse_hex(input, &value)) {
            snprintf(output, output_size, "ERROR: INVALID INPUT");
            return 0;
        }
    } else if (in_type == TYPE_DEC) {
        if (!parse_decimal(input, &value)) {
            snprintf(output, output_size, "ERROR: INVALID INPUT");
            return 0;
        }
    } else {
        snprintf(output, output_size, "ERROR: INVALID INPUT TYPE");
        return 0;
    }

    if (out_type == TYPE_BIN) {
        char tmp[64];
        to_binary_string(value, tmp, sizeof(tmp));
        snprintf(output, output_size, "%s", tmp);
        return 1;
    }

    if (out_type == TYPE_HEX) {
        char tmp[64];
        to_hex_string(value, tmp, sizeof(tmp));
        snprintf(output, output_size, "%s", tmp);
        return 1;
    }

    if (out_type == TYPE_DEC) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "%d", value);
        snprintf(output, output_size, "%s", tmp);
        return 1;
    }

    snprintf(output, output_size, "ERROR: INVALID OUTPUT TYPE");
    return 0;
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

static void read_line_input(char *buffer, size_t size) {
    size_t len = 0;

    printf("> ");
    fflush(stdout);

    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (ch == 8 || ch == 127) {
            if (len > 0) {
                len--;
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }

        if (ch < 32) {
            continue;
        }

        if (len + 1 >= size) {
            continue;
        }

        buffer[len++] = (char)ch;
        putchar(ch);
        fflush(stdout);
    }

    buffer[len] = '\0';
    printf("\n");
}

static void save_to_file(const char *input, const char *result) {
    char filename[MAX_INPUT];
    printf("FILENAME: ");
    if (scanf("%1023s", filename) != 1) {
        flush_stdin();
        printf("Invalid filename.\n");
        return;
    }
    flush_stdin();

    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Could not open file: %s\n", filename);
        return;
    }

    fprintf(f, "PicoMite Converter\n");
    fprintf(f, "Input: %s\n", input);
    fprintf(f, "Result:\n");
    fprintf(f, "%s\n", result);
    fclose(f);

    printf("SAVED: %s\n", filename);
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
        trim_newline(input);

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
            printf("\n%s\n", output);
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
            save_to_file(input, output);
        }

        printf("\nPRESS ANY KEY\n");
        int ch = getchar();
        if (ch == EOF) {
            break;
        }
        flush_stdin();
    }

    return 0;
}

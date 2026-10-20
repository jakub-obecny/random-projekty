
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 40
#define HEIGHT 15
#define GENERATIONS 25

typedef int Grid[HEIGHT][WIDTH];

void random_grid(Grid grid) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            grid[y][x] = rand() % 100 < 30;
        }
    }
}

int count_neighbors(Grid grid, int x, int y) {
    int count = 0;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0)
                continue;

            int nx = x + dx;
            int ny = y + dy;

            if (nx >= 0 && nx < WIDTH &&
                ny >= 0 && ny < HEIGHT) {
                count += grid[ny][nx];
            }
        }
    }

    return count;
}

void next_generation(Grid current, Grid next) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int neighbors = count_neighbors(current, x, y);

            if (current[y][x]) {
                next[y][x] = (neighbors == 2 || neighbors == 3);
            } else {
                next[y][x] = (neighbors == 3);
            }
        }
    }
}

void render(Grid grid, int generation, int seed_number) {
    printf("=== C GAME OF LIFE ===\n");
    printf("World #%d | Generation %d/%d\n\n",
           seed_number, generation, GENERATIONS);

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf("%s", grid[y][x] ? "##" : "  ");
        }
        printf("\n");
    }

    printf("\n");
}

int main(void) {
    srand((unsigned int)time(NULL));

    Grid current;
    Grid next;

    int seed_number = 1;

    while (1) {
        // Generate a completely new world.
        random_grid(current);

        for (int generation = 1;
             generation <= GENERATIONS;
             generation++) {

            render(current, generation, seed_number);

            next_generation(current, next);

            // Copy the next generation into the current grid.
            for (int y = 0; y < HEIGHT; y++) {
                for (int x = 0; x < WIDTH; x++) {
                    current[y][x] = next[y][x];
                }
            }
        }

        // Separator between worlds.
        
        // Separator between worlds.
        for (int i = 0; i < WIDTH * 2; i++) {
            printf("-");
        }

        printf("\nWorld #%d finished!\n", seed_number);
        printf("Press ENTER to generate the next world...");
        fflush(stdout);

        // Wait until ENTER is pressed.
        int ch;
        do {
            ch = getchar();
        } while (ch != '\n' && ch != EOF);

        if (ch == EOF) {
            break;
        }

        printf("\n");
        seed_number++;

        seed_number++;
    }

    return 0;
}

#include <string.h>
#include <stdlib.h>
#include "board.h"

/* Returns the cell at index j along line i, walking from the wall the tiles
   are sliding toward. This lets one piece of merge logic serve all 4 moves. */
static int *at(Board *b, Direction dir, int i, int j)
{
    switch (dir) {
    case DIR_LEFT:  return &b->cell[i][j];
    case DIR_RIGHT: return &b->cell[i][SIZE - 1 - j];
    case DIR_UP:    return &b->cell[j][i];
    default:        return &b->cell[SIZE - 1 - j][i];
    }
}

void board_reset(Board *b)
{
    memset(b, 0, sizeof(*b));
    board_spawn(b);
    board_spawn(b);
}

int board_move(Board *b, Direction dir)
{
    int moved = 0;
    int i, j;

    for (i = 0; i < SIZE; i++) {
        int line[SIZE];
        int out[SIZE] = {0};
        int n = 0, w = 0;

        /* collapse out the gaps */
        for (j = 0; j < SIZE; j++) {
            int v = *at(b, dir, i, j);
            if (v)
                line[n++] = v;
        }

        /* merge equal neighbours, left to right, each tile merging at most once */
        for (j = 0; j < n; j++) {
            if (j + 1 < n && line[j] == line[j + 1]) {
                int merged = line[j] * 2;
                out[w++] = merged;
                b->score += merged;
                if (merged == 2048)
                    b->won = 1;
                j++;
            } else {
                out[w++] = line[j];
            }
        }

        for (j = 0; j < SIZE; j++) {
            int *slot = at(b, dir, i, j);
            if (*slot != out[j]) {
                *slot = out[j];
                moved = 1;
            }
        }
    }

    return moved;
}

void board_spawn(Board *b)
{
    int empty[SIZE * SIZE];
    int n = 0, i, j, pick;

    for (i = 0; i < SIZE; i++)
        for (j = 0; j < SIZE; j++)
            if (b->cell[i][j] == 0)
                empty[n++] = i * SIZE + j;

    if (n == 0)
        return;

    pick = empty[rand() % n];
    /* 90% chance of a 2, matching the original game */
    b->cell[pick / SIZE][pick % SIZE] = (rand() % 10 == 0) ? 4 : 2;
}

int board_has_moves(const Board *b)
{
    int i, j;

    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            if (b->cell[i][j] == 0)
                return 1;
            if (j + 1 < SIZE && b->cell[i][j] == b->cell[i][j + 1])
                return 1;
            if (i + 1 < SIZE && b->cell[i][j] == b->cell[i + 1][j])
                return 1;
        }
    }

    return 0;
}

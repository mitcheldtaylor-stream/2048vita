#ifndef BOARD_H
#define BOARD_H

#define SIZE 4

typedef enum {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef struct {
    int cell[SIZE][SIZE];   /* 0 = empty, otherwise the tile value */
    int score;
    int won;                /* set once a 2048 tile has been made */
} Board;

void board_reset(Board *b);

/* Slides and merges in the given direction. Returns 1 if anything moved,
   which is also the signal that a new tile should be spawned. */
int  board_move(Board *b, Direction dir);

void board_spawn(Board *b);
int  board_has_moves(const Board *b);

#endif

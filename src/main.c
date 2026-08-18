#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/rtc.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <vita2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"

#define SCREEN_W 960
#define SCREEN_H 544

#define TILE     100
#define GAP      14
#define BOARD_PX (SIZE * TILE + (SIZE + 1) * GAP)
#define BOARD_X  70
#define BOARD_Y  ((SCREEN_H - BOARD_PX) / 2)

#define PANEL_X  580

#define SAVE_DIR  "ux0:data/2048"
#define SAVE_PATH "ux0:data/2048/best.bin"

#define RGBA(r, g, b) RGBA8((r), (g), (b), 255)

static const unsigned int COL_BG         = RGBA(0xFA, 0xF8, 0xEF);
static const unsigned int COL_BOARD      = RGBA(0xBB, 0xAD, 0xA0);
static const unsigned int COL_EMPTY      = RGBA(0xCD, 0xC1, 0xB4);
static const unsigned int COL_TEXT_DARK  = RGBA(0x77, 0x6E, 0x65);
static const unsigned int COL_TEXT_LIGHT = RGBA(0xF9, 0xF6, 0xF2);
static const unsigned int COL_GOLD       = RGBA(0xED, 0xC2, 0x2E);
static const unsigned int COL_OVERLAY    = RGBA8(0xEE, 0xE4, 0xDA, 200);

struct tile_style {
    int value;
    unsigned int bg;
    unsigned int fg;
};

static const struct tile_style TILE_STYLES[] = {
    {    2, RGBA(0xEE, 0xE4, 0xDA), RGBA(0x77, 0x6E, 0x65) },
    {    4, RGBA(0xED, 0xE0, 0xC8), RGBA(0x77, 0x6E, 0x65) },
    {    8, RGBA(0xF2, 0xB1, 0x79), RGBA(0xF9, 0xF6, 0xF2) },
    {   16, RGBA(0xF5, 0x95, 0x63), RGBA(0xF9, 0xF6, 0xF2) },
    {   32, RGBA(0xF6, 0x7C, 0x5F), RGBA(0xF9, 0xF6, 0xF2) },
    {   64, RGBA(0xF6, 0x5E, 0x3B), RGBA(0xF9, 0xF6, 0xF2) },
    {  128, RGBA(0xED, 0xCF, 0x72), RGBA(0xF9, 0xF6, 0xF2) },
    {  256, RGBA(0xED, 0xCC, 0x61), RGBA(0xF9, 0xF6, 0xF2) },
    {  512, RGBA(0xED, 0xC8, 0x50), RGBA(0xF9, 0xF6, 0xF2) },
    { 1024, RGBA(0xED, 0xC5, 0x3F), RGBA(0xF9, 0xF6, 0xF2) },
    { 2048, RGBA(0xED, 0xC2, 0x2E), RGBA(0xF9, 0xF6, 0xF2) },
};
#define NUM_STYLES ((int)(sizeof(TILE_STYLES) / sizeof(TILE_STYLES[0])))

static vita2d_pgf *pgf;

static void style_for(int value, unsigned int *bg, unsigned int *fg)
{
    int i;

    for (i = 0; i < NUM_STYLES; i++) {
        if (TILE_STYLES[i].value == value) {
            *bg = TILE_STYLES[i].bg;
            *fg = TILE_STYLES[i].fg;
            return;
        }
    }

    /* anything past 2048 keeps the darkest style */
    *bg = RGBA(0x3C, 0x3A, 0x32);
    *fg = COL_TEXT_LIGHT;
}

/* Draws text centred inside the box at (x, y, w, h). */
static void draw_centered(const char *text, int x, int y, int w, int h,
                          float scale, unsigned int color)
{
    int tw = vita2d_pgf_text_width(pgf, scale, text);
    int th = vita2d_pgf_text_height(pgf, scale, text);

    vita2d_pgf_draw_text(pgf, x + (w - tw) / 2, y + (h + th) / 2, color, scale, text);
}

static int load_best(void)
{
    int best = 0;
    SceUID fd = sceIoOpen(SAVE_PATH, SCE_O_RDONLY, 0777);

    if (fd < 0)
        return 0;

    if (sceIoRead(fd, &best, sizeof(best)) != sizeof(best) || best < 0)
        best = 0;

    sceIoClose(fd);
    return best;
}

static void save_best(int best)
{
    SceUID fd;

    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir(SAVE_DIR, 0777);

    fd = sceIoOpen(SAVE_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0)
        return;

    sceIoWrite(fd, &best, sizeof(best));
    sceIoClose(fd);
}

static void draw_score_box(const char *label, int value, int x, int y)
{
    char buf[32];

    vita2d_draw_rectangle(x, y, 150, 74, COL_BOARD);
    draw_centered(label, x, y + 6, 150, 26, 1.0f, COL_EMPTY);

    snprintf(buf, sizeof(buf), "%d", value);
    draw_centered(buf, x, y + 30, 150, 36, 1.3f, COL_TEXT_LIGHT);
}

static void draw_board(const Board *b, int best, int game_over)
{
    char buf[32];
    int i, j;

    vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, COL_BG);
    vita2d_draw_rectangle(BOARD_X, BOARD_Y, BOARD_PX, BOARD_PX, COL_BOARD);

    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            int x = BOARD_X + GAP + j * (TILE + GAP);
            int y = BOARD_Y + GAP + i * (TILE + GAP);
            int v = b->cell[i][j];
            unsigned int bg, fg;
            float scale;

            if (v == 0) {
                vita2d_draw_rectangle(x, y, TILE, TILE, COL_EMPTY);
                continue;
            }

            style_for(v, &bg, &fg);
            vita2d_draw_rectangle(x, y, TILE, TILE, bg);

            /* shrink the digits as the numbers get longer so they still fit */
            scale = (v < 100) ? 1.9f : (v < 1000) ? 1.5f : 1.15f;
            snprintf(buf, sizeof(buf), "%d", v);
            draw_centered(buf, x, y, TILE, TILE, scale, fg);
        }
    }

    vita2d_pgf_draw_text(pgf, PANEL_X, 90, COL_TEXT_DARK, 3.0f, "2048");

    draw_score_box("SCORE", b->score, PANEL_X, 120);
    draw_score_box("BEST", best, PANEL_X + 165, 120);

    vita2d_pgf_draw_text(pgf, PANEL_X, 250, COL_TEXT_DARK, 1.0f,
                         "D-Pad / Stick : Move");
    vita2d_pgf_draw_text(pgf, PANEL_X, 282, COL_TEXT_DARK, 1.0f,
                         "Triangle      : New game");
    vita2d_pgf_draw_text(pgf, PANEL_X, 314, COL_TEXT_DARK, 1.0f,
                         "Start         : Quit");

    if (b->won)
        vita2d_pgf_draw_text(pgf, PANEL_X, 380, COL_GOLD, 1.2f,
                             "2048 reached! Keep going.");

    if (game_over) {
        vita2d_draw_rectangle(BOARD_X, BOARD_Y, BOARD_PX, BOARD_PX, COL_OVERLAY);
        draw_centered("Game Over", BOARD_X, BOARD_Y, BOARD_PX, BOARD_PX - 60,
                      2.4f, COL_TEXT_DARK);
        draw_centered("Press Triangle to retry", BOARD_X, BOARD_Y + 80,
                      BOARD_PX, BOARD_PX, 1.1f, COL_TEXT_DARK);
    }
}

/* Turns the current pad state into a direction, or -1 for none.
   The analog stick is treated as a d-pad with a dead zone. */
static int read_direction(const SceCtrlData *pad)
{
    if ((pad->buttons & SCE_CTRL_UP)    || pad->ly < 60)  return DIR_UP;
    if ((pad->buttons & SCE_CTRL_DOWN)  || pad->ly > 195) return DIR_DOWN;
    if ((pad->buttons & SCE_CTRL_LEFT)  || pad->lx < 60)  return DIR_LEFT;
    if ((pad->buttons & SCE_CTRL_RIGHT) || pad->lx > 195) return DIR_RIGHT;
    return -1;
}

int main(void)
{
    Board board;
    SceCtrlData pad;
    SceRtcTick tick;
    int best, game_over = 0;
    int prev_dir = -1;
    unsigned int prev_buttons = 0;

    sceRtcGetCurrentTick(&tick);
    srand((unsigned int)tick.tick);

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    vita2d_init();
    vita2d_set_clear_color(COL_BG);
    pgf = vita2d_load_default_pgf();

    best = load_best();
    board_reset(&board);

    while (1) {
        int dir;

        memset(&pad, 0, sizeof(pad));
        sceCtrlPeekBufferPositive(0, &pad, 1);

        if (pad.buttons & SCE_CTRL_START)
            break;

        /* Triangle starts a fresh game, on the press edge only */
        if ((pad.buttons & SCE_CTRL_TRIANGLE) && !(prev_buttons & SCE_CTRL_TRIANGLE)) {
            board_reset(&board);
            game_over = 0;
        }

        dir = read_direction(&pad);
        /* only act when the direction changes, so a held d-pad moves once */
        if (dir >= 0 && dir != prev_dir && !game_over) {
            if (board_move(&board, (Direction)dir)) {
                board_spawn(&board);

                if (board.score > best) {
                    best = board.score;
                    save_best(best);
                }
                if (!board_has_moves(&board))
                    game_over = 1;
            }
        }

        prev_dir = dir;
        prev_buttons = pad.buttons;

        vita2d_start_drawing();
        vita2d_clear_screen();
        draw_board(&board, best, game_over);
        vita2d_end_drawing();
        vita2d_swap_buffers();
    }

    vita2d_free_pgf(pgf);
    vita2d_fini();
    sceKernelExitProcess(0);

    return 0;
}

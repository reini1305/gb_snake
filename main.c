#include <gb/gb.h>
#include <gb/cgb.h>
#include <gbdk/console.h>
#include <string.h>
#include <stdio.h>
#include <rand.h>
#include "tiles.h"
#include "map.h"

#define RIGHT 0
#define LEFT  1
#define UP    2
#define DOWN  3
#define UPDATE_SNAKE 15
#define APPLE 144

const uint16_t cgb_palette[4] = {21369, 2737, 6534, 2274};
const int8_t delta_x[4] = {1, -1, 0, 0};
const int8_t delta_y[4] = {0, 0, -1, 1};
uint8_t score;
uint8_t dir;
uint8_t prev_dir;
uint8_t head_x;
uint8_t head_y;
uint8_t tail_x;
uint8_t tail_y;
uint8_t update=0;
uint8_t enable_move=0;

void update_score(void) {
    gotoxy(0,0);
    printf("Score: %d", score);
}

void update_tail(void) {
    uint8_t tile;
    set_vram_byte(get_bkg_xy_addr(tail_x, tail_y), 128);
    // check which tail to draw
    tile = get_bkg_tile_xy(tail_x+1, tail_y);
    if (tile == 130 || tile == 131 || tile == 133){
        tail_x++;
        switch (tile) {
            case 130:
                tile = 138;
                break;
            case 131:
                tile = 136;
                break;
            default:
                tile = 137;
                break;
        } 
    } else {
        tile = get_bkg_tile_xy(tail_x-1, tail_y);
        if (tile == 129 || tile == 132 || tile == 133){
            tail_x--;
            switch (tile) {
                case 129:
                    tile = 138;
                    break;
                case 132:
                    tile = 136;
                    break;
                default:
                    tile = 135;
                    break;
            } 
        } else {
            tile = get_bkg_tile_xy(tail_x, tail_y+1);
            if (tile == 131 || tile == 132 || tile == 134){
                tail_y++;
                switch (tile) {
                    case 131:
                        tile = 135;
                        break;
                    case 132:
                        tile = 137;
                        break;
                    default:
                        tile = 138;
                        break;
                } 
            } else {
                tile = get_bkg_tile_xy(tail_x, tail_y-1);
                tail_y--;
                switch (tile) {
                    case 130:
                        tile = 135;
                        break;
                    case 129:
                        tile = 137;
                        break;
                    default:
                        tile = 136;
                        break;
                } 
            }
        }
    }
    set_vram_byte(get_bkg_xy_addr(tail_x, tail_y), tile);
}

void spawn_apple(void) {
    uint8_t x = (arand() + 128u) / 14 + 1;
    uint8_t y = (arand() + 128u) / 17 + 2;
    while (get_bkg_tile_xy(x, y) != 128) {
        x = (arand() + 128u) / 14 + 1;
        y = (arand() + 128u) / 17 + 2;
    }
    set_vram_byte(get_bkg_xy_addr(x, y), APPLE);
}

uint8_t move_snake(void) {
    uint8_t tile = 0;

    // move head
    head_x += delta_x[dir];
    head_y += delta_y[dir];

    // check if we hit something
    tile = get_bkg_tile_xy(head_x, head_y);
    if (tile > 128 && tile < APPLE) {
        // we hit something, game over
        return 1;
    } 
    if (tile == APPLE) {
        score++;
        spawn_apple();
    } else {
        update_tail();
    }
    // overwrite old head position
    if ((prev_dir == DOWN && dir == RIGHT) || (prev_dir == LEFT && dir == UP)) {
        tile = 132;
    } else if ((prev_dir == DOWN && dir == LEFT) || (prev_dir == RIGHT && dir == UP)) {
        tile = 131;
    } else if ((prev_dir == DOWN && dir == DOWN) || (prev_dir == UP && dir == UP)) {
        tile = 134;
    } else if ((prev_dir == UP && dir == RIGHT) || (prev_dir == LEFT && dir == DOWN)) {
        tile = 129;
    } else if ((prev_dir == UP && dir == LEFT) || (prev_dir == RIGHT && dir == DOWN)) {
        tile = 130;
    } else if ((prev_dir == LEFT && dir == LEFT) || (prev_dir == RIGHT && dir == RIGHT)) {
        tile = 133; 
    }
    set_vram_byte(get_bkg_xy_addr(head_x-delta_x[dir], head_y-delta_y[dir]), tile);
    // draw head
    set_vram_byte(get_bkg_xy_addr(head_x, head_y), 139 + dir);
    prev_dir = dir;
    return 0;
}

void game_over(void) {
    while (score--){
        update_tail();
        for (enable_move = 0; enable_move < 5; enable_move++)
            wait_vbl_done();
    }
    gotoxy(5,9);
    printf("Game Over!");
}

void reset_game(void) {
    score = 0;
    enable_move = 0;
    cls();
    update_score();
    set_bkg_tiles(0, 0, 20,18, background_map);
    update_score();
    spawn_apple();
    dir = RIGHT;
    prev_dir = RIGHT;
    head_x=0x0D;
    head_y=0x08;
    tail_x=0x0B;
    tail_y=0x08;
    initarand(DIV_REG);
}

void main(void)
{
    uint8_t i;
    
    HIDE_BKG;
    set_bkg_data(128, 17, tile_map);
    reset_game();
    set_bkg_palette(0, 1, cgb_palette);
    SHOW_BKG;

    while(1) {
        i = joypad();

        if (i == J_START) {
            enable_move = enable_move? 0:1;
            waitpadup();
        } else if (i == J_LEFT){
            enable_move = 1;
            if (dir != RIGHT)
                dir = LEFT;
        } else if (i == J_RIGHT){
            enable_move = 1;
            if (dir != LEFT)
                dir = RIGHT;
        } else if (i == J_DOWN){
            enable_move = 1;
            if (dir != UP)
                dir = DOWN;
        } else if (i == J_UP){
            enable_move = 1;
            if (dir != DOWN)
                dir = UP;
        }
        if (enable_move) {
            update++;
        }
        if (update == UPDATE_SNAKE) {
            if (move_snake()) {
                game_over();
                waitpadup();
                while (!joypad())
                {
                    wait_vbl_done();
                }
                waitpadup();
                reset_game();
            }
            update = 0;
            update_score();
        }
        wait_vbl_done();
    }
}

#include <gb/gb.h>
#include <gbdk/console.h>
#include <rand.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "game.h"
#include "link.h"
#include "PrintCmd.h"
#include "res/map.h"
#include "res/battlemap.h"
#include "res/title.h"
#include "sound/sound.h"

// Global variables defined in main.c
extern uint8_t new_scx_reg;
extern uint8_t new_scy_reg;
extern uint8_t scx_reg;
extern uint8_t scy_reg;
extern snake snakes[2];
extern uint8_t player;
extern uint8_t multiplayer;
extern uint8_t enable_move;
extern uint8_t play_audio;

// Global variables only for game.c
uint8_t num_apples=1;
const int8_t delta_x[4] = {1, -1, 0, 0};
const int8_t delta_y[4] = {0, 0, -1, 1};
uint8_t synced=FALSE;
uint16_t highscore=0;
uint8_t printed=FALSE;


uint8_t min(uint8_t a, uint8_t b) {
    return a<b? a:b;
}

uint8_t max(uint8_t a, uint8_t b) {
    return a>b? a:b;
}

void update_scroll(void) {
    new_scx_reg = min(max(snakes[player].head_x,10)-10,12)*8;
    new_scy_reg = min(max(snakes[player].head_y,9)-9,14)*8;
}

void update_score(void) {
    if (multiplayer){
        gotoxy(7,0);
        printf("S2 %d", snakes[1].score);
        gotoxy(0,0);
        printf("S1 %d", snakes[0].score);
    } else {
        gotoxy(0,0);
        printf("Score %d", snakes[player].score);
    }
}

void update_tail(uint8_t player) {
    uint8_t tile;
    set_vram_byte(get_bkg_xy_addr(snakes[player].tail_x, snakes[player].tail_y), OFFSET);
    // check which tail to draw
    tile = get_bkg_tile_xy(snakes[player].tail_x+1, snakes[player].tail_y);
    if (tile == 130 || tile == 131 || tile == 133){
        snakes[player].tail_x++;
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
        tile = get_bkg_tile_xy(snakes[player].tail_x-1, snakes[player].tail_y);
        if (tile == 129 || tile == 132 || tile == 133){
            snakes[player].tail_x--;
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
            tile = get_bkg_tile_xy(snakes[player].tail_x, snakes[player].tail_y+1);
            if (tile == 131 || tile == 132 || tile == 134){
                snakes[player].tail_y++;
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
                tile = get_bkg_tile_xy(snakes[player].tail_x, snakes[player].tail_y-1);
                snakes[player].tail_y--;
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
    set_vram_byte(get_bkg_xy_addr(snakes[player].tail_x, snakes[player].tail_y), tile);
}

void spawn_apple(void) {
    uint8_t x = min(max(2, (arand() + RAND_OFFSET) / 8), 29);
    uint8_t y = min(max(3, (arand() + RAND_OFFSET) / 8), 29);
    while ((get_bkg_tile_xy(x, y) != OFFSET) || 
           (get_bkg_tile_xy(x+1, y) == WALL) ||
           (get_bkg_tile_xy(x, y+1) == WALL) ||
           (get_bkg_tile_xy(x-1, y) == WALL) ||
           (get_bkg_tile_xy(x, y-1) == WALL)) {
        x = min(max(2, (arand() + RAND_OFFSET) / 8), 29);
        y = min(max(3, (arand() + RAND_OFFSET) / 8), 29);
    }
    VBK_REG = 1;
    set_vram_byte(get_bkg_xy_addr(x, y), 2);
    VBK_REG = 0;
    set_vram_byte(get_bkg_xy_addr(x, y), APPLE);
}

void add_apple(void) {
    Audio_SFX_Play(SFX_APPLE_INC);
    if (num_apples >= (multiplayer? 7:10))
    {
        num_apples = 1;
        reset_game(FALSE);
    } else {
        spawn_apple();
        num_apples++;
        set_vram_byte(get_bkg_xy_addr(20-num_apples, 0), APPLE);
    }
}

uint8_t move_snake(uint8_t player) {
    uint8_t tile = 0;

    // move head
    snakes[player].head_x += delta_x[snakes[player].dir];
    snakes[player].head_y += delta_y[snakes[player].dir];

    // check if we hit something
    tile = get_bkg_tile_xy(snakes[player].head_x, snakes[player].head_y);
    if (tile > OFFSET && tile < APPLE) {
        // we hit something, game over
        return 1;
    } 
    if (tile == APPLE) {
        snakes[player].score++;
        Audio_SFX_Play(SFX_APPLE_EAT);
        spawn_apple();
    } else {
        update_tail(player);
    }
    // overwrite old head position
    if ((snakes[player].prev_dir == DOWN && snakes[player].dir == RIGHT) || (snakes[player].prev_dir == LEFT && snakes[player].dir == UP)) {
        tile = 132;
    } else if ((snakes[player].prev_dir == DOWN && snakes[player].dir == LEFT) || (snakes[player].prev_dir == RIGHT && snakes[player].dir == UP)) {
        tile = 131;
    } else if ((snakes[player].prev_dir == DOWN && snakes[player].dir == DOWN) || (snakes[player].prev_dir == UP && snakes[player].dir == UP)) {
        tile = 134;
    } else if ((snakes[player].prev_dir == UP && snakes[player].dir == RIGHT) || (snakes[player].prev_dir == LEFT && snakes[player].dir == DOWN)) {
        tile = 129;
    } else if ((snakes[player].prev_dir == UP && snakes[player].dir == LEFT) || (snakes[player].prev_dir == RIGHT && snakes[player].dir == DOWN)) {
        tile = 130;
    } else if ((snakes[player].prev_dir == LEFT && snakes[player].dir == LEFT) || (snakes[player].prev_dir == RIGHT && snakes[player].dir == RIGHT)) {
        tile = 133; 
    }
    set_vram_byte(get_bkg_xy_addr(snakes[player].head_x-delta_x[snakes[player].dir], snakes[player].head_y-delta_y[snakes[player].dir]), tile);
    // draw head
    VBK_REG = 1;
    set_vram_byte(get_bkg_xy_addr(snakes[player].head_x, snakes[player].head_y), 0);
    VBK_REG = 0;
    set_vram_byte(get_bkg_xy_addr(snakes[player].head_x, snakes[player].head_y), 139 + snakes[player].dir);
    snakes[player].prev_dir = snakes[player].dir;
    update_scroll();
    return 0;
}

inline void show_game_over(uint8_t who_lost) {
    uint8_t i_lost = (player == 0 && (who_lost & 0x01)) || (player == 1 && (who_lost & 0x02));
    uint8_t other_lost = (player == 0 && (who_lost & 0x02)) || (player == 1 && (who_lost & 0x01));
    uint8_t both_lost = i_lost && other_lost;
    if (snakes[player].score > highscore)
        highscore = snakes[player].score;
    
    if (i_lost) {
        while (snakes[player].score--){
            update_tail(player);
            Audio_SFX_Play(SFX_CRASH);
            wait_vbl_done();
        }
        if (multiplayer) {
            gotoxy(15,0);
            printf("Lost!");
        } else {
            gotoxy(10,0);
            printf("Game Over!");
        }
    }
    if (other_lost) {
        other_lost = player==0? 1:0;
        while (snakes[other_lost].score--){
            update_tail(other_lost);
            Audio_SFX_Play(SFX_CRASH);
            wait_vbl_done();
        }
        gotoxy(15,0);
        printf("Won!");
    }
    if (both_lost) {
        gotoxy(15,0);
        printf("Draw!");
    }
}

void reset_game(uint8_t show_title_screen) {
    uint8_t i;
    if (show_title_screen)
        show_title();
    snakes[0].score = 0;
    snakes[1].score = 0;
    enable_move = 0;
    for(i=0; i<20-num_apples; i++){
        set_vram_byte(get_bkg_xy_addr(i,0), OFFSET);
    }
    VBK_REG = 1;
    set_bkg_tiles(0, 0, 32, 32, multiplayer? background_map_battlePLN1 : background_mapPLN1);
    VBK_REG = 0;
    set_bkg_tiles(0, 0, 32, 32, multiplayer? background_map_battlePLN0 : background_mapPLN0);
    update_score();
    for(i=0; i<num_apples; i++){
        spawn_apple();
        set_vram_byte(get_bkg_xy_addr(19-i,0), APPLE);
    }
    snakes[0].dir = RIGHT;
    snakes[0].prev_dir = RIGHT;
    snakes[0].head_x=0x09;
    snakes[0].head_y=0x08;
    snakes[0].tail_x=0x07;
    snakes[0].tail_y=0x08;
    snakes[1].dir = LEFT;
    snakes[1].prev_dir = LEFT;
    snakes[1].head_x=21;
    snakes[1].head_y=27;
    snakes[1].tail_x=23;
    snakes[1].tail_y=27;
    update_scroll();
    if (multiplayer == FALSE) {
        //delete second snake
        set_vram_byte(get_bkg_xy_addr(snakes[1].head_x,snakes[1].head_y), OFFSET);
        set_vram_byte(get_bkg_xy_addr(snakes[1].tail_x,snakes[1].tail_y), OFFSET);
        set_vram_byte(get_bkg_xy_addr(snakes[1].tail_x-1,snakes[1].tail_y), OFFSET);
    } else {
        if (player == 0)
            send_command(RIGHT, FALSE);
        else
            send_command(LEFT, FALSE);
    }
}


void show_title(void) {
    scx_reg = 0;
    scy_reg = 0;
    VBK_REG = 1;
    set_bkg_tiles(0, 0, 20, 18, title_screenPLN1);
    VBK_REG = 0;
    set_bkg_tiles(0, 0, 20, 18, title_screenPLN0);
    gotoxy(3,15);
    if (highscore > 0) {
        printf("Highscore: %d", highscore);
        gotoxy(16,16);
        printf("  ");
    } else {
        printf("Start to play");
        gotoxy(2,15);
        printf("B toggles music");
        gotoxy(2,16);
        printf("Select to add");
    }
    receive_byte();
    while (1)
    {
        if ( joypad() == J_SELECT ){
            if (!(GetPrinterStatus() && CheckLinkCable())) {
                PrinterInit();
                PrintScreen(TRUE);
            }
            printed = TRUE;
        }
        if ((multiplayer == FALSE || (multiplayer == TRUE && player == 0)) && J_START == joypad()) {
            waitpadup();
            break;
        }
        if (multiplayer) {
            gotoxy(2,14);
            printf("You are player %d!",player+1);
        }
        if (_io_status == IO_IDLE && !printed) {
            multiplayer = TRUE;
            if (!synced) {
                synced = TRUE;
                initarand(_io_in);
                player = 1; // we were late in the game
                receive_byte();
            } else if (_io_in == GO){
                // other player pressed start
                return;
            }
        }
        if ( joypad() == J_B ){
            play_audio = play_audio? FALSE:TRUE;
            waitpadup();
        }
        wait_vbl_done();
    }
    waitpadup();
    if (player == 0){
        send_command(GO, TRUE);
    }
}



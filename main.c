#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include <stdio.h>
#include <rand.h>
#include "common.h"
#include "link.h"
#include "game.h"
#include "res/tiles.h"
#include "sound/sound.h"

uint8_t update=0;
uint8_t enable_move=0;
uint8_t new_scx_reg=0;
uint8_t new_scy_reg=0;
uint8_t scx_reg=0;
uint8_t scy_reg=0;
uint8_t player=0;
uint8_t multiplayer=FALSE;
snake snakes[2];
uint8_t play_audio = FALSE;
const uint16_t tile_map_palettes[] =
{
  tile_mapCGBPal0c0,tile_mapCGBPal0c1,tile_mapCGBPal0c2,tile_mapCGBPal0c3,
  tile_mapCGBPal1c0,tile_mapCGBPal1c1,tile_mapCGBPal1c2,tile_mapCGBPal1c3,
  tile_mapCGBPal2c0,tile_mapCGBPal2c1,tile_mapCGBPal2c2,tile_mapCGBPal2c3,
};

void scanline_isr(void) {
    while(STAT_REG & STATF_BUSY);
    switch (LYC_REG) {
        case 0: 
            SCX_REG = 0;
            SCY_REG = 0;
            LYC_REG = 7;
            break;
        case 7:
            SCX_REG = scx_reg;
            SCY_REG = scy_reg;
            LYC_REG = 0;
            break;
    }
}

void vblank_isr(void) {
    if (play_audio) {
        Audio_Music_Resume();
    } else {
        Audio_Music_Stop();
    }
    Audio_FrameProcess();
}

void main(void)
{
    uint8_t i;
    HIDE_BKG;
    printf(" "); // to initialize tile map
    set_bkg_data(OFFSET, 18, tile_map);
    set_bkg_palette(0, 1, &tile_map_palettes[0]);
    set_bkg_palette(1, 1, &tile_map_palettes[4]);
    set_bkg_palette(2, 1, &tile_map_palettes[8]);
    SHOW_BKG;
    CRITICAL {
        STAT_REG |= STATF_LYC; LYC_REG = 0;
        add_LCD(scanline_isr);
        add_VBL(vblank_isr);
        add_SIO(nowait_int_handler);
        set_interrupts(VBL_IFLAG | LCD_IFLAG | SIO_IFLAG);

        Audio_Init();
        Audio_Music_Play(0);
        play_audio = TRUE;
    }
    // Try to send our random seed to the other player
    // i = DIV_REG;
    i = 0;
    initarand(i);
    _io_out = i;
    send_byte();
    while((_io_status == IO_SENDING) && (joypad() == 0));
    if(_io_status == IO_IDLE)
        multiplayer = TRUE;
    else
        multiplayer = FALSE;
    reset_game(TRUE);
    if (multiplayer && player == 1) {
        // player 1 is passive and listening for the sync pulse from player 0
        receive_byte();
    }
    while(1) {
        // update scroll registers
        if (scx_reg < new_scx_reg) {
            scx_reg++;
        } else if (scx_reg > new_scx_reg) {
            scx_reg--;
        }
        if (scy_reg < new_scy_reg) {
            scy_reg++;
        } else if (scy_reg > new_scy_reg) {
            scy_reg--;
        }

        i = joypad();
       
        if ((multiplayer && player == 0 || !multiplayer) && i == J_START && enable_move == 0) {
            enable_move = 1;
            waitpadup();
            send_command(START, FALSE);
        } else if (i == J_SELECT && enable_move == 0 && (multiplayer && player == 0 || !multiplayer)) {
            add_apple();
            waitpadup();
            send_command(SELECT, FALSE);
        } else if (i == J_LEFT && enable_move == 1){
            if (snakes[player].prev_dir != RIGHT) {
                snakes[player].dir = LEFT;
                send_command(LEFT, FALSE);
            }
        } else if (i == J_RIGHT && enable_move == 1){
            if (snakes[player].prev_dir != LEFT) {
                snakes[player].dir = RIGHT;
                send_command(RIGHT, FALSE);
            }
        } else if (i == J_DOWN && enable_move == 1){
            if (snakes[player].prev_dir != UP) {
                snakes[player].dir = DOWN;
                send_command(DOWN, FALSE);
            }
        } else if (i == J_UP && enable_move == 1){
            if (snakes[player].prev_dir != DOWN) {
                snakes[player].dir = UP;
                send_command(UP, FALSE);
            }
        }
        if (++update == UPDATE_SNAKE) {
            // both players come here and sync up the state of the snakes before updating the
            // field
            update = 0;
            sync();
            if (enable_move) {
                uint8_t game_over;
                game_over = move_snake(0);
                if (multiplayer) {
                    game_over += 2*move_snake(1);
                }
                if (game_over) {
                    show_game_over(game_over);
                    waitpadup();
                    if (player == 0) { 
                        while (joypad() != J_START)
                        {
                            wait_vbl_done();
                        }
                        waitpadup();
                        send_command(GO, TRUE);
                    } else {
                        // wait for the start command from player 1
                        do {
                            while (_io_status == IO_RECEIVING);
                            receive_byte();
                        } while (_io_in != GO);
                    }
                    reset_game(TRUE);
                }
                update_score();
            }
        }
        wait_vbl_done();
    }
}

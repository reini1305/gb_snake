#include <gb/gb.h>
#include <gb/cgb.h>
#include <gbdk/console.h>
#include <string.h>
#include <stdio.h>
#include <rand.h>
#include "res/tiles.h"
#include "res/map.h"
#include "res/title.h"
#include "PrintCmd.h"
#include "common.h"

#include "sound/sound.h"
#define SFX_APPLE_EAT 0x11
#define SFX_CRASH 0xC
#define SFX_APPLE_INC 0x4 

uint8_t play_audio = FALSE;

#define UPDATE_SNAKE 8
#define OFFSET 128
#define APPLE 144
#define WALL  143
#define RAND_OFFSET ((__GBDK_VERSION <= 405)? 128 : 8)

const int8_t delta_x[4] = {1, -1, 0, 0};
const int8_t delta_y[4] = {0, 0, -1, 1};
uint16_t highscore = 0;
uint8_t update=0;
uint8_t enable_move=0;
uint8_t new_scx_reg=0;
uint8_t new_scy_reg=0;
uint8_t scx_reg=0;
uint8_t scy_reg=0;
uint8_t num_apples=1;
uint8_t player=0;
uint8_t multiplayer=FALSE;
uint8_t synced=FALSE;

typedef struct snake_ {
    uint8_t dir;
    uint8_t prev_dir;
    uint8_t head_x;
    uint8_t head_y;
    uint8_t tail_x;
    uint8_t tail_y;
    uint16_t score;
} snake;
snake snakes[2];

void send_command(uint8_t command, uint8_t force) {
    static uint8_t prev_command = UNKNOWN;
    if (multiplayer && command != prev_command || force) {
        _io_out = command;
        send_byte();
        prev_command = command;
    }
}

void update_score(void) {
    if (multiplayer){
        gotoxy(7,0);
        printf("S2 %d", snakes[1].score);
        gotoxy(0,0);
        printf("S1 %d", snakes[0].score);
        //send_command(snakes[player].score + UNKNOWN);
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

void game_over(uint8_t who_lost) {
    uint8_t i_lost = (player == 0 && (who_lost & 0x01)) || (player == 1 && (who_lost & 0x02));
    uint8_t other_lost = (player == 0 && (who_lost & 0x02)) || (player == 1 && (who_lost & 0x01));
    if (snakes[player].score > highscore)
        highscore = snakes[player].score;
    
    if (i_lost) {
        while (snakes[player].score--){
            update_tail(player);
            Audio_SFX_Play(SFX_CRASH);
            //for (enable_move = 0; enable_move < 5; enable_move++) // delay update_tail
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
            receive_byte();
        }
        if ((multiplayer == FALSE || (multiplayer == TRUE && player == 0)) && J_START == joypad()) {
            waitpadup();
            break;
        }
        if (multiplayer) {
            gotoxy(2,14);
            printf("You are player %d!",player+1);
        }
        if (_io_status == IO_IDLE) {
            multiplayer = TRUE;
            if (!synced) {
                synced = TRUE;
                initarand(_io_in);
                player = 1; // we were late in the game
            } else if (_io_in == START){
                // other player pressed start
                return;
            }
        }
        if (multiplayer) {
            receive_byte();
        }
        if ( joypad() == J_B ){
            play_audio = play_audio? FALSE:TRUE;
            waitpadup();
        }
        wait_vbl_done();
    }
    waitpadup();
    if (player == 0){
        send_command(START, TRUE);
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
    set_bkg_tiles(0, 0, 32, 32, background_mapPLN1);
    VBK_REG = 0;
    set_bkg_tiles(0, 0, 32, 32, background_mapPLN0);
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
    receive_byte();
    if (multiplayer == FALSE) {
        //delete second snake
        set_vram_byte(get_bkg_xy_addr(snakes[1].head_x,snakes[1].head_y), OFFSET);
        set_vram_byte(get_bkg_xy_addr(snakes[1].tail_x,snakes[1].tail_y), OFFSET);
        set_vram_byte(get_bkg_xy_addr(snakes[1].tail_x-1,snakes[1].tail_y), OFFSET);
    }
}

void scanline_isr() {
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

void vblank_isr() {
    if (play_audio) {
        Audio_Music_Resume();
    } else {
        Audio_Music_Stop();
    }
    Audio_FrameProcess();
}

const uint16_t tile_map_palettes[] =
{
  tile_mapCGBPal0c0,tile_mapCGBPal0c1,tile_mapCGBPal0c2,tile_mapCGBPal0c3,
  tile_mapCGBPal1c0,tile_mapCGBPal1c1,tile_mapCGBPal1c2,tile_mapCGBPal1c3,
  tile_mapCGBPal2c0,tile_mapCGBPal2c1,tile_mapCGBPal2c2,tile_mapCGBPal2c3,
};

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

void process_link(void) {
    uint8_t other = player == 0? 1:0;
    uint8_t received = _io_in;
    switch (received) {
        case START:
            enable_move = 1;
            break;
        case UP:
        case DOWN:
        case LEFT:
        case RIGHT:
            snakes[other].dir = received;
            break;
        case SELECT:
            add_apple();
            break;
        default:
            break;
    }
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
    i = DIV_REG;
    initarand(i);
    _io_out = i;
    send_byte();
    while((_io_status == IO_SENDING) && (joypad() == 0));
    if(_io_status == IO_IDLE)
        multiplayer = TRUE;
    else
        multiplayer = FALSE;
    reset_game(TRUE);
    while(1) {
        wait_vbl_done();
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

        if (multiplayer){ 
            if (_io_status == IO_IDLE) {
                // we have received something
                process_link();
            }
            receive_byte();
        }

        if (enable_move) {
            update++;
        }
        if (update == UPDATE_SNAKE) { 
            uint8_t go;
            go = move_snake(0);
            if (multiplayer) {
                go += 2*move_snake(1);
            }
            if (go) {
                game_over(go);
                waitpadup();
                if (player == 0) {   
                    while (!joypad())
                    {
                        wait_vbl_done();
                    }
                    waitpadup();
                    send_command(START, TRUE);
                } else {
                    while (_io_status == IO_RECEIVING);
                }
                reset_game(TRUE);
            }
            update = 0;
            update_score();
        }
        
        if ((multiplayer && player == 1 || !multiplayer) && i == J_START) {
            enable_move = 1;
            waitpadup();
            send_command(START, TRUE);
        } else if (i == J_SELECT && enable_move == 0) {
            add_apple();
            waitpadup();
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
    }
}

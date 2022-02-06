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

#include "sound/sound.h"
#define SFX_APPLE_EAT 0x11
#define SFX_CRASH 0xC
#define SFX_APPLE_INC 0x4 

//extern const unsigned char audio_sys[];
uint8_t play_audio = 0;

#define RIGHT 0
#define LEFT  1
#define UP    2
#define DOWN  3
#define UPDATE_SNAKE 8
#define OFFSET 128
#define APPLE 144
#define WALL  143
#define RAND_OFFSET ((__GBDK_VERSION <= 405)? 128 : 8)

const int8_t delta_x[4] = {1, -1, 0, 0};
const int8_t delta_y[4] = {0, 0, -1, 1};
uint16_t score;
uint16_t highscore = 0;
uint8_t dir;
uint8_t prev_dir;
uint8_t head_x;
uint8_t head_y;
uint8_t tail_x;
uint8_t tail_y;
uint8_t update=0;
uint8_t enable_move=0;
uint8_t new_scx_reg=0;
uint8_t new_scy_reg=0;
uint8_t scx_reg=0;
uint8_t scy_reg=0;
uint8_t num_apples=1;

void update_score(void) {
    gotoxy(0,0);
    printf("Score %d", score);
}

void update_tail(void) {
    uint8_t tile;
    set_vram_byte(get_bkg_xy_addr(tail_x, tail_y), OFFSET);
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

uint8_t min(uint8_t a, uint8_t b) {
    return a<b? a:b;
}

uint8_t max(uint8_t a, uint8_t b) {
    return a>b? a:b;
}

void update_scroll(void) {
    new_scx_reg = min(max(head_x,10)-10,12)*8;
    new_scy_reg = min(max(head_y,9)-9,14)*8;
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

uint8_t move_snake(void) {
    uint8_t tile = 0;

    // move head
    head_x += delta_x[dir];
    head_y += delta_y[dir];

    // check if we hit something
    tile = get_bkg_tile_xy(head_x, head_y);
    if (tile > OFFSET && tile < APPLE) {
        // we hit something, game over
        return 1;
    } 
    if (tile == APPLE) {
        score++;
        Audio_SFX_Play(SFX_APPLE_EAT);
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
    VBK_REG = 1;
    set_vram_byte(get_bkg_xy_addr(head_x, head_y), 0);
    VBK_REG = 0;
    set_vram_byte(get_bkg_xy_addr(head_x, head_y), 139 + dir);
    prev_dir = dir;
    update_scroll();
    return 0;
}

void game_over(void) {
    if (score > highscore)
        highscore = score;
    while (score--){
        Audio_SFX_Play(SFX_CRASH);
        update_tail();
        for (enable_move = 0; enable_move < 5; enable_move++)
            wait_vbl_done();
    }
    Audio_SFX_Play(SFX_CRASH);
    gotoxy(10,0);
    printf("Game Over!");
}

void show_title(void) {
    scx_reg = 0;
    scy_reg = 0;
    VBK_REG = 1;
    set_bkg_tiles(0, 0, 20, 18, title_screenPLN1);
    VBK_REG = 0;
    set_bkg_tiles(0, 0, 20, 18, title_screenPLN0);
    gotoxy(3,14);
    if (highscore > 0) {
        printf("Highscore: %d", highscore);
        gotoxy(16,16);
        printf("  ");
    } else {
        printf("Start to play");
        gotoxy(2,16);
        printf("Select to add");
    }
    while (J_START != joypad())
    {
        if ( joypad() == J_SELECT ){
            if (!(GetPrinterStatus() && CheckLinkCable())) {
                PrinterInit();
                PrintScreen(TRUE);
            }
        }
        wait_vbl_done();
    }
    waitpadup();
}

void reset_game(uint8_t show_title_screen) {
    uint8_t i;
    if (show_title_screen)
        show_title();
    score = 0;
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
    dir = RIGHT;
    prev_dir = RIGHT;
    head_x=0x09;
    head_y=0x08;
    tail_x=0x07;
    tail_y=0x08;
    update_scroll();
    initarand(DIV_REG);
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
        Audio_FrameProcess();
    }
}

const uint16_t tile_map_palettes[] =
{
  tile_mapCGBPal0c0,tile_mapCGBPal0c1,tile_mapCGBPal0c2,tile_mapCGBPal0c3,
  tile_mapCGBPal1c0,tile_mapCGBPal1c1,tile_mapCGBPal1c2,tile_mapCGBPal1c3,
  tile_mapCGBPal2c0,tile_mapCGBPal2c1,tile_mapCGBPal2c2,tile_mapCGBPal2c3,
};

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
    reset_game(TRUE);
    CRITICAL {
        STAT_REG |= STATF_LYC; LYC_REG = 0;
        add_LCD(scanline_isr);
        add_VBL(vblank_isr);
        set_interrupts(VBL_IFLAG | LCD_IFLAG);

        Audio_Init();
        Audio_Music_Play(0);
        play_audio = 1;
    }



    while(1) {
        i = joypad();

        if (i == J_START) {
            enable_move = 1;
            waitpadup();
        } else if (i == J_SELECT && enable_move == 0) {
            Audio_SFX_Play(SFX_APPLE_INC);
            if (num_apples >= 10)
            {
                num_apples = 1;
                reset_game(FALSE);
            } else {
                spawn_apple();
                num_apples++;
                set_vram_byte(get_bkg_xy_addr(20-num_apples, 0), APPLE);
            }
            waitpadup();
        } else if (i == J_LEFT || i == J_B){
            enable_move = 1;
            if (dir != RIGHT)
                dir = LEFT;
        } else if (i == J_RIGHT || i == J_A){
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
                reset_game(TRUE);
            }
            update = 0;
            update_score();
        }
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
    }
}

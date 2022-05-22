#ifndef COMMON_H
#define COMMON_H

#include <gb/gb.h>

#define RIGHT   0
#define LEFT    1
#define UP      2
#define DOWN    3
#define START   4
#define SELECT  5
#define GO      6
#define UNKNOWN 7

#define SFX_APPLE_EAT 0x11
#define SFX_CRASH 0xC
#define SFX_APPLE_INC 0x4
#define UPDATE_SNAKE 8
#define OFFSET 128
#define APPLE 144
#define WALL  143
#define RAND_OFFSET ((__GBDK_VERSION <= 405)? 128 : 8)

typedef struct snake_ {
    uint8_t dir;
    uint8_t prev_dir;
    uint8_t head_x;
    uint8_t head_y;
    uint8_t tail_x;
    uint8_t tail_y;
    uint16_t score;
} snake;

#endif //COMMON_H
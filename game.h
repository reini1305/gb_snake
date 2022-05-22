#ifndef GAME_H
#define GAME_H
#include <gb/gb.h>

extern void update_scroll(void);
extern void update_score(void);
extern void update_tail(uint8_t player);
extern void spawn_apple(void);
extern void add_apple(void);
extern uint8_t move_snake(uint8_t player);
extern inline void show_game_over(uint8_t who_lost);
extern void reset_game(uint8_t show_title_screen);
extern void show_title(void);
#endif //GAME_H
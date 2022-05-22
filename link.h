#ifndef LINK_H
#define LINK_H
#include <gb/gb.h>

extern void send_command(uint8_t command, uint8_t now);
extern void process_link(void);
extern void sync(void);

#endif //LINK_H
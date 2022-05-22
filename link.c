#include "common.h"
#include "game.h"

extern uint8_t multiplayer;
extern uint8_t player;
extern uint8_t enable_move;
extern snake snakes[2];

void send_command(uint8_t command, uint8_t now) {
    if (multiplayer) {
        _io_out = command;
        if (now) {
            send_byte();
        }
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

void sync(void) {
    if (multiplayer) {
        if (player == 0) {
            // We start by sending our current command/direction
            delay(5);
            send_byte();
            while (_io_status == IO_SENDING);
            if (_io_status == IO_ERROR) {
                // We send again  our current command/direction
                delay(5);
                send_byte();
                while (_io_status == IO_SENDING);
            }
            // Now we wait for the answer of player 1
            receive_byte();
            while (_io_status == IO_RECEIVING);
            process_link();
            send_command(UNKNOWN, FALSE);
        } else {
            // We are the passive one and wait until we received a command
            while (_io_status == IO_RECEIVING);
            process_link();
            // Now we send our update
            delay(5);
            send_byte();
            while (_io_status == IO_SENDING);
            if (_io_status == IO_ERROR) {
                delay(5);
                 // We send again our current command/direction
                send_byte();
                while (_io_status == IO_SENDING);
            }
            // And go back to listening
            receive_byte();
        }
    }
}


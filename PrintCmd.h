// Original library from http://shen.mansell.tripod.com/games/gameboy/gameboy.html
// Ported to GBDK 2020 by T0biasCZe https://drive.google.com/file/d/1pEHIUL0EyPDbkVUQ2C5DMvJX_z5_1e4X/view?usp=sharing

#include <gb/gb.h>

extern unsigned char PrinterStatus[3];

extern void PrinterInit (void);
extern void SendByte(unsigned char byte);
extern void SendPrinterCommand(unsigned char *Command);
extern int CheckLinkCable(void);
extern int GetPrinterStatus(void);
extern int CheckForErrors(void);
extern void PrintTileData(unsigned char *TileData, uint8_t lf, uint8_t num_packets);
extern uint8_t CheckBusy(void);
extern void PrintScreen(uint8_t linefeed);

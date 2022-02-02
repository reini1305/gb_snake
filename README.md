# Snake for the Gameboy / Gameboy Color

![Screenshot Color](https://github.com/reini1305/gb_snake/raw/main/images/snake.png)
![Title Color](https://github.com/reini1305/gb_snake/raw/main/images/title.png)

# Usage
You can load the ROM onto a flash cart or in an emulator. The key mapping is as follows:
- D-Pad to change direction
- SELECT sets the number of apples on the board before game start
- START starts the game
- On the splash screen, pressing SELECT will cause the highscore to get printed (if a Gameboy Printer is connected)

# Compilation
The ROM can be compiled with [GBDK 2020](https://github.com/gbdk-2020/gbdk-2020). The repository contains a Makefile for Linux and Windows with WSL that will work if you clone it in the gbdk folder. For convenience, a compiled ROM is provided in the [releases](https://github.com/reini1305/gb_snake/releases).
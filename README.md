# A MineSweeper-like game
Using SDL2, runs natively and on the web using emscripten.

![image](https://github.com/Mkac003/mines/assets/70202245/97a75e32-486a-4a87-879f-fd295dc2bf0b)

## Building
#### Native
requres the SDL2 and SDL2_image libraries installed
`gcc mines.c -lSDL2 -lSDL2_image -o mines_build`

#### Web
The web version is made using [Emscripten](https://emscripten.org/). `emcc` needs to be avaliable, see the [emscripten installation guide](https://emscripten.org/docs/getting_started/downloads.html) for further details.
`emcc mines.c -O3 --shell-file shell.html --preload-file res -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["png"]' -o build/web.html`

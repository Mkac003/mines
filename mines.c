/*
  A MineSweeper game written by Mkac003 (http://martin.don.cz)
  
  Features:
    - usual minesweeper things (open tile, set flag, ...)
    - chording (with the middle mouse button) see https://en.wikipedia.org/wiki/Chording section 'Minesweeper tactic'
  
  todo: make this codebase compilable with emscripten
*/

// source emsdk/emsdk_env.sh
// emcc mines.c -O3 --shell-file shell.html --preload-file res -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["png"]' -o web/web.html

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

int count_set_bits(int n) {
  int c = 0;
  while (n) {
    c += n & 1;
    n >>= 1;
    }
  return c;
  }

void draw_texture(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y) {
  int w, h;
  SDL_QueryTexture(texture, NULL, NULL, &w, &h);
  SDL_RenderCopy(renderer, texture, NULL, &(SDL_Rect) {x, y, w, h});
  }

void draw_rigid_rect(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y, int w, int h) {
  SDL_SetRenderDrawColor(renderer, 165, 165, 165, 255);
  SDL_RenderFillRect(renderer, &(SDL_Rect) {x, y, w, h});
  
  SDL_SetRenderDrawColor(renderer, 248, 248, 248, 255);
  SDL_RenderFillRect(renderer, &(SDL_Rect) {x, y, w, 3});
  
  SDL_SetRenderDrawColor(renderer, 207, 207, 207, 255);
  SDL_RenderFillRect(renderer, &(SDL_Rect) {x, y, 3, h});
  
  SDL_SetRenderDrawColor(renderer, 94, 94, 94, 255);
  SDL_RenderFillRect(renderer, &(SDL_Rect) {x, y+h-3, w, 3});
  
  SDL_SetRenderDrawColor(renderer, 127, 127, 127, 255);
  SDL_RenderFillRect(renderer, &(SDL_Rect) {x+w-3, y, 3, h});
  
  SDL_RenderCopy(renderer, texture, &(SDL_Rect) {0, 0, 3, 3}, &(SDL_Rect) {x, y, 3, 3});
  SDL_RenderCopy(renderer, texture, &(SDL_Rect) {19, 0, 3, 3}, &(SDL_Rect) {x+w-3, y, 3, 3});
  SDL_RenderCopy(renderer, texture, &(SDL_Rect) {19, 19, 3, 3}, &(SDL_Rect) {x+w-3, y+h-3, 3, 3});
  SDL_RenderCopy(renderer, texture, &(SDL_Rect) {0, 19, 3, 3}, &(SDL_Rect) {x, y+h-3, 3, 3});
  }

void draw_inset_rect(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y, int w, int h) {
  SDL_SetRenderDrawColor(renderer, 165, 165, 165, 255);
  SDL_RenderFillRect(renderer, &(SDL_Rect) {x, y, w, h});
  
  SDL_SetRenderDrawColor(renderer, 94, 94, 94, 255);
  SDL_RenderFillRect(renderer, &(SDL_Rect) {x, y, w, 3});
  
  SDL_SetRenderDrawColor(renderer, 127, 127, 127, 255);
  SDL_RenderFillRect(renderer, &(SDL_Rect) {x, y, 3, h});
  
  SDL_SetRenderDrawColor(renderer, 248, 248, 248, 255);
  SDL_RenderFillRect(renderer, &(SDL_Rect) {x, y+h-3, w, 3});
  
  SDL_SetRenderDrawColor(renderer, 207, 207, 207, 255);
  SDL_RenderFillRect(renderer, &(SDL_Rect) {x+w-3, y, 3, h});
  
  SDL_RenderCopy(renderer, texture, &(SDL_Rect) {0, 0, 3, 3}, &(SDL_Rect) {x, y, 3, 3});
  SDL_RenderCopy(renderer, texture, &(SDL_Rect) {19, 0, 3, 3}, &(SDL_Rect) {x+w-3, y, 3, 3});
  SDL_RenderCopy(renderer, texture, &(SDL_Rect) {19, 19, 3, 3}, &(SDL_Rect) {x+w-3, y+h-3, 3, 3});
  SDL_RenderCopy(renderer, texture, &(SDL_Rect) {0, 19, 3, 3}, &(SDL_Rect) {x, y+h-3, 3, 3});
  }

// Button
typedef struct {
  int x;
  int y;
  int w;
  int h;
  int image;
  uint8_t state;
  } Button;

#define BUTTON_CLICKED 1 // 0000 0001
#define BUTTON_PUSHED  2 // 0000 0010
#define BUTTON_HOVERED 4 // 0000 0100

#define BUTTON_IS_CLICKED(b) (b->state & BUTTON_CLICKED)
#define BUTTON_IS_HOVERED(b) (b->state & BUTTON_HOVERED)
#define BUTTON_IS_PUSHED(b)  (b->state & BUTTON_PUSHED)

void draw_button(SDL_Renderer *renderer, Button *button, SDL_Texture **textures) {
  int offset_x = 3;
  int offset_y = 3;
  if (BUTTON_IS_PUSHED(button)) {
    draw_inset_rect(renderer, textures[12], button->x, button->y, button->w, button->h);
    offset_x ++;
    offset_y ++;
    }
  else {
    draw_rigid_rect(renderer, textures[11], button->x, button->y, button->w, button->h);
    }
  draw_texture(renderer, textures[button->image], button->x+offset_x, button->y+offset_y);
  }

void update_button(Button *button, uint32_t mouse_just_clicked) {
  int mouse_x, mouse_y;
  uint32_t button_state = SDL_GetMouseState(&mouse_x, &mouse_y);
  
  if ((mouse_x >= button->x && mouse_x <= button->x+button->w)
   && (mouse_y >= button->y && mouse_y <= button->y+button->h)) button->state |= BUTTON_HOVERED;
  else button->state &= ~BUTTON_HOVERED;
  
  if (button_state & SDL_BUTTON(SDL_BUTTON_LEFT)
                   && BUTTON_IS_HOVERED(button)) button->state |= BUTTON_PUSHED;
  else button->state &= ~BUTTON_PUSHED;
  
  if (BUTTON_IS_CLICKED(button)) button->state &= ~BUTTON_CLICKED;
  if (BUTTON_IS_HOVERED(button)) {
    if (mouse_just_clicked & SDL_BUTTON_LEFT) {
      button->state |= BUTTON_CLICKED;
      // printf("MOUSEBUTTONUP\n");
      }
    }
  
  }

// Tile
#define Tile uint8_t

#define RIGHT_MASK 15 // 0000 1111

#define TILE0       0 // 0000 0000
#define TILE1       1 // 0000 0001
#define TILE2       2 // 0000 0010
#define TILE3       3 // 0000 0011
#define TILE4       4 // 0000 0100
#define TILE5       5 // 0000 0101
#define TILE6       6 // 0000 0110
#define TILE7       7 // 0000 0111
#define TILE8       8 // 0000 1000
#define TILE_INVA  10 // 0000 1010 invalid tile
#define TILE_FLAG  16 // XXX1 XXXX
#define TILE_MINE  32 // XX1X XXXX
#define TILE_RVLD  64 // X1XX XXXX 1 if the tile is revealed
#define TILE_WFLG 128 // 1XXX XXXX 1 if the tile is dented

#define IS_MINE(x) (x & TILE_MINE)
#define IS_FLAG(x) (x & TILE_FLAG)
#define IS_RVLD(x) (x & TILE_RVLD)
#define IS_WFLG(x) (x & TILE_WFLG)
#define TILE_GET_NUMBER(x) (x & RIGHT_MASK)
#define IS_EMPTY(x) ((x & RIGHT_MASK) == 0)

#define IN_FIELD(x, y, field) !(x < 0 || x >= field->width || y < 0 || y >= field->height)

#define TILE_SIZE 22
#define TOPBAR_HEIGHT 70

const int offsets3x3[] = {-1, 0, -1, -1, 0, -1, 1, -1, 1, 0, 1, 1, 0, 1, -1, 1, 0, 0};

typedef struct {
  int width;
  int height;
  Tile **tiles;
  } MineField;

Tile get_tile(MineField *field, int x, int y) {
  if (x < 0 || x >= field->width)  return TILE_INVA;
  if (y < 0 || y >= field->height) return TILE_INVA;
  return field->tiles[x][y];
  }

uint8_t reveal(MineField *field, int x, int y) {
  if (get_tile(field, x, y) == TILE_INVA) return TILE_INVA;
  field->tiles[x][y] |= TILE_RVLD;
  field->tiles[x][y] &= ~TILE_FLAG;
  
  return field->tiles[x][y];
  }

uint8_t check(MineField *field, int x, int y) {
  if (get_tile(field, x, y) == TILE_INVA) return TILE_INVA;
  return field->tiles[x][y];
  }

MineField *generate_field(int width, int height, int n_mines) {
  MineField *field = malloc(sizeof(MineField));
  field->width = width;
  field->height = height;
  
  Tile **tiles = malloc(sizeof(Tile *) * width);
  field->tiles = tiles;
  
  for (int r=0;r<width;r++) {
    tiles[r] = calloc(height, sizeof(Tile));
    }
  
  int n = 0;
  int i = 0;
  int x, y;
  while (n < n_mines) {
    x = rand() % width;
    y = rand() % height;
    
    if (i > n_mines*2) break; // iteration limit limit
    i ++;
    if (IS_MINE(tiles[x][y])) continue;
    
    tiles[x][y] |= TILE_MINE;
    n ++;
    }
  
  for (x=0;x<width;x++) {
    for (y=0;y<height;y++) {
      if (IS_MINE(tiles[x][y])) continue;
      
      uint8_t neighbors = 0;
      
      for (int i=0;i<16;i+=2) {
        int neighbor_x = x + offsets3x3[i];
        int neighbor_y = y + offsets3x3[i+1];
        
        if (!IN_FIELD(neighbor_x, neighbor_y, field)) continue;
        if (!IS_MINE(get_tile(field, neighbor_x, neighbor_y))) continue;
        neighbors ++;
        }
      
      tiles[x][y] = neighbors;
      }
    }
  
  return field;
  }

// recursive, returns true if a mine was revealed
bool dig(MineField *field, int x, int y) {
  Tile t = field->tiles[x][y];
  bool m = false;
  
  if (IS_FLAG(t)) return m;
  if (IS_RVLD(t)) return m;
  m = m | IS_MINE(reveal(field, x, y));
  if (!IS_EMPTY(t)) return m;
  if (IS_MINE(t)) return m;
  
  for (int i=0;i<16;i+=2) {
    int neighbor_x = x + offsets3x3[i];
    int neighbor_y = y + offsets3x3[i+1];
    t = check(field, neighbor_x, neighbor_y);
    if (t == TILE_INVA) continue;
    
    if (IS_MINE(t)) m = true;
    if (dig(field, neighbor_x, neighbor_y)) m = true;
    }
  
  return m;
  }

void show_all(MineField *field) {
  for (int x=0;x<field->width;x++) {
    for (int y=0;y<field->height;y++) {
      
      Tile t = field->tiles[x][y];
      if (IS_FLAG(t)) {
        if (!IS_MINE(t)) {
          field->tiles[x][y] |= TILE_WFLG;
          }
        }
      else {
        field->tiles[x][y] |= TILE_RVLD;
        }
      
      }
    }
  }

// returns true if a mine has been reached
bool run_chord(MineField *field, int hovered_tile_x, int hovered_tile_y) {
  Tile t;
  uint8_t flags = 0;
  bool m = false;
  
  if (!IN_FIELD(hovered_tile_x, hovered_tile_y, field)) return m;
  if (!IS_RVLD(field->tiles[hovered_tile_x][hovered_tile_y])) return m;
  
  for (int i=0;i<16;i+=2) {
    int x = hovered_tile_x + offsets3x3[i];
    int y = hovered_tile_y + offsets3x3[i+1];
    
    if (!IN_FIELD(x, y, field)) continue;
    
    t = field->tiles[x][y];
    if (IS_FLAG(t)) flags ++;
    }
  
  // printf("run_chord: flags: %d tile_n: %d\n", flags, TILE_GET_NUMBER(field->tiles[hovered_tile_x][hovered_tile_y]));
  if (flags != TILE_GET_NUMBER(field->tiles[hovered_tile_x][hovered_tile_y])) return m;
  for (int i=0;i<16;i+=2) {
    int x = hovered_tile_x + offsets3x3[i];
    int y = hovered_tile_y + offsets3x3[i+1];
    
    if (!IN_FIELD(x, y, field)) continue;
    
    t = field->tiles[x][y];
    if (IS_FLAG(t)) continue;
    
    bool a = dig(field, x, y);
    m = m | a;
    }
  
  return m;
  }

void free_field(MineField *field) {
  for (int r=0;r<field->width;r++) {
    free(field->tiles[r]);
    }
  free(field->tiles);
  free(field);
  }

#define PADDING 8

#define IMG_TILE_FLAG    10
#define IMG_TILE_UNKNOWN 11
#define IMG_TILE_INSET   12
#define IMG_TILE_OPENED  13

#define IMG_BIG_FLAG     14
#define IMG_BIG_MINE     15
#define IMG_BIG_RETRY    16
#define IMG_TILE_WFLG    17

#define GAME_PLAYING 0
#define GAME_OVER    1

int main() {
  // Init
  SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  IMG_Init(IMG_INIT_PNG);
  srand(time(NULL));
  
  // Create a Window
  SDL_Window *window = SDL_CreateWindow("MineSweeper", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 50, 50, SDL_WINDOW_HIDDEN);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  
  // Load assets
  SDL_Texture *textures[] = {
    IMG_LoadTexture(renderer, "res/tile0.png"),
    IMG_LoadTexture(renderer, "res/tile1.png"),
    IMG_LoadTexture(renderer, "res/tile2.png"),
    IMG_LoadTexture(renderer, "res/tile3.png"),
    IMG_LoadTexture(renderer, "res/tile4.png"),
    IMG_LoadTexture(renderer, "res/tile5.png"),
    IMG_LoadTexture(renderer, "res/tile6.png"),
    IMG_LoadTexture(renderer, "res/tile7.png"),
    IMG_LoadTexture(renderer, "res/tile8.png"),
    IMG_LoadTexture(renderer, "res/tile_bomb.png"),
    IMG_LoadTexture(renderer, "res/tile_flag.png"),
    IMG_LoadTexture(renderer, "res/unknown.png"),
    IMG_LoadTexture(renderer, "res/unknown_inset.png"),
    IMG_LoadTexture(renderer, "res/opened.png"),
    IMG_LoadTexture(renderer, "res/bigbutton_flag.png"),
    IMG_LoadTexture(renderer, "res/bigbutton_mine.png"),
    IMG_LoadTexture(renderer, "res/bigbutton_retry.png"),
    IMG_LoadTexture(renderer, "res/tile_wrong_flag.png"),
    };
  
  SDL_Surface *icon = IMG_Load("res/tile8.png");
  SDL_SetWindowIcon(window, icon);
  SDL_FreeSurface(icon);
  
  int field_x = PADDING+3;
  int field_y = TOPBAR_HEIGHT;
  
  // Setup game
  int field_width = 18;
  int field_height = 18;
  int field_mines = 18*18/8;
  
  int game_state = GAME_PLAYING;
  
  MineField *field = generate_field(field_width, field_height, field_mines); // 46, 46, 46*46/10
  bool chord = false;
  
  // UI
  int win_width = field->width*TILE_SIZE+PADDING*2+6;
  int win_height = field->height*TILE_SIZE+TOPBAR_HEIGHT+PADDING+3;
  SDL_SetWindowSize(window, win_width, win_height);
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  
  Button big_button = {win_width/2-19, TOPBAR_HEIGHT/2-19, 38, 38, IMG_BIG_FLAG, 0};
  
  // Run!
  SDL_Event event;
  uint32_t mouse_just_clicked = 0;
  
  int run = 1;
  int hovered_tile_x, hovered_tile_y;
  int mouse_x, mouse_y;
  
  SDL_ShowWindow(window);
  while (run) {
    SDL_GetMouseState(&mouse_x, &mouse_y);
    
    hovered_tile_x = (mouse_x-field_x) / TILE_SIZE;
    hovered_tile_y = (mouse_y-field_y) / TILE_SIZE;
    
    mouse_just_clicked = 0;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) run = 0;
      if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (game_state == GAME_PLAYING) {
          
          if ((mouse_x >= field_x && mouse_x < field_width*TILE_SIZE+field_x) && (mouse_y >= field_y && mouse_y < field_height*TILE_SIZE+field_y)) {
            const Tile t = field->tiles[hovered_tile_x][hovered_tile_y];
            
            hovered_tile_x = (event.button.x-field_x) / TILE_SIZE;
            hovered_tile_y = (event.button.y-field_y) / TILE_SIZE;
            if (event.button.button == SDL_BUTTON_LEFT && !IS_RVLD(t)) {
              if (dig(field, hovered_tile_x, hovered_tile_y)) {
                game_state = GAME_OVER;
                show_all(field);
                big_button.image = IMG_BIG_RETRY;
                }
              }
            else if (event.button.button == SDL_BUTTON_RIGHT) {
              if (!IS_RVLD(t)) {
                if (IS_FLAG(t)) {
                  field->tiles[hovered_tile_x][hovered_tile_y] &= ~TILE_FLAG;
                  }
                else {
                  field->tiles[hovered_tile_x][hovered_tile_y] |= TILE_FLAG;
                  }
                }
              }
            else if (event.button.button == SDL_BUTTON_MIDDLE) {
              chord = true;
              // printf("hovered: %d %d\n", hovered_tile_x, hovered_tile_y);
              }
            }
          }
        }
      if (event.type == SDL_MOUSEBUTTONUP) {
        mouse_just_clicked |= event.button.button;
        if (event.button.button == SDL_BUTTON_MIDDLE) {
          chord = false;
          if (run_chord(field, hovered_tile_x, hovered_tile_y)) {
            printf("run_chord: digged up a mine\n");
            game_state = GAME_OVER;
            show_all(field);
            big_button.image = IMG_BIG_RETRY;
            }
          }
        }
      }
    
    if (!run) break;
    
    update_button(&big_button, mouse_just_clicked);
    
    if (big_button.state & BUTTON_CLICKED) {
      free_field(field);
      field = generate_field(field_width, field_height, field_mines);
      game_state = GAME_PLAYING;
      big_button.image = IMG_BIG_FLAG;
      }
    
    
    // Draw
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // Top Bar
    draw_rigid_rect(renderer, textures[11], 0, 0, field->width*TILE_SIZE+PADDING*2+6, field->height*TILE_SIZE+TOPBAR_HEIGHT+3+PADDING);
    draw_inset_rect(renderer, textures[12], PADDING, PADDING, field->width*TILE_SIZE+6, TOPBAR_HEIGHT-PADDING*2);
    draw_button(renderer, &big_button, textures);
    
    draw_inset_rect(renderer, textures[12], field_x-3, field_y-3, field->width*TILE_SIZE+6, field->height*TILE_SIZE+6);
    
    // Field
    Tile t;
    int screen_x, screen_y;
    for (int x=0;x<field->width;x++) {
      for (int y=0;y<field->height;y++) {
        t = field->tiles[x][y];
        uint8_t n = TILE_GET_NUMBER(t);
        
        screen_x = x * TILE_SIZE + field_x;
        screen_y = y * TILE_SIZE + field_y;
        if (IS_RVLD(t)) {
          if (IS_MINE(t)) {
            draw_texture(renderer, textures[IMG_TILE_UNKNOWN], screen_x, screen_y);
            draw_texture(renderer, textures[9], screen_x+3, screen_y+3);
            }
          else if (IS_WFLG(t)) {
            draw_texture(renderer, textures[IMG_TILE_UNKNOWN], screen_x, screen_y);
            draw_texture(renderer, textures[IMG_TILE_WFLG], screen_x+3, screen_y+3);
            }
          else {
            draw_texture(renderer, textures[IMG_TILE_OPENED], screen_x, screen_y);
            draw_texture(renderer, textures[n], screen_x+3, screen_y+3);
            }
          }
        else {
          draw_texture(renderer, textures[IMG_TILE_UNKNOWN], screen_x, screen_y);
          if (IS_FLAG(t)) {
            draw_texture(renderer, textures[IMG_TILE_FLAG], screen_x+3, screen_y+3);
            }
          }
        }
      }
    
    if (chord) {
      Tile t;
      for (int i=0;i<18;i+=2) {
        int x = hovered_tile_x + offsets3x3[i];
        int y = hovered_tile_y + offsets3x3[i+1];
        
        if (!IN_FIELD(x, y, field)) continue;
        
        t = field->tiles[x][y];
        if (IS_RVLD(t)) continue;
        if (IS_FLAG(t)) continue;
        
        screen_x = x * TILE_SIZE + field_x;
        screen_y = y * TILE_SIZE + field_y;
        
        draw_texture(renderer, textures[IMG_TILE_OPENED], screen_x, screen_y);
        }
      }
    
    SDL_RenderPresent(renderer);
    SDL_Delay(1/60);
    }
  
  // Quit
  for (int i=0;i<sizeof(textures)/sizeof(SDL_Texture *);i++) SDL_DestroyTexture(textures[i]);
  free_field(field);
  
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  
  return 0;
  }
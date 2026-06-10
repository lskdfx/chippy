#include "chip8.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t FONT[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("Usage: %s <ROM> <Scale> <Delay>\n", argv[0]);
    return 1;
  }

  char *romFile = argv[1];
  int scale = atoi(argv[2]);
  int delay = atoi(argv[3]);

  Chip8 vm;
  chip8_init(&vm);
  chip8_load_rom(&vm, romFile);

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  SDL_Texture *texture = NULL;
  window = SDL_CreateWindow("Chippy", DISPLAY_W * scale, DISPLAY_H * scale,
                            SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, NULL);
  texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_STREAMING, DISPLAY_W, DISPLAY_H);
  if (!window) {
    printf("Window has error: %s\n", SDL_GetError());
    return 1;
  }
  if (!renderer) {
    printf("Renderer has error: %s\n", SDL_GetError());
    return 1;
  }
  if (!texture) {
    printf("Texture has error: %s\n", SDL_GetError());
    return 1;
  }
  int pitch = sizeof(vm.display[0]) * DISPLAY_W;
  while ((bool)true) {
    chip8_cycle(&vm);
    update_screen(renderer, texture, vm.display, pitch);
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
// pitch is one row of the display, use vm->display[0]*DISPLAY_W
void update_screen(SDL_Renderer *renderer, SDL_Texture *texture,
                   void const *display, int pitch) {
  SDL_UpdateTexture(texture, NULL, display, pitch);
  SDL_RenderClear(renderer);
  SDL_RenderTexture(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void chip8_init(Chip8 *vm) {
  memset(vm, 0, sizeof(Chip8));
  vm->PC = 0x200;
  memcpy(vm->memory, FONT, sizeof(FONT));
}

void chip8_load_rom(Chip8 *vm, const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("faah thers no file lmao");
    exit(1);
  }

  fread(&vm->memory[0x200], 1, MEMORY_SIZE - 0x200, f);
  fclose(f);
}

void chip8_cycle(Chip8 *vm) {
  vm->opcode = (vm->memory[vm->PC] << 8) | vm->memory[vm->PC + 1];
  vm->PC += 2;
  uint8_t X = (vm->opcode & 0x0F00) >> 8;
  uint8_t Y = (vm->opcode & 0x00F0) >> 4;
  uint8_t N = (vm->opcode & 0x000F);
  uint8_t NN = (vm->opcode & 0x00FF);
  uint16_t NNN = (vm->opcode & 0x0FFF);

  switch (vm->opcode & 0xF000) {
  case 0x0000:
    switch (vm->opcode & 0x00FF) {
    // CLS
    case 0x00E0:
      memset(vm->display, 0, sizeof(vm->display));
      break;
    // RET
    case 0x00EE:
      --vm->SP;
      vm->PC = vm->stack[vm->SP];
      break;
    }
    break;
  // JP addr
  case 0x1000:
    vm->PC = NNN;
    break;
  // CALL addr
  case 0x2000:
    vm->stack[vm->SP] = vm->PC;
    ++vm->SP;
    vm->PC = NNN;
    break;
  // LD Vx
  case 0x6000:
    vm->V[X] = NN;
    break;
  // ADD Vx
  case 0x7000:
    vm->V[X] += NN;
    break;
  // LD I addr
  case 0xA000:
    vm->I = NNN;
    break;
  // DXYN
  case 0xD000:
    uint8_t x_cord = vm->V[X] % DISPLAY_W;
    uint8_t y_cord = vm->V[Y] % DISPLAY_H;
    vm->V[0xF] = 0;
    for (int row = 0; row < N; row++) {
      uint8_t sprite_data = vm->memory[vm->I + row];
      for (int col = 0; col < 8; col++) {
        uint8_t pixel = sprite_data & (0x80 >> col);
        uint32_t *screenpixel =
            &vm->display[(y_cord + row) * DISPLAY_W + (x_cord + col)];

        if (pixel) {
          if (*screenpixel == 0xFFFFFFFF) {
            vm->V[0xF] = 1;
          }
          *screenpixel ^= 0xFFFFFFFF;
        }
      }
    }
    break;
  }

  if (vm->delay_timer > 0) {
    vm->delay_timer--;
  }
  if (vm->sound_timer > 0) {
    vm->sound_timer--;
  }
}

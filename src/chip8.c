#include "chip8.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_oldnames.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
  srand(time(NULL));
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
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART);
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
  uint32_t lastTime = SDL_GetTicks();
  bool stop = false;
  while (!stop) {
    stop = process_keypress(vm.keys);
    uint32_t currentTime = SDL_GetTicks();
    uint32_t timeChange = timeChange + currentTime - lastTime;
    lastTime = currentTime;
    if (timeChange > 16.67 && vm.delay_timer > 0) {
      timeChange = 0;
      vm.delay_timer--;
    }
    if (timeChange > 16.67 && vm.sound_timer > 0) {
      timeChange = 0;
      vm.sound_timer--;
    }
    SDL_Delay(delay);
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
  uint8_t flag = 0;

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
  // 3XNN - Skip conditionally (3, 4, 5, 9)
  case 0x3000:
    if (vm->V[X] == NN) {
      vm->PC += 2;
    }
    break;
  // 4XNN
  case 0x4000:
    if (vm->V[X] != NN) {
      vm->PC += 2;
    }
    break;
  // 5XY0
  case 0x5000:
    if (vm->V[X] == vm->V[Y]) {
      vm->PC += 2;
    }
    break;
  // 9XYO
  case 0x9000:
    if (vm->V[X] != vm->V[Y]) {
      vm->PC += 2;
    }
    break;
  // LD Vx
  case 0x6000:
    vm->V[X] = NN;
    break;
  // ADD Vx
  case 0x7000:
    vm->V[X] += NN;
    break;
  // catch 8XY0, 1, 2, 3, 4, 5, 6, 7, E
  case 0x8000:
    switch (vm->opcode & 0x000F) {
    // 8XY0
    case 0x0000:
      vm->V[X] = vm->V[Y];
      break;
    // 8XY1
    case 0x0001:
      vm->V[X] = vm->V[X] | vm->V[Y];
      break;
    // 8XY2
    case 0x0002:
      vm->V[X] = vm->V[X] & vm->V[Y];
      break;
    // 8XY3
    case 0x0003:
      vm->V[X] = vm->V[X] ^ vm->V[Y];
      break;
    // 8XY4
    case 0x0004:
      if (vm->V[X] + vm->V[Y] > 0xFF) {
        flag = 1;
      } else {
        flag = 0;
      }
      vm->V[X] = vm->V[X] + vm->V[Y];
      vm->V[0xF] = flag;
      break;
    // 8XY5
    case 0x0005:
      if (vm->V[X] >= vm->V[Y]) {
        flag = 1;
      } else {
        flag = 0;
      }
      vm->V[X] = vm->V[X] - vm->V[Y];
      vm->V[0xF] = flag;
      break;
    // 8XY7
    case 0x0007:
      if (vm->V[Y] >= vm->V[X]) {
        flag = 1;
      } else {
        flag = 0;
      }
      vm->V[X] = vm->V[Y] - vm->V[X];
      vm->V[0xF] = flag;
      break;
    // 8XY6
    case 0x0006:
      flag = 0;
      flag = 0x1 & vm->V[X];
      vm->V[X] = vm->V[X] >> 1;
      vm->V[0xF] = flag;
      break;
    // 8XYE
    case 0x000E:
      flag = 0;
      flag = 0x80 & vm->V[X];
      flag = flag >> 7;
      vm->V[X] = vm->V[X] << 1;
      vm->V[0xF] = flag;
    }
    break;
  // LD I addr
  case 0xA000:
    vm->I = NNN;
    break;
  // BNNN
  case 0xB000:
    vm->PC = NNN + vm->V[0];
    break;
  // CXNN
  case 0xC000:
    vm->V[X] = rand() & NN;
    break;
  // EX9E and EXA1
  case 0xE000:
    switch (vm->opcode & 0x00F0) {
    case 0x0090:
      if (vm->keys[vm->V[X]] == 1) {
        vm->PC += 2;
      }
      break;
    case 0x00A0:
      if (vm->keys[vm->V[X]] == 0) {
        vm->PC += 2;
      }
      break;
    }
    break;
  // FX07, FX15, FX18, FX1E, FX0A, FX29, FX33, FX55, FX65
  case 0xF000:
    switch (vm->opcode & 0x00FF) {
    case 0x0007:
      vm->V[X] = vm->delay_timer;
      break;
    case 0x0015:
      vm->delay_timer = vm->V[X];
      break;
    case 0x0018:
      vm->sound_timer = vm->V[X];
      break;
    case 0x001E:
      if (vm->I + vm->V[X] > 0xFF) {
        vm->I += vm->V[X];
        vm->V[0xF] = 1;
      } else {
        vm->I += vm->V[X];
      }
      break;
    case 0x000A:
      if (process_keypress(vm->keys)) {
        vm->PC += 2;
      } else {
        vm->PC -= 2;
      }
      break;
    case 0x0029:
      vm->I = vm->V[X] * sizeof(FONT[0]);
      break;
    case 0x0033:
      vm->memory[vm->I] = vm->V[X] / 100;
      vm->memory[vm->I + 1] = (vm->V[X] / 10) % 10;
      vm->memory[vm->I + 2] = vm->V[X] % 10;
      break;

    case 0x0055:
      for (uint8_t i = 0; i <= X; i++) {
        vm->memory[vm->I + i] = vm->V[i];
      }
      break;
    case 0x0065:
      for (uint8_t i = 0; i <= X; i++) {
        vm->V[i] = vm->memory[vm->I + i];
      }
      break;
    }
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
}

bool process_keypress(uint8_t *keys) {
  bool stop = false;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      stop = true;
    } else if (event.type == SDL_EVENT_KEY_DOWN) {
      switch (event.key.scancode) {
      case SDL_SCANCODE_ESCAPE:
        stop = true;
        break;
      case SDL_SCANCODE_1:
        keys[0] = 1;
        break;
      case SDL_SCANCODE_2:
        keys[1] = 1;
        break;
      case SDL_SCANCODE_3:
        keys[2] = 1;
        break;
      case SDL_SCANCODE_4:
        keys[3] = 1;
        break;
      case SDL_SCANCODE_Q:
        keys[4] = 1;
        break;
      case SDL_SCANCODE_W:
        keys[5] = 1;
        break;
      case SDL_SCANCODE_E:
        keys[6] = 1;
        break;
      case SDL_SCANCODE_R:
        keys[7] = 1;
        break;
      case SDL_SCANCODE_A:
        keys[8] = 1;
        break;
      case SDL_SCANCODE_S:
        keys[9] = 1;
        break;
      case SDL_SCANCODE_D:
        keys[10] = 1;
        break;
      case SDL_SCANCODE_F:
        keys[11] = 1;
        break;
      case SDL_SCANCODE_Z:
        keys[12] = 1;
        break;
      case SDL_SCANCODE_X:
        keys[13] = 1;
        break;
      case SDL_SCANCODE_C:
        keys[14] = 1;
        break;
      case SDL_SCANCODE_V:
        keys[15] = 1;
        break;
      default:
        break;
      }
    } else if (event.type == SDL_EVENT_KEY_UP) {
      switch (event.key.scancode) {
      case SDL_SCANCODE_1:
        keys[0] = 0;
        break;
      case SDL_SCANCODE_2:
        keys[1] = 0;
        break;
      case SDL_SCANCODE_3:
        keys[2] = 0;
        break;
      case SDL_SCANCODE_4:
        keys[3] = 0;
        break;
      case SDL_SCANCODE_Q:
        keys[4] = 0;
        break;
      case SDL_SCANCODE_W:
        keys[5] = 0;
        break;
      case SDL_SCANCODE_E:
        keys[6] = 0;
        break;
      case SDL_SCANCODE_R:
        keys[7] = 0;
        break;
      case SDL_SCANCODE_A:
        keys[8] = 0;
        break;
      case SDL_SCANCODE_S:
        keys[9] = 0;
        break;
      case SDL_SCANCODE_D:
        keys[10] = 0;
        break;
      case SDL_SCANCODE_F:
        keys[11] = 0;
        break;
      case SDL_SCANCODE_Z:
        keys[12] = 0;
        break;
      case SDL_SCANCODE_X:
        keys[13] = 0;
        break;
      case SDL_SCANCODE_C:
        keys[14] = 0;
        break;
      case SDL_SCANCODE_V:
        keys[15] = 0;
        break;
      default:
        break;
      }
    }
  }
  return stop;
}

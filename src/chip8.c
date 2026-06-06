#include "chip8.h"
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

int main() { return 0; }

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
      memset(vm->memory, 0, sizeof(vm->memory));
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
}

#pragma once
#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#define MEMORY_SIZE 4096
#define DISPLAY_W 64
#define DISPLAY_H 32
#define STACK_SIZE 16
#define NUM_REGS 16

typedef struct {
  uint8_t memory[MEMORY_SIZE];
  uint8_t V[NUM_REGS];
  uint16_t PC;
  uint16_t I;
  uint16_t stack[STACK_SIZE];
  uint8_t SP;
  uint8_t delay_timer;
  uint8_t sound_timer;
  uint32_t display[DISPLAY_W * DISPLAY_H];
  uint8_t keys[16];
  uint16_t opcode;
} Chip8;

bool process_keypress(uint8_t *keys);
void update_screen(SDL_Renderer *renderer, SDL_Texture *texture,
                   void const *display, int pitch);
void chip8_init(Chip8 *vm);
void chip8_load_rom(Chip8 *vm, const char *path);
void chip8_cycle(Chip8 *vm);

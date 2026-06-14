# chippy
## Description
This is a Chip-8 emulator that I wrote in C to learn more about emulation and about C as well, since basically all of my previous programming experience has been in Python and Java, and I wanted to learn C. Written on Arch Linux, you might need to install SDL to run it, I'm not sure. Made for #horizons at Hack Club.
## Screenshots
<img width="1552" height="865" alt="Screenshot_20260613_220055" src="https://github.com/user-attachments/assets/afeb0015-f063-4e8d-b54b-65abd5aa1677" />
<img width="1549" height="754" alt="Screenshot_20260613_220131" src="https://github.com/user-attachments/assets/921321c5-6abb-40cc-96e1-c23bcf2ef131" />

## Tech Stack
I used SDL3 to render everything, and the logic and all is written in C.
## Motivation
I wanted to make this because I like emulation, and my eventual goal is to write a GPU Emulator in C, and this project was to learn more about how emulators and C work.
## How does it work?
I implemented all the instructions for the emulator based on Tobias V. Langhoff's guide (https://tobiasvl.github.io/blog/write-a-chip-8-emulator/), and the logic works how he described it. SDL is used to show the display, and the program runs by executing each instruction, and continuing based on what the instruction has it do. I tested them with test ROMs that are also in the roms folder.

## How to run
1. Download the AppImage
2. Make the file executable (chmod +x)
3. Run the AppImage (Usage: ./Chippy-x86_64.AppImage <ROM> <Scale> <Delay>)

## AI Usage
I used AI in this project to help me with fixing some bugs in the project, namely, the keypad not mapping correctly, and small errors in some instructions.

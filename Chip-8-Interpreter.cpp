/*  
    Manveer Dhanjal
    Description:
    Chip-8 works by using nibbles to jump from memory location to memory location
    X: The second nibble. Used to look up one of the 16 registers (VX) from V0 through VF.
    Y: The third nibble. Also used to look up one of the 16 registers (VY) from V0 through VF.
    N: The fourth nibble. A 4-bit number.
    NN: The second byte (third and fourth nibbles). An 8-bit immediate number.
    NNN: The second, third and fourth nibbles. A 12-bit immediate memory address.
*/
#include <iostream>
#include <cstdint> //used to guarntee that the computer architecture does not matter. gives a fixed interger width
#include <cstring> //Needed for memset
#include <SDL2/SDL.h> // used to display graphics
#include <fstream> //used to read rom files
//TODO Havent decided on what graphic library to use yet
class Chip8{
    public:
    std::uint8_t memory[4096]; //memory needed to run emulator (4kb)
    std::uint32_t Display[64 * 32]; //displays the emulator running
    std::uint16_t PC; //points to current memory instruction
    std::uint16_t I; //points to location in memory or just an address pointer 
    std::uint16_t Stack[16]; //used for subroutines and functions
    std::uint8_t stack_pointer; //points to the current stack
    std::uint8_t delay_timer; //decrements a 60hz (60 times per second) until it hits 0
    std::uint8_t sound_timer; //functions like the delay_timer but gives a beeping sound until timer hits 0
    std::uint8_t V[16]; //chip-8 emulators have 8 byte registers and there are 16 of them, they go from 0x00 to 0xFF or V0 to VF
    std::uint16_t opcode;
    std::uint8_t keypad[16];
    /*
    A keypad that chip-8 emulators use.
    1	2	3	4
    Q	W	E	R
    A	S	D	F
    Z	X	C	V
    how the implementation will look
    */
   std::uint8_t fontset[80] = {
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
    //Important notes: Roms will all start from memory location 0x200 and end at 0x
    void OP_00E0()//Clears the display
    {
    memset(Display,0,sizeof(Display));
    }
    void OP_1NNN(){ //jumps to new memory location
        PC = opcode & 0x0FFF;
        /*
            for this code its jumping to memory location by doing this.
            example: opcode = 0x1234
            what this does is 
            0001 0010 0011 0100
           +0000 1111 1111 1111 
        */
    }
    void OP_2NNN(){
        Stack[stack_pointer] = PC;
        ++stack_pointer;
        PC = opcode & 0x0FFF;
        //this opcode instrcution stores the current program counter in the stack for a temporary jump to the address NNN
    }
    void OP_6XNN()//sets register VX
    {
        std::uint8_t X = (opcode & 0x0F00) >> 8; // we shift over 8 bits so we get where X is stored (we want 0x0A not 0x0A00 as our register)
        //The & operation compares the two bits and sees if they are one or see in the second 4 bit space
        std::uint8_t NN = (opcode & 0x00FF); // We get the value we are storing in v[x]
        V[X] = NN;
    } 
    void OP_DXYN(){
        uint16_t X = V[(opcode & 0x0F00) >> 8]; //Fetchs the position of the sprite
        uint16_t Y = V[(opcode & 0x00F0) >> 4]; 
        uint16_t height = opcode & 0x000F; //is the pixel value
        uint16_t pixel;
        V[0xF] = 0; //resets collision flag
        for (int yline = 0; yline < height; yline++){
            pixel = memory[I + yline]; //fetches the sprite from memory
            for(int xline = 0; xline < 8; xline++){
                if((pixel & (0x80 >> xline)) != 0){ //checks if the pixel is set to 1
                    if(Display[(X + xline + ((Y + yline) * 64))] == 1){
                        V[0xF] = 1; //sets collision flag
                    }
                    Display[X + xline + ((Y + yline) * 64)] ^= 1; //draws the sprite to the display
                }
            }
        }
    }
    void OP_ANNN(){
        I = opcode & 0x0FFF; //Sets index register to Address NNN
    }
    void OP_7XNN(){
        uint16_t X = (opcode & 0x0F00) >> 8; //Value of X
        uint16_t NN = opcode & 0x00FF; // Value of NN
        V[X] += NN; //Adds NN to VX (carry flag needs to be changed if there is a carry)
        //TODO: Implement carry flag
    }
    //The functions below will not be written inline with the class
    void initialize();
    void cycle();
    void LoadROM(const char* filename);
};
void Chip8::cycle(){
    opcode = (memory[PC] << 8u) | memory[PC + 1];
    PC += 2;
    switch(opcode & 0xF000){
        case 0x1000:
            OP_1NNN();
            break;
        case 0x2000:
            OP_2NNN();
            break;
        case 0x6000:
            OP_6XNN();
            break;
        case 0xD000:
            OP_DXYN();
            break;
        case 0xA000:
            OP_ANNN();
            break;
        case 0x7000:
            OP_7XNN();
            break;
        default:
        std::cout << "Opcode switch missing\n";
    }
    if(delay_timer > 0){
        --delay_timer;
    }
    if(sound_timer > 0){
        if(sound_timer == 1){
            std::cout << "DONE\n";
            --sound_timer;
            }
        }
}

void Chip8::initialize(){
    PC = 0x200; //PC starts at 0x200
    opcode = 0; //start state
    I = 0;
    stack_pointer = 0;
    OP_00E0(); //clears display to be restarted

    for(auto i = 0; i<16;i++)
    {
        Stack[i] = 0; //clear stack
    }
    for(auto i = 0; i<16;i++)
    {
        V[i] = 0; //clear register V0-VF
    }
    for(auto i = 0; i<4096;i++)
    {
        memory[i] = 0; //clear memory
    }

    for (auto i = 0;i<80;++i){
        memory[i] = fontset[i]; //loads the fontset into memory
    }
}
void Chip8::LoadROM(const char* filename){
    std::ifstream file(filename, std::ios::binary | std::ios::ate); //reads the binary file
    if(file.is_open()){
        std::streampos size = file.tellg(); //gets the size of the file
        char* buffer = new char[size]; //creates a buffer to hold the file data
        file.seekg(0, std::ios::beg); //moves the file pointer to the beginning of the file
        file.read(buffer, size); //reads the file data into the buffer
        file.close(); //closes the file

        for(int i = 0; i < size; i++){
            memory[0x200 + i] = buffer[i]; //loads the rom into memory starting at 0x200
        }
        delete[] buffer; //frees the buffer memory
    }
    else{
        std::cout << "Failed to open ROM\n";
    }
}
Chip8 MainChip8;

int main(int argc, char* argv[]){


    SDL_Init(SDL_INIT_VIDEO); //initializes SDL2 library

    SDL_Window* window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, 0);
    //creates a window called chip-8 emulator that is scaled 64*10 by 32*10
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0); //creates a renderer to render the graphics to the window
    SDL_Texture* texture = SDL_CreateTexture(
        renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        64, 32); 
        //creates a texture to render the display to the window

    MainChip8.initialize();
    MainChip8.LoadROM(argv[1]);

    bool running = true;
    while(running){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT){
                running = false;
            }
        }
        for(int i =0;i<10;i++){
            MainChip8.cycle();
        }
        uint32_t pixels[64 * 32];
        for(int i = 0; i < 64 * 32; i++){
            pixels[i] = (MainChip8.Display[i] == 1) ? 0xFFFFFFFF : 0xFF000000; //sets the pixel to white if its on and black if its off
        }
        SDL_UpdateTexture(texture, NULL, pixels, 64 * sizeof(uint32_t)); //updates the texture with the new pixel data
        SDL_RenderClear(renderer); //clears the renderer
        SDL_RenderCopy(renderer, texture, NULL, NULL); //copies the texture to the renderer
        SDL_RenderPresent(renderer); //presents the renderer to the window
        SDL_Delay(16); //delays the loop to run at approximately 60 frames per second
    }


    return 0;
}

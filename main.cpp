#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdio>
#include "../teenyat.h"

using namespace std;

const tny_uword SNIFF_NEAR_FOOD = 0x9000; //sniff nearest food source, 8 cell radius
const tny_uword SNIFF_NEAR_PHER = 0x9001; //sniff nearest pheremone signal, 8 cell radius
const tny_uword SNIFF_STRONG_PHER = 0x9002; //sniff strongest pheremone signal, 8 cell radius
const tny_uword SNIFF_PHER_DIR = 0x9003; // sniff pheremone in direction (no radius)
const tny_uword DROP_PHER = 0x9005; //lay down a fresh pheremone signal
const tny_uword MOVE = 0x9006; //move in cardinal and subcardinal direction
const tny_uword SET_SNIFF_DIR =0x9007;//set sniff direction for long range
typedef struct {
    teenyat *t;
    short x;
    short y;
    short dir;
    char r; //rgba values for color
    char g;
    char b;
    char a;
} tnyant;

typedef struct {
    short pher_level;
    int food;
    short ant_pres;
} tnycell;

void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay);

tnycell tnymap[128][128];

vector<tnyant> ant_list;
int num_ant = 0;

void bus_read(tnyant *t, tny_uword addr, tny_word data, uint16_t *delay) {
    switch (addr)
    {
    case SNIFF_NEAR_FOOD:
        /* code */
        break;
    
    default:
        break;
    }
}

void bus_write(tnyant *t, tny_uword addr, tny_word data, uint16_t *delay) {
    switch (addr)
    {
    case MOVE:
        if(tnymap[ant_list[num_ant].x][ant_list[num_ant].y].ant_pres){tnymap[ant_list[num_ant].x][ant_list[num_ant].y].ant_pres--;}
        ant_list[num_ant].x = (ant_list[num_ant].x + (short) data.bytes.byte0-0x80)%128;
        ant_list[num_ant].y = (ant_list[num_ant].y + (short) data.bytes.byte1-0x80)%128;
        tnymap[ant_list[num_ant].x][ant_list[num_ant].y].ant_pres++;
        break;
    case DROP_PHER:
        tnymap[ant_list[num_ant].x][ant_list[num_ant].y].pher_level = data.bytes.byte0;
    default:
        break;
    }
}

int main(int argc, char *argv[]) {
    int ant_num =0;
    if (argc%2-1) {
        std::cout << "Please provide a binary file and number for each file" << std::endl;
        return 1;
    }
    for (int i =2;i<argc; i+=2){
        FILE *bin_file = fopen(argv[i-1], "rb");
        if(bin_file !=NULL){
            for(int j = 0;j<(int) argv[i]; j++){
                ant_list.push_back(tnyant());
                tny_init_from_file(ant_list[ant_num++].t,bin_file, bus_read, bus_write);
            }
           fclose(bin_file);
        } else {
            cout << "err binary file path";
        }
    }
}
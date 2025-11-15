#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdio>
#include "../teenyat.h"

using namespace std;

const tny_uword SNIFF_NEAR_FOOD = 0x9000; //sniff nearest food source
const tny_uword SNIFF_NEAR_PHER = 0x9001; //sniff nearest pheremone signal
const tny_uword SNIFF_STRONG_PHER = 0x9002; //sniff strongest pheremone signal
const tny_uword SNIFF_PHER_DIR = 0x9003; // sniff pheremone in direction
const tny_uword DROP_PHER = 0x9005; //lay down a fresh pheremone signal
const tny_uword MOVE = 0x9006; //move in cardinal and subcardinal direction

typedef struct {
    teenyat *t;
    short x;
    short y;
} TnyAnt;



void ant_bus_read(teenyat *t, tny_uword addr, tny_uword *data, uint16_t *delay) {

}

void ant_bus_write(teenyat *t, tny_uword addr, tny_uword *data, uint16_t *delay) {
    switch (addr)
    {
    case MOVE:
        
        break;
    
    default:
        break;
    }
}

int main(int argc, char *argv[]) {
   
}
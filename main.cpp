#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdio>
#include "../teenyat.h"

using namespace std;

const tny_uword SNIFF_NEAR_FOOD = 0x9000; //sniff nearest food source, 2 cell radius
const tny_uword SNIFF_NEAR_PHER = 0x9001; //sniff nearest pheremone signal, 6 cell radius
const tny_uword SNIFF_STRONG_PHER = 0x9002; //sniff strongest pheremone signal, 6 cell radius
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

void bus_read(tnyant *t, tny_uword addr, tny_word *data, uint16_t *delay) {
    switch (addr)
    {
    case SNIFF_NEAR_FOOD:
        tny_word food;
        food.bytes.byte0 =100;
        food.bytes.byte1 =100;
        for (int i = -4;i <5; i++) {
            for (int j =-4;j<5;j++) {
                if (tnymap[(ant_list[num_ant].x+i)%128][(ant_list[num_ant].y+j)%128].food >0 && (food.bytes.byte0-4)*(food.bytes.byte0-4)+(food.bytes.byte1-4)*(food.bytes.byte1-4) > i*1+j*j){
                    food.bytes.byte0 = i+4;
                    food.bytes.byte1 = j+4;    
                }
            }
        }
        data->bytes.byte0 = food.bytes.byte0;
        data->bytes.byte1 = food.bytes.byte1;
        break;
    case SNIFF_NEAR_PHER:
        tny_word food;
        food.bytes.byte0 =100;
        food.bytes.byte1 =100;
        for (int i = -8;i <9; i++) {
            for (int j =-8;j<9;j++) {
                if (tnymap[(ant_list[num_ant].x+i)%128][(ant_list[num_ant].y+j)%128].pher_level >0 && (food.bytes.byte0-4)*(food.bytes.byte0-4)+(food.bytes.byte1-4)*(food.bytes.byte1-4) > i*1+j*j){
                    food.bytes.byte0 = i+8;
                    food.bytes.byte1 = j+8;    
                }
            }
        }
        data->bytes.byte0 = food.bytes.byte0;
        data->bytes.byte1 = food.bytes.byte1;
        break;
    case SNIFF_STRONG_PHER:
        tny_word food;
        food.bytes.byte0 =100;
        food.bytes.byte1 =100;
        int pher_level =0;
        for (int i = -8;i <9; i++) {
            for (int j =-8;j<9;j++) {
                if (tnymap[(ant_list[num_ant].x+i)%128][(ant_list[num_ant].y+j)%128].pher_level > pher_level){
                    food.bytes.byte0 = i+8;
                    food.bytes.byte1 = j+8;
                    pher_level = tnymap[(ant_list[num_ant].x+i)%128][(ant_list[num_ant].y+j)%128].pher_level;
                }
            }
        }
        data->bytes.byte0 = food.bytes.byte0;
        data->bytes.byte1 = food.bytes.byte1;
        break;
    case SNIFF_PHER_DIR:
        tny_word pher;
        pher.bytes.byte0 = 100;
        pher.bytes.byte1 = 0;
        switch (ant_list[num_ant].dir)
        {
        case 1:
            for (int i=0;i<32;i++) {
                if (tnymap[(ant_list[num_ant].x+i)%128][ant_list[num_ant].y].pher_level > pher.bytes.byte1){
                    pher.bytes.byte1 = tnymap[(ant_list[num_ant].x+i)%128][ant_list[num_ant].y].pher_level;
                    pher.bytes.byte0 = i;
                }
            }    
            break;
        case 2:
            for (int i=0;i<32;i++) {
                if (tnymap[ant_list[num_ant].x][(ant_list[num_ant].y+i)%128].pher_level > pher.bytes.byte1){
                    pher.bytes.byte1 = tnymap[(ant_list[num_ant].x+i)%128][ant_list[num_ant].y].pher_level;
                    pher.bytes.byte0 = i;
                }
            }    
            break;
        case 3:
            for (int i=0;i<32;i++) {
                if (tnymap[(ant_list[num_ant].x-i)%128][ant_list[num_ant].y].pher_level > pher.bytes.byte1){
                    pher.bytes.byte1 = tnymap[(ant_list[num_ant].x+i)%128][ant_list[num_ant].y].pher_level;
                    pher.bytes.byte0 = i;
                }
            }    
            break;
        case 4:
            for (int i=0;i<32;i++) {
                if (tnymap[ant_list[num_ant].x][(ant_list[num_ant].y-i)%128].pher_level > pher.bytes.byte1){
                    pher.bytes.byte1 = tnymap[(ant_list[num_ant].x+i)%128][ant_list[num_ant].y].pher_level;
                    pher.bytes.byte0 = i;
                }
            }    
            break;
        default:
            break;
        }
        data->bytes.byte0 = pher.bytes.byte0;
        data->bytes.byte1 = pher.bytes.byte1;
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
    case SET_SNIFF_DIR:
        ant_list[num_ant].dir = data.u;
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
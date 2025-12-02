#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdio>
#include <thread>
#include <chrono>
#include "../teenyat.h"
#include "graphics.h"

using namespace std;

const tny_uword SNIFF_NEAR_FOOD = 0x9000; //sniff nearest food source, 2 cell radius
const tny_uword SNIFF_NEAR_PHER = 0x9001; //sniff nearest pheremone signal, 6 cell radius
const tny_uword SNIFF_STRONG_PHER = 0x9002; //sniff strongest pheremone signal, 6 cell radius
const tny_uword SNIFF_PHER_DIR = 0x9003; // sniff pheremone in direction (no radius)
const tny_uword DROP_PHER = 0x9005; //lay down a fresh pheremone signal
const tny_uword MOVE = 0x9006; //move in cardinal and subcardinal direction
const tny_uword SET_SNIFF_DIR =0x9007;//set sniff direction for long range
const tny_uword SET_RG = 0x9008; //sets R and G
const tny_uword SET_BA = 0x9009; //sets B and A


typedef struct {
    teenyat *t;
    short x;
    short y;
    short dir;
    char r; //rgba values for color
    char g;
    char b;
    char a;
    bool carrying_food;  // Added for graphics
    int state;          // Added for graphics
} tnyant;

typedef struct {
    short pher_level;
    int food;
    short ant_pres;
    short nest;
} tnycell;

void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay);

tnycell tnymap[128][128];

vector<tnyant> ant_list;
int num_ant = 0;

// Fixed function signature - only change needed
void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay) {
    *delay = 1; // Added delay setting
    switch (addr)
    {
    case SNIFF_NEAR_FOOD:
        {
            tny_word food;
            food.bytes.byte0 =100;
            food.bytes.byte1 =100;
            for (int i = -4;i <5; i++) {
                for (int j =-4;j<5;j++) {
                    if (tnymap[(ant_list[num_ant].x+i+128)%128][(ant_list[num_ant].y+j+128)%128].food >0 && (food.bytes.byte0-4)*(food.bytes.byte0-4)+(food.bytes.byte1-4)*(food.bytes.byte1-4) > i*i+j*j){
                        food.bytes.byte0 = i+4;
                        food.bytes.byte1 = j+4;    
                    }
                }
            }
            
            data->bytes.byte0 = food.bytes.byte0;
            data->bytes.byte1 = food.bytes.byte1;
        }
        break;
    case SNIFF_NEAR_PHER:
        {
            tny_word pher; // Fixed variable name
            pher.bytes.byte0 =100;
            pher.bytes.byte1 =100;
            for (int i = -8;i <9; i++) {
                for (int j =-8;j<9;j++) {
                    if (tnymap[(ant_list[num_ant].x+i+128)%128][(ant_list[num_ant].y+j+128)%128].pher_level >0 && (pher.bytes.byte0-8)*(pher.bytes.byte0-8)+(pher.bytes.byte1-8)*(pher.bytes.byte1-8) > i*i+j*j){
                        pher.bytes.byte0 = i+8;
                        pher.bytes.byte1 = j+8;    
                    }
                }
            }
            data->bytes.byte0 = pher.bytes.byte0;
            data->bytes.byte1 = pher.bytes.byte1;
        }
        break;
    case SNIFF_STRONG_PHER:
        {
            tny_word strong; // Fixed variable name
            strong.bytes.byte0 =100;
            strong.bytes.byte1 =100;
            int pher_level =0;
            for (int i = -8;i <9; i++) {
                for (int j =-8;j<9;j++) {
                    if (tnymap[(ant_list[num_ant].x+i+128)%128][(ant_list[num_ant].y+j+128)%128].pher_level > pher_level){
                        strong.bytes.byte0 = i+8;
                        strong.bytes.byte1 = j+8;
                        pher_level = tnymap[(ant_list[num_ant].x+i+128)%128][(ant_list[num_ant].y+j+128)%128].pher_level;
                    }
                }
            }
            data->bytes.byte0 = strong.bytes.byte0;
            data->bytes.byte1 = strong.bytes.byte1;
        }
        break;
    case SNIFF_PHER_DIR:
        {
            tny_word pher_dir;
            pher_dir.bytes.byte0 = 100;
            pher_dir.bytes.byte1 = 0;
            switch (ant_list[num_ant].dir)
            {
            case 1:
                for (int i=0;i<32;i++) {
                    if (tnymap[(ant_list[num_ant].x+i)%128][ant_list[num_ant].y].pher_level > pher_dir.bytes.byte1){
                        pher_dir.bytes.byte1 = tnymap[(ant_list[num_ant].x+i)%128][ant_list[num_ant].y].pher_level;
                        pher_dir.bytes.byte0 = i;
                    }
                }    
                break;
            case 2:
                for (int i=0;i<32;i++) {
                    if (tnymap[ant_list[num_ant].x][(ant_list[num_ant].y+i)%128].pher_level > pher_dir.bytes.byte1){
                        pher_dir.bytes.byte1 = tnymap[ant_list[num_ant].x][(ant_list[num_ant].y+i)%128].pher_level; // Fixed array access
                        pher_dir.bytes.byte0 = i;
                    }
                }    
                break;
            case 3:
                for (int i=0;i<32;i++) {
                    if (tnymap[(ant_list[num_ant].x-i+128)%128][ant_list[num_ant].y].pher_level > pher_dir.bytes.byte1){
                        pher_dir.bytes.byte1 = tnymap[(ant_list[num_ant].x-i+128)%128][ant_list[num_ant].y].pher_level; // Fixed array access
                        pher_dir.bytes.byte0 = i;
                    }
                }    
                break;
            case 4:
                for (int i=0;i<32;i++) {
                    if (tnymap[ant_list[num_ant].x][(ant_list[num_ant].y-i+128)%128].pher_level > pher_dir.bytes.byte1){
                        pher_dir.bytes.byte1 = tnymap[ant_list[num_ant].x][(ant_list[num_ant].y-i+128)%128].pher_level; // Fixed array access
                        pher_dir.bytes.byte0 = i;
                    }
                }    
                break;
            default:
                break;
            }
            data->bytes.byte0 = pher_dir.bytes.byte0;
            data->bytes.byte1 = pher_dir.bytes.byte1;
        }
        break; // Added missing break
    default:
        data->u = 0; // Added default case
        break;
    }
}

// Fixed function signature - only change needed
void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay) {
    *delay = 1; // Added delay setting
    switch (addr)
    {
    case MOVE:
        {
            if(tnymap[ant_list[num_ant].x][ant_list[num_ant].y].ant_pres){tnymap[ant_list[num_ant].x][ant_list[num_ant].y].ant_pres--;}
            
            short old_x = ant_list[num_ant].x; // Added for graphics
            short old_y = ant_list[num_ant].y; // Added for graphics
            
            ant_list[num_ant].x = (ant_list[num_ant].x + (short) data.bytes.byte0-0x80+128)%128; // Fixed bounds
            ant_list[num_ant].y = (ant_list[num_ant].y + (short) data.bytes.byte1-0x80+128)%128; // Fixed bounds
            tnymap[ant_list[num_ant].x][ant_list[num_ant].y].ant_pres++;
            
            // Added food collection for graphics
            if (tnymap[ant_list[num_ant].x][ant_list[num_ant].y].food > 0) {
                tnymap[ant_list[num_ant].x][ant_list[num_ant].y].food--;
                ant_list[num_ant].carrying_food = true;
                ant_list[num_ant].state = 2;
                ant_list[num_ant].r = 255; ant_list[num_ant].g = 215; ant_list[num_ant].b = 0;
                cout << "Ant " << num_ant << " found food!" << endl;
            }
            
            cout << "Ant " << num_ant << " moved from (" << old_x << "," << old_y << ") to (" << ant_list[num_ant].x << "," << ant_list[num_ant].y << ")" << endl;
        }
        break;
    case DROP_PHER:
        tnymap[ant_list[num_ant].x][ant_list[num_ant].y].pher_level = data.bytes.byte0;
        cout << "Ant " << num_ant << " dropped pheromone level " << (int)data.bytes.byte0 << endl;
        break; // Added missing break
    case SET_SNIFF_DIR:
        ant_list[num_ant].dir = data.u;
        break; // Added missing break
    default:
        break;
    }
}

// Added graphics functions
void update_graphics_state() {
    // Sync food from graphics to simulation
    for (int x = 0; x < 128; x++) {
        for (int y = 0; y < 128; y++) {
            tnymap[x][y].food = 0;
        }
    }
    
    for (int gx = 0; gx < 50; gx++) {
        for (int gy = 0; gy < 50; gy++) {
            if (get_world_cell(gx, gy) == 1) {
                int sim_x = (gx * 128) / 50;
                int sim_y = (gy * 128) / 50;
                for (int dx = 0; dx < 3; dx++) {
                    for (int dy = 0; dy < 3; dy++) {
                        int sx = (sim_x + dx) % 128;
                        int sy = (sim_y + dy) % 128;
                        tnymap[sx][sy].food = 1;
                    }
                }
            }
        }
    }
    
    for (int i = 0; i < ant_list.size(); i++) {
        add_ant(i, (ant_list[i].x * 50) / 128, (ant_list[i].y * 50) / 128, ant_list[i].carrying_food, ant_list[i].state);
    }
    
    for (int x = 0; x < 50; x++) {
        for (int y = 0; y < 50; y++) {
            int map_x = (x * 128) / 50;
            int map_y = (y * 128) / 50;
            if (map_x < 128 && map_y < 128) {
                update_pheromone(x, y, tnymap[map_x][map_y].pher_level, 0);
            }
        }
    }
    
    render_world();
}

void decay_pheromones() {
    for (int x = 0; x < 128; x++) {
        for (int y = 0; y < 128; y++) {
            if (tnymap[x][y].pher_level > 0) {
                tnymap[x][y].pher_level--;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    // Added graphics initialization
    init_graphics();
    if (!graphics_active()) {
        cout << "Failed to init graphics" << endl;
        return 1;
    }
    
    // Initialize world map
    memset(tnymap, 0, sizeof(tnymap));
    
    int ant_num =0;
    if (argc%2-1) {
        std::cout << "Please provide a binary file and number for each file" << std::endl;
        return 1;
    }
    for (int i =2;i<argc; i+=2){
        FILE *bin_file = fopen(argv[i-1], "rb");
        if(bin_file !=NULL){
            for(int j = 0;j<atoi(argv[i]); j++){ // Fixed: use atoi instead of cast
                ant_list.push_back(tnyant());
                
                // Added initialization
                ant_list[ant_num].t = (teenyat*)malloc(sizeof(teenyat));
                rewind(bin_file);
                
                if (tny_init_from_file(ant_list[ant_num].t, bin_file, bus_read, bus_write)) {
                    ant_list[ant_num].x = 64 + (rand() % 20) - 10;
                    ant_list[ant_num].y = 64 + (rand() % 20) - 10;
                    ant_list[ant_num].dir = 1;
                    ant_list[ant_num].r = 255; ant_list[ant_num].g = 68; ant_list[ant_num].b = 68; ant_list[ant_num].a = 255;
                    ant_list[ant_num].carrying_food = false;
                    ant_list[ant_num].state = 0;
                    cout << "Ant " << ant_num << " initialized at (" << ant_list[ant_num].x << "," << ant_list[ant_num].y << ")" << endl;
                    ant_num++;
                } else {
                    cout << "Failed to initialize ant CPU" << endl;
                    free(ant_list[ant_num].t);
                    ant_list.pop_back();
                }
            }
           fclose(bin_file);
        } else {
            cout << "err binary file path: " << argv[i-1] << endl;
        }
    }
    
    cout << "Created " << ant_list.size() << " ants" << endl;
    
    // MUCH FASTER simulation loop for smooth movement
    int frame = 0;
    int decay_timer = 0;
    
    while (graphics_active() && frame < 50000) {
        frame++;
        
        for (int i = 0; i < ant_list.size(); i++) {
            num_ant = i;
            if (ant_list[i].t) {
                // EXECUTE MANY MORE CYCLES - ants will move much faster
                for (int cycle = 0; cycle < 500; cycle++) {  // Increased from 15 to 50!
                    tny_clock(ant_list[i].t);
                }
            }
        }
        
        // Slower pheromone decay for visible trails
        if (++decay_timer >= 300) {  // Increased from 100 to 300 - trails last 3x longer
            decay_pheromones();
            decay_timer = 0;
        }
        
        update_graphics_state();
        
        if (frame % 500 == 0) {  // Less frequent status updates
            cout << "Frame " << frame << " - " << ant_list.size() << " ants active" << endl;
        }
        
        // FASTER frame rate for smooth animation
        this_thread::sleep_for(chrono::milliseconds(30)); // Changed from 60ms to 30ms = 33 FPS
    }
    
    // Cleanup
    for (auto& ant : ant_list) {
        if (ant.t) {
            free(ant.t);
        }
    }
    
    cleanup_graphics();
    return 0;
}
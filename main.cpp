#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdio>
#include <thread>
#include <chrono>
#include <ctime>

#include "../teenyat.h"
#include "graphics.h"

using namespace std;

// -----------------------------------------------------------------------------
// Ports (matching your TeenyAnt spec)
// -----------------------------------------------------------------------------

const tny_uword SNIFF_NEAR_FOOD   = 0x9000; // sniff nearest food, local area
const tny_uword SNIFF_NEAR_PHER   = 0x9001; // sniff pheromone by direction
const tny_uword SNIFF_STRONG_PHER = 0x9002; // strongest pheromone in radius
const tny_uword SNIFF_PHER_DIR    = 0x9003; // sniff along facing direction
const tny_uword DROP_PHER         = 0x9005; // drop pheromone at current cell

// MOVE: direction code in low byte
//   4 = NORTH (y-1)
//   5 = EAST  (x+1)
//   6 = SOUTH (y+1)
//   7 = WEST  (x-1)
const tny_uword MOVE             = 0x9006;

const tny_uword SET_SNIFF_DIR    = 0x9007; // set facing direction (0..3)
const tny_uword CHECK_CARRYING    = 0x9010; // check if carrying food
const tny_uword SET_RG           = 0x9008; // reserved
const tny_uword SET_BA           = 0x9009; // reserved / optional color tweak
const tny_uword TRY_PICKUP_FOOD  = 0x9011; // Try to pickup food at current location
const tny_uword CHECK_NEST       = 0x900A;
const tny_uword CAN_CARRY        =0x900B;

// -----------------------------------------------------------------------------
// World + ant state
// -----------------------------------------------------------------------------

typedef struct {
    short pher_level;  // 0..255 pheromone (food trail)
    int   food;        // food count
    short ant_pres;    // number of ants in this cell
    short nest;        // 1 if part of nest region
} tnycell;

typedef struct {
    teenyat *t;
    short    x;
    short    y;
    short    dir;        // facing direction (0..3) for SNIFF_PHER_DIR
    unsigned char r, g, b, a;
    bool     can_carry;
    bool     carrying_food;
    int      state;
    int      file_index; // which binary this ant came from
} tnyant;

tnycell        tnymap[128][128];
vector<tnyant> ant_list;
int            num_ant = 0;  // index of ant currently being clocked on the bus

void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay); // func defs


// -----------------------------------------------------------------------------
// Per-file color palette + HUD registration
// -----------------------------------------------------------------------------

static void color_for_file(int file_index,
                           unsigned char &r,
                           unsigned char &g,
                           unsigned char &b)
{
    static const unsigned char palette[][3] = {
        {255,   0,   0}, // red
        {  0, 255,   0}, // green
        {  0,   0, 255}, // blue
        {255, 165,   0}, // orange
        {255,   0, 255}, // magenta
        {128,   0, 128}, // purple
        {  0, 255, 255}, // cyan
        {255, 255,   0}, // yellow
    };
    int idx = file_index % (int)(sizeof(palette) / sizeof(palette[0]));
    r = palette[idx][0];
    g = palette[idx][1];
    b = palette[idx][2];
}

// -----------------------------------------------------------------------------
// BUS READ: sensors
// -----------------------------------------------------------------------------

void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay) {
    (void)t;
    *delay = 1;
    data->u = 0;

    switch (addr) {

    // Nearest food in a 9x9 box around the ant.
    // Returns offsets in bytes: (i+4, j+4), or 0x64/0x64 if none.
    case SNIFF_NEAR_FOOD:
    {
        tny_word food;
        food.bytes.byte0 = 0x64; // sentinel "no target"
        food.bytes.byte1 = 0x64;

        for (int i = -4; i <= 4; i++) {
            for (int j = -4; j <= 4; j++) {
                int nx = (ant_list[num_ant].x + i + 128) % 128;
                int ny = (ant_list[num_ant].y + j + 128) % 128;

                if (tnymap[nx][ny].food > 0) {
                    // Compare squared distances to prefer closest
                    int old_dx    = (int)food.bytes.byte0 - 4;
                    int old_dy    = (int)food.bytes.byte1 - 4;
                    int old_dist2 = old_dx * old_dx + old_dy * old_dy;
                    int new_dist2 = i * i + j * j;

                    if (food.bytes.byte0 == 0x64 || new_dist2 < old_dist2) {
                        food.bytes.byte0 = (unsigned char)(i + 4);
                        food.bytes.byte1 = (unsigned char)(j + 4);
                    }
                }
            }
        }

        data->bytes.byte0 = food.bytes.byte0;
        data->bytes.byte1 = food.bytes.byte1;
    }
    break;

    // Directional pheromone sniff: sum pheromone in 4 cardinal directions
    // and return dir in byte0 (0=N,1=E,2=S,3=W) or 0x64/0x64 if none.
    case SNIFF_NEAR_PHER:
    {
        const unsigned char NO = 0x64;

        int cx = ant_list[num_ant].x;
        int cy = ant_list[num_ant].y;

        int sumN = 0, sumE = 0, sumS = 0, sumW = 0;

        // Skip radius 0/1 to avoid "self" pheromone bias
        for (int dist = 2; dist <= 8; dist++) {
            // north
            int nx = cx;
            int ny = (cy - dist + 128) % 128;
            sumN += tnymap[nx][ny].pher_level;

            // south
            ny = (cy + dist + 128) % 128;
            sumS += tnymap[nx][ny].pher_level;

            // east
            ny = cy;
            nx = (cx + dist + 128) % 128;
            sumE += tnymap[nx][ny].pher_level;

            // west
            nx = (cx - dist + 128) % 128;
            sumW += tnymap[nx][ny].pher_level;
        }

        int best = 0;
        int dir  = -1;

        if (sumN > best) { best = sumN; dir = 0; }
        if (sumE > best) { best = sumE; dir = 1; }
        if (sumS > best) { best = sumS; dir = 2; }
        if (sumW > best) { best = sumW; dir = 3; }

        if (dir < 0 || best <= 0) {
            data->bytes.byte0 = NO;
            data->bytes.byte1 = NO;
        } else {
            data->bytes.byte0 = (unsigned char)dir;
            data->bytes.byte1 = 0;
        }
    }
    break;

    // Strongest pheromone (within 17x17 square).
    // Returns (offset, strength): byte0 = dx+8, byte1 = strength.
    case SNIFF_STRONG_PHER:
    {
        tny_word strong;
        strong.bytes.byte0 = 0;
        strong.bytes.byte1 = 0;

        int max_pher = 0;

        for (int i = -8; i <= 8; i++) {
            for (int j = -8; j <= 8; j++) {
                int nx = (ant_list[num_ant].x + i + 128) % 128;
                int ny = (ant_list[num_ant].y + j + 128) % 128;
                int level = tnymap[nx][ny].pher_level;
                if (level > max_pher) {
                    max_pher = level;
                    strong.bytes.byte0 = (unsigned char)(i + 8);
                    strong.bytes.byte1 = (unsigned char)max_pher;
                }
            }
        }

        data->bytes.byte0 = strong.bytes.byte0;
        data->bytes.byte1 = strong.bytes.byte1;
    }
    break;

    // Sniff pheromone along the ant's facing direction (stored in dir).
    // dir = 0..3 (0=N,1=E,2=S,3=W).
    // Returns (distance, strength) or (0x64,0) if none.
    case SNIFF_PHER_DIR:
    {
        tny_word out;
        out.bytes.byte0 = 0x64;
        out.bytes.byte1 = 0;

        int cx = ant_list[num_ant].x;
        int cy = ant_list[num_ant].y;

        int best_strength = 0;
        int best_dist     = 0;

        int dcode = ant_list[num_ant].dir & 3;

        int dx = 0, dy = 0;
        if      (dcode == 0) { dx =  0; dy = -1; }
        else if (dcode == 1) { dx =  1; dy =  0; }
        else if (dcode == 2) { dx =  0; dy =  1; }
        else                 { dx = -1; dy =  0; }

        for (int dist = 1; dist <= 32; dist++) {
            int nx = cx + dx * dist;
            int ny = cy + dy * dist;
            if (nx < 0 || nx >= 128 || ny < 0 || ny >= 128) break;

            int level = tnymap[nx][ny].pher_level;
            if (level > best_strength) {
                best_strength = level;
                best_dist     = dist;
            }
        }

        if (best_strength > 0) {
            out.bytes.byte0 = (unsigned char)best_dist;
            out.bytes.byte1 = (unsigned char)best_strength;
        }
        data->bytes.byte0 = out.bytes.byte0;
        data->bytes.byte1 = out.bytes.byte1;
    }
    break;

    // Add this case in the bus_read switch statement:

    case CHECK_CARRYING:
        // Fix: Use the bytes structure to set the value
        data->bytes.byte0 = ant_list[num_ant].carrying_food ? 1 : 0;
        data->bytes.byte1 = 0;
        break;
    case CHECK_NEST:
        if (tnymap[ant_list[num_ant].x][ant_list[num_ant].y].nest >1){
            data->u = 0xFFFF;
        } else {
            data->u = 0x0000;
        }
        break;
    default:
        break;
    }
}

// -----------------------------------------------------------------------------
// BUS WRITE: actions (MOVE, DROP_PHER, etc.)
// -----------------------------------------------------------------------------

void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay) {
    (void)t;
    *delay = 1;

    switch (addr) {

    // MOVE
    // byte 0 is y centered at 0x80
    // byte 1 is x centered at 0x80 can use basic directions to move one, but can move further if desire
    case MOVE:
    {
        int cx = ant_list[num_ant].x;
        int cy = ant_list[num_ant].y;

        if (tnymap[cx][cy].ant_pres > 0){tnymap[cx][cy].ant_pres--;}

        int x = (data.bytes.byte1) - 0x80;
        int y = (data.bytes.byte0) - 0x80;
        

        int nx = cx + x;
        int ny = cy + y;
        //cout << cx << "," << cy << ",!!" << x << "," << y << ",!!" << nx << "," << ny << ",\n";

        // Clamp to 0..127
        if (nx < 0)   nx = nx % 128+128;
        if (nx > 127) nx = nx % 128;
        if (ny < 0)   ny = ny % 128+128;
        if (ny > 127) ny = ny % 128;

        ant_list[num_ant].x = (short)nx;
        ant_list[num_ant].y = (short)ny;
        tnymap[nx][ny].ant_pres++;
        short dir_m;
        if (y<0 && x>=0) {dir_m = 0;}
        else if (x>0 && y>=0){dir_m = 1;}
        else if (y>0 && x<=0){dir_m = 2;}
        else {dir_m = 3;}
        
        ant_list[num_ant].dir = dir_m;

        // SINGLE food collection check
        if (tnymap[nx][ny].food > 0 && ant_list[num_ant].can_carry) {
            tnymap[nx][ny].food--;
            ant_list[num_ant].carrying_food = true;
            ant_list[num_ant].state = 2;
            ant_list[num_ant].r = 255; ant_list[num_ant].g = 215; ant_list[num_ant].b = 0;

            // Clear the food cell on graphics
            int gx = (nx * 50) / 128;
            int gy = (ny * 50) / 128;
            if (gx >= 0 && gx < 50 && gy >= 0 && gy < 50)
                update_world_cell(gx, gy, 0);
            
            // Clear pheromone when food is picked up
            int clear_radius = 12;
            for (int dx = -clear_radius; dx <= clear_radius; dx++) {
                for (int dy = -clear_radius; dy <= clear_radius; dy++) {
                    int clear_x = (nx + dx + 128) % 128;
                    int clear_y = (ny + dy + 128) % 128;
                    tnymap[clear_x][clear_y].pher_level = 0;
                }
            }
            
            cout << "Ant " << num_ant << " found food and cleared pheromone area!" << endl;
        }

        // Drop food in nest - check simulation coordinates directly
        if (ant_list[num_ant].carrying_food) {
            // Check if ant is in nest region (simulation coordinates)
            if (nx >= 56 && nx <= 72 && ny >= 56 && ny <= 72) {
                ant_list[num_ant].carrying_food = false;
                ant_list[num_ant].state = 0;
                
                // Reset ant color based on file index
                unsigned char r, g, b;
                color_for_file(ant_list[num_ant].file_index, r, g, b);
                ant_list[num_ant].r = r;
                ant_list[num_ant].g = g;
                ant_list[num_ant].b = b;
                
                add_nest_food(1);
                cout << "Ant " << num_ant << " delivered food to nest and reset color!" << endl;
            }
        }
    }
    break;

    case DROP_PHER:
        // Pheromone only exists in sim grid; graphics mirror it
        if (!ant_list[num_ant].can_carry)
        tnymap[ant_list[num_ant].x][ant_list[num_ant].y].pher_level =
            data.bytes.byte0;
        break;

    case SET_SNIFF_DIR:{
        // Let user code explicitly set dir (0..3)
        // auto convert from regular dirs
        int snx = ((int)data.bytes.byte1) - 0x80;
        int sny = ((int)data.bytes.byte0) - 0x80;
        short dir_sn;
        if (sny<0 && snx>=0) {dir_sn =0;}
        if (snx>0 && sny>=0){dir_sn =1;}
        if (sny>0 && snx<=0){dir_sn = 2;}
        if (snx<0 && sny<=0) {dir_sn = 3;}
        
        ant_list[num_ant].dir = dir_sn;
        break;
    }
    case CHECK_CARRYING:
        // New port: check if the ant is carrying food
        // Returns 1 if carrying food, 0 if not
        data.bytes.byte0 = ant_list[num_ant].carrying_food ? 1 : 0;
        data.bytes.byte1 = 0;
        break;

    case SET_RG:
        // optional; keep no-op for now
        *delay = 1;
        break;

    case SET_BA:
        // optional; allow user to tweak B/A if you want
        ant_list[num_ant].b = data.bytes.byte0;
        ant_list[num_ant].a = data.bytes.byte1;
        break;

    case TRY_PICKUP_FOOD:
        // Attempt to pickup food at the current location
        if (tnymap[ant_list[num_ant].x][ant_list[num_ant].y].food > 0) {
            tnymap[ant_list[num_ant].x][ant_list[num_ant].y].food--;
            ant_list[num_ant].carrying_food = true;
            ant_list[num_ant].state = 2;
            ant_list[num_ant].r = 255; ant_list[num_ant].g = 215; ant_list[num_ant].b = 0;
        }
        break;
    case CAN_CARRY:
        ant_list[num_ant].can_carry = true;
        break;
    default:
        break;
    }
}

// -----------------------------------------------------------------------------
// Mapping between 50×50 graphics world and 128×128 sim world
// -----------------------------------------------------------------------------

static void map_food_to_sim() {
    // Reset sim food
    for (int x = 0; x < 128; x++)
        for (int y = 0; y < 128; y++)
            tnymap[x][y].food = 0;

    // For each 50×50 graphics cell with food, paint a small 3×3 patch in 128×128
    for (int gx = 0; gx < 50; gx++) {
        for (int gy = 0; gy < 50; gy++) {
            if (get_world_cell(gx, gy) == 1) {
                int sim_x = (gx * 128) / 50;
                int sim_y = (gy * 128) / 50;
                for (int dx = 0; dx < 3; dx++) {
                    for (int dy = 0; dy < 3; dy++) {
                        int sx = sim_x + dx;
                        int sy = sim_y + dy;
                        if (sx >= 0 && sx < 128 && sy >= 0 && sy < 128)
                            tnymap[sx][sy].food = 1;
                    }
                }
            }
        }
    }
}

static void map_ants_to_graphics() {
    for (int i = 0; i < (int)ant_list.size(); i++) {
        add_ant(i,
                ant_list[i].x,        // sim coords 0..127
                ant_list[i].y,
                ant_list[i].carrying_food ? 1 : 0,
                ant_list[i].state,
                ant_list[i].r,
                ant_list[i].g,
                ant_list[i].b);
    }
}

static void map_pheromones_to_graphics() {
    // FIRST: Clear all pheromone in graphics
    for (int gx = 0; gx < 50; gx++) {
        for (int gy = 0; gy < 50; gy++) {
            update_pheromone(gx, gy, 0, 0); // Clear everything first
        }
    }
    
    // THEN: Add pheromone where it exists in sim
    for (int sx = 0; sx < 128; sx++) {
        for (int sy = 0; sy < 128; sy++) {
            int level = tnymap[sx][sy].pher_level;
            if (level <= 0) continue;

            int gx = (sx * 50) / 128;
            int gy = (sy * 50) / 128;
            if (gx >= 0 && gx < 50 && gy >= 0 && gy < 50)
                update_pheromone(gx, gy, level, 0);
        }
    }
}

static void decay_pheromones() {
    // Simple decay: subtract 1 when > 0
    for (int x = 0; x < 128; x++) {
        for (int y = 0; y < 128; y++) {
            if (tnymap[x][y].pher_level > 0) {
                tnymap[x][y].pher_level--;
                if (tnymap[x][y].pher_level < 0)
                    tnymap[x][y].pher_level = 0;
            }
        }
    }
}

void clear_dead_trails() {
    // Clear pheromone in areas where there's no food nearby
    for (int x = 0; x < 128; x++) {
        for (int y = 0; y < 128; y++) {
            if (tnymap[x][y].pher_level > 50) {  // Only check strong pheromone areas
                // Check if there's food in a 6-cell radius
                bool food_nearby = false;
                for (int dx = -6; dx <= 6 && !food_nearby; dx++) {
                    for (int dy = -6; dy <= 6 && !food_nearby; dy++) {
                        int check_x = (x + dx + 128) % 128;
                        int check_y = (y + dy + 128) % 128;
                        if (tnymap[check_x][check_y].food > 0) {
                            food_nearby = true;
                        }
                    }
                }
                
                // If no food nearby, reduce pheromone faster
                if (!food_nearby) {
                    tnymap[x][y].pher_level = tnymap[x][y].pher_level / 2;  // Cut in half
                }
            }
        }
    }
}

// Limit food to 5 piles max and spawn new ones randomly
static void maybe_spawn_food_dynamic() {
    const int MAX_FOOD = 5;

    int current_food = 0;
    for (int gx = 0; gx < 50; gx++)
        for (int gy = 0; gy < 50; gy++)
            if (get_world_cell(gx, gy) == 1)
                current_food++;

    if (current_food >= MAX_FOOD) return;

    // ~1/120 chance per frame
    if (rand() % 120 != 0) return;

    for (int tries = 0; tries < 50; tries++) {
        int gx = rand() % 50;
        int gy = rand() % 50;
        if (get_world_cell(gx, gy) == 0) {
            update_world_cell(gx, gy, 1);
            cout << "New food spawned at (" << gx << "," << gy << ")\n";
            break;
        }
    }
}

static void update_graphics_state() {
    map_food_to_sim();
    map_ants_to_graphics();
    map_pheromones_to_graphics();
    render_world();
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));

    init_graphics();
    if (!graphics_active()) {
        cout << "Graphics init failed.\n";
        return 1;
    }

    memset(tnymap, 0, sizeof(tnymap));

    // Mark a central nest region in sim (approx 3×3 nest in graphics)
    for (int x = 56; x <= 72; x++) {
        for (int y = 56; y <= 72; y++) {
            if (x >= 0 && x < 128 && y >= 0 && y < 128)
                tnymap[x][y].nest = 1;
        }
    }

    int ant_num    = 0;
    int file_index = 0;

    if (argc % 2 - 1) {
        cout << "Usage: " << argv[0]
             << " bin1 count1 [bin2 count2 ...]\n";
    }

    // argv pattern: teenyants.exe bin1 count1 bin2 count2 ...
    for (int i = 2; i < argc; i += 2, file_index++) {
        FILE *bin_file = fopen(argv[i - 1], "rb");
        if (!bin_file) {
            cout << "Error opening binary file: " << argv[i - 1] << endl;
            continue;
        }

        int count = atoi(argv[i]);

        unsigned char base_r, base_g, base_b;
        color_for_file(file_index, base_r, base_g, base_b);
        register_program_info(file_index, argv[i - 1],
                              base_r, base_g, base_b);

        cout << "Loading " << count << " ants from " << argv[i - 1]
             << " (file index " << file_index << ")\n";

        for (int j = 0; j < count; j++) {
            ant_list.push_back(tnyant());
            ant_list[ant_num].t = (teenyat*)malloc(sizeof(teenyat));
            rewind(bin_file);

            if (tny_init_from_file(ant_list[ant_num].t,
                                   bin_file,
                                   bus_read,
                                   bus_write))
            {
                ant_list[ant_num].x   = 64 + (rand() % 20) - 10;
                ant_list[ant_num].y   = 64 + (rand() % 20) - 10;
                ant_list[ant_num].dir = 1; // facing east by default
                ant_list[ant_num].a   = 255;
                ant_list[ant_num].carrying_food = false;
                ant_list[ant_num].state         = 0;
                ant_list[ant_num].file_index    = file_index;
                ant_list[ant_num].can_carry = false;
                ant_list[ant_num].r = base_r;
                ant_list[ant_num].g = base_g;
                ant_list[ant_num].b = base_b;

                cout << "Ant " << ant_num << " from file " << file_index
                     << " at (" << ant_list[ant_num].x << ","
                     << ant_list[ant_num].y << ")\n";
                ant_num++;
            } else {
                cout << "Failed to initialize ant CPU for "
                     << argv[i - 1] << endl;
                free(ant_list[ant_num].t);
                ant_list.pop_back();
            }
        }

        fclose(bin_file);
    }

    cout << "Total ants: " << ant_list.size() << endl;

    int frame       = 0;
    int decay_timer = 0;

    while (graphics_active() && frame < 50000) {
        frame++;
        // Run each ant's teenyAT CPU
        for (int i = 0; i < (int)ant_list.size(); i++) {
            num_ant = i;
            if (ant_list[i].t) {
                // Plenty of cycles per frame for smooth movement
                for (int cycle = 0; cycle < 100; cycle++)
                    tny_clock(ant_list[i].t);
            }
        }

        // Pheromone decay every N frames to achieve ~700ms lifetime
        // At 30ms per frame: 700ms ÷ 30ms = ~23 frames
        //if (++decay_timer >= 23) {
        //    decay_pheromones();
        //    decay_timer = 0;
        //}

        // Faster pheromone decay for shorter trails
        // At 30ms per frame: 400ms ÷ 30ms = ~13 frames
        //if (++decay_timer >= 13) {
        //    decay_pheromones();
        //    decay_timer = 0;
        //}
        
        // MUCH faster pheromone decay - trails disappear quickly!
        // At 30ms per frame: 300ms ÷ 30ms = ~10 frames
        if (++decay_timer >= 5) {
            decay_pheromones();
            decay_timer = 0;
        }
        
        // Slower pheromone decay so you can see trails
        // At 30ms per frame: 1000ms ÷ 30ms = ~33 frames
        //if (++decay_timer >= 33) {
        //    decay_pheromones();
        //    decay_timer = 0;
        //}
        
        // Clear dead trails every 50 frames (less frequent)
        if (frame % 50 == 0) {
            clear_dead_trails();
        }

        maybe_spawn_food_dynamic();
        update_graphics_state();

        if (frame % 500 == 0) {
            cout << "Frame " << frame
                 << " | ants: " << ant_list.size()
                 << " | nest food: " << get_nest_food() << endl;
        }

        this_thread::sleep_for(chrono::milliseconds(30));
    }

    // Cleanup
    for (auto &ant : ant_list) {
        if (ant.t) free(ant.t);
    }

    cleanup_graphics();
    return 0;
}

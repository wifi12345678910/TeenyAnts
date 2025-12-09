#include "graphics.h"
#include "tigr.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define SCREEN_W   1000
#define SCREEN_H   800

#define HUD_H      60          // top HUD bar height
#define WORLD_SIZE 50

// World area height in pixels (below HUD)
#define WORLD_PIX_H (SCREEN_H - HUD_H)

// Cell size so the 50x50 grid fits vertically
#define CELL_SIZE   (WORLD_PIX_H / WORLD_SIZE)
// Actual world width in pixels (may be < SCREEN_W so we can center)
#define WORLD_PIX_W (CELL_SIZE * WORLD_SIZE)
// Left margin so world is centered
#define WORLD_OFFSET_X ((SCREEN_W - WORLD_PIX_W) / 2)

// Convert world grid (0..49) → screen pixels
static int world_to_screen_x(int wx) { return WORLD_OFFSET_X + wx * CELL_SIZE; }
static int world_to_screen_y(int wy) { return HUD_H + wy * CELL_SIZE; }

static Tigr *screen      = NULL;
static int   initialized = 0;

static int nest_food_stored = 0;

typedef struct {
    float x, y;     // sim coords (0..127)
    float dir;      // angle in radians
    int   ant_id;
    int   carrying_food;
    int   state;
    int   base_r, base_g, base_b;
} GraphicsAnt;

static GraphicsAnt ants[100];
static int         ant_count = 0;

// 50x50 world for food / nest
static int world_grid[WORLD_SIZE][WORLD_SIZE];
// 50x50 pheromone (from 128x128 sim, mapped down)
static int pheromone_food[WORLD_SIZE][WORLD_SIZE];

// Program legend info
typedef struct {
    char label[32];
    int  r, g, b;
    int  used;
} ProgramInfo;

#define MAX_PROGRAMS 8
static ProgramInfo programs[MAX_PROGRAMS];

// ---------------------------------------------------------------------
// Legend registration (from main.cpp)
// ---------------------------------------------------------------------

void register_program_info(int index, const char *label, int r, int g, int b) {
    if (index < 0 || index >= MAX_PROGRAMS) return;

    programs[index].r    = r;
    programs[index].g    = g;
    programs[index].b    = b;
    programs[index].used = 1;

    if (label) {
        const char *base = label;
        const char *p    = label;
        while (*p) {
            if (*p == '\\' || *p == '/')
                base = p + 1;
            p++;
        }
        strncpy(programs[index].label, base, sizeof(programs[index].label) - 1);
        programs[index].label[sizeof(programs[index].label) - 1] = '\0';
    } else {
        snprintf(programs[index].label, sizeof(programs[index].label),
                 "prog%d", index);
    }
}

// ---------------------------------------------------------------------
// Small drawing helpers
// ---------------------------------------------------------------------

static void draw_circle_pix(int cx0, int cy0, int radius, TPixel col) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx*dx + dy*dy <= radius*radius) {
                int sx = cx0 + dx;
                int sy = cy0 + dy;
                if (sx >= 0 && sx < SCREEN_W &&
                    sy >= 0 && sy < SCREEN_H) {
                    tigrPlot(screen, sx, sy, col);
                }
            }
        }
    }
}

static void draw_line_pix(int x0, int y0, int x1, int y1, TPixel col) {
    int steps = 6;
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;
        int sx = x0 + (int)((x1 - x0) * t);
        int sy = y0 + (int)((y1 - y0) * t);
        if (sx >= 0 && sx < SCREEN_W &&
            sy >= 0 && sy < SCREEN_H) {
            tigrPlot(screen, sx, sy, col);
        }
    }
}

// ---------------------------------------------------------------------
// Init / shutdown
// ---------------------------------------------------------------------

void init_graphics(void) {
    if (initialized) return;

    printf("🐜 Initializing TeenyAnts Graphics System...\n");

    screen = tigrWindow(SCREEN_W, SCREEN_H,
                        "TeenyAnts Colony Simulation", TIGR_FIXED);
    if (!screen) {
        printf("❌ Failed to create TeenyAnts window\n");
        return;
    }

    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int y = 0; y < WORLD_SIZE; y++) {
            world_grid[x][y]     = 0;
            pheromone_food[x][y] = 0;
        }
    }

    // Simple 3x3 nest in center of 50x50 world
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int nx = WORLD_SIZE / 2 + dx;
            int ny = WORLD_SIZE / 2 + dy;
            if (nx >= 0 && nx < WORLD_SIZE &&
                ny >= 0 && ny < WORLD_SIZE)
                world_grid[nx][ny] = 2;
        }
    }

    // Initial random food
    for (int i = 0; i < 3; i++) {
        int fx = rand() % WORLD_SIZE;
        int fy = rand() % WORLD_SIZE;
        if (world_grid[fx][fy] == 0) {
            world_grid[fx][fy] = 1;
            printf("🍎 Initial food spawned at (%d,%d)\n", fx, fy);
        }
    }

    ant_count        = 0;
    nest_food_stored = 0;

    for (int i = 0; i < MAX_PROGRAMS; i++) {
        programs[i].used      = 0;
        programs[i].label[0]  = '\0';
        programs[i].r = programs[i].g = programs[i].b = 255;
    }

    initialized = 1;

    printf("✅ Graphics ready: %dx%d window, %dx%d world\n",
           SCREEN_W, SCREEN_H, WORLD_SIZE, WORLD_SIZE);
}

void cleanup_graphics(void) {
    if (screen) {
        tigrFree(screen);
        screen = NULL;
    }
    initialized = 0;
    printf("🎮 TeenyAnts graphics cleaned up\n");
}

int graphics_active(void) {
    return screen && !tigrClosed(screen) && !tigrKeyDown(screen, TK_ESCAPE);
}

// ---------------------------------------------------------------------
// World helpers
// ---------------------------------------------------------------------

void spawn_random_food(void) {
    int spawned = 0;
    while (spawned < 3) {
        int fx = rand() % WORLD_SIZE;
        int fy = rand() % WORLD_SIZE;
        if (world_grid[fx][fy] == 0) {
            world_grid[fx][fy] = 1;
            spawned++;
            printf("🍎 Initial food spawned at (%d,%d)\n", fx, fy);
        }
    }
}

int get_world_cell(int x, int y) {
    if (x >= 0 && x < WORLD_SIZE && y >= 0 && y < WORLD_SIZE)
        return world_grid[x][y];
    return 0;
}

void add_nest_food(int amount) {
    if (amount > 0)
        nest_food_stored += amount;
}

int get_nest_food(void) {
    return nest_food_stored;
}

// ---------------------------------------------------------------------
// Ant bookkeeping
// ---------------------------------------------------------------------

void add_ant(int ant_id, int x, int y,
             int carrying_food, int state,
             int r, int g, int b)
{
    int idx = -1;
    for (int i = 0; i < ant_count; i++) {
        if (ants[i].ant_id == ant_id) {
            idx = i;
            break;
        }
    }

    if (idx == -1 && ant_count < 100) {
        idx              = ant_count++;
        ants[idx].ant_id = ant_id;
        ants[idx].dir    = 0.0f;
    }

    if (idx >= 0) {
        float new_x = (float)x;
        float new_y = (float)y;

        float dx = new_x - ants[idx].x;
        float dy = new_y - ants[idx].y;
        if (dx != 0.0f || dy != 0.0f)
            ants[idx].dir = atan2f(dy, dx);

        ants[idx].x             = new_x;
        ants[idx].y             = new_y;
        ants[idx].carrying_food = carrying_food;
        ants[idx].state         = state;
        ants[idx].base_r        = r;
        ants[idx].base_g        = g;
        ants[idx].base_b        = b;
    }
}

void draw_ant(int x, int y, uint32_t color) {
    (void)x; (void)y; (void)color;
    // actual ant drawing happens in render_world from ants[]
}

// ---------------------------------------------------------------------
// Cells / food / pheromone drawing
// ---------------------------------------------------------------------

void draw_cell(int x, int y, uint32_t color) {
    if (!screen) return;

    int sx = world_to_screen_x(x);
    int sy = world_to_screen_y(y);

    int r = (color >> 16) & 0xFF;
    int g = (color >> 8)  & 0xFF;
    int b =  color        & 0xFF;

    TPixel pixel = tigrRGB(r, g, b);

    if (color == 0x32CD32) {
        // FOOD: big black dot in the cell
        int cx = sx + CELL_SIZE/2;
        int cy = sy + CELL_SIZE/2;
        TPixel black = tigrRGB(0, 0, 0);
        draw_circle_pix(cx, cy, 5, black);
    } else if (color == 0x8B4513) {
        // NEST: brown block
        tigrFill(screen, sx, sy, CELL_SIZE, CELL_SIZE, pixel);
    } else {
        tigrFill(screen, sx, sy, CELL_SIZE, CELL_SIZE, pixel);
    }
}

void draw_food(int screen_x, int screen_y) {
    (void)screen_x; (void)screen_y;
    // not used directly; food is drawn via draw_cell
}

void draw_pheromone(int x, int y, int strength) {
    if (!screen || strength <= 0) return;

    int cx = world_to_screen_x(x) + CELL_SIZE/2;
    int cy = world_to_screen_y(y) + CELL_SIZE/2;

    TPixel pheromone;
    if (strength > 60)
        pheromone = tigrRGB(255, 60,  0);
    else if (strength > 30)
        pheromone = tigrRGB(255, 130, 0);
    else
        pheromone = tigrRGB(255, 210, 80);

    int radius = 3 + (strength / 20);
    if (radius > 7) radius = 7;

    for (int layer = 0; layer < 2; layer++) {
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dy = -radius; dy <= radius; dy++) {
                if (dx*dx + dy*dy <= radius*radius) {
                    int px = cx + dx;
                    int py = cy + dy;
                    if (px >= 0 && px < SCREEN_W &&
                        py >= 0 && py < SCREEN_H) {
                        tigrPlot(screen, px, py, pheromone);
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------
// Screen / grid / HUD
// ---------------------------------------------------------------------

void clear_screen(uint32_t color) {
    (void)color;
    if (!screen) return;

    TPixel base  = tigrRGB(220, 205, 170);
    TPixel grain = tigrRGB(195, 185, 155);
    tigrClear(screen, base);

    for (int y = HUD_H; y < SCREEN_H; y += 4) {
        for (int x = 0; x < SCREEN_W; x += 4) {
            if ((x + y) % 7 == 0)
                tigrPlot(screen, x, y, grain);
        }
    }
}

void draw_world_grid(void) {
    if (!screen) return;
    TPixel grid_fine = tigrRGB(150, 150, 150);
    TPixel grid_bold = tigrRGB(110, 110, 110);

    // Vertical lines at exact cell boundaries
    for (int gx = 0; gx <= WORLD_SIZE; gx++) {
        int sx = world_to_screen_x(gx);   // uses CELL_SIZE and WORLD_OFFSET_X
        if (sx < 0) sx = 0;
        if (sx >= SCREEN_W) sx = SCREEN_W - 1;

        for (int y = HUD_H; y < SCREEN_H; y++) {
            if (gx % 10 == 0)
                tigrPlot(screen, sx, y, grid_bold);
            else if (gx % 5 == 0)
                tigrPlot(screen, sx, y, grid_fine);
        }
    }

    // Horizontal lines at exact cell boundaries
    for (int gy = 0; gy <= WORLD_SIZE; gy++) {
        int sy = world_to_screen_y(gy);   // uses CELL_SIZE and HUD_H
        if (sy < HUD_H) sy = HUD_H;
        if (sy >= SCREEN_H) sy = SCREEN_H - 1;

        for (int x = 0; x < SCREEN_W; x++) {
            if (gy % 10 == 0)
                tigrPlot(screen, x, sy, grid_bold);
            else if (gy % 5 == 0)
                tigrPlot(screen, x, sy, grid_fine);
        }
    }
}


void draw_status_info(int ant_count_display, int food_count) {
    if (!screen) return;

    tigrFill(screen, 0, 0, SCREEN_W, HUD_H, tigrRGB(15, 20, 30));

    char status[256];
    sprintf(status,
            "TeenyAnts: %d Ants | %d Food on ground | Nest food stored: %d",
            ant_count_display, food_count, nest_food_stored);

    TPixel white      = tigrRGB(255, 255, 255);
    TPixel brown      = tigrRGB(139, 69, 19);
    TPixel gray       = tigrRGB(200, 200, 200);
    TPixel trail_col  = tigrRGB(255, 210, 80);

    tigrPrint(screen, tfont, 10,  6, white, status);
    tigrPrint(screen, tfont, 10, 24, brown, "Nest = brown cells");
    tigrPrint(screen, tfont, 10, 36, gray,  "Food = big black dots");
    tigrPrint(screen, tfont, 260,24, trail_col,
              "Pheromone trail = orange/yellow glow");
    tigrPrint(screen, tfont, 260,36, white,
              "Carrying food = brighter ant");

    // Multi-column program list so all active files are visible
    int base_x1 = 600;
    int base_x2 = 780;
    int y1      = 24;
    int y2      = 24;

    tigrPrint(screen, tfont, base_x1,  6, white, "Programs / Ant Colors:");

    for (int i = 0; i < MAX_PROGRAMS; i++) {
        if (!programs[i].used) continue;

        char line[64];
        snprintf(line, sizeof(line), " = %s", programs[i].label);

        if (y1 <= HUD_H - 12) {
            // Left column
            TPixel pc = tigrRGB(programs[i].r, programs[i].g, programs[i].b);
            tigrFill(screen, base_x1, y1, 12, 12, pc);
            tigrPrint(screen, tfont, base_x1 + 16, y1, white, line);
            y1 += 12;
        } else {
            // Right column if left is full
            TPixel pc = tigrRGB(programs[i].r, programs[i].g, programs[i].b);
            tigrFill(screen, base_x2, y2, 12, 12, pc);
            tigrPrint(screen, tfont, base_x2 + 16, y2, white, line);
            y2 += 12;
        }
    }
}

void present_graphics(void) {
    if (!screen) return;
    tigrUpdate(screen);
}

// ---------------------------------------------------------------------
// World state updates from simulation
// ---------------------------------------------------------------------

void update_world_cell(int x, int y, int cell_type) {
    if (x >= 0 && x < WORLD_SIZE && y >= 0 && y < WORLD_SIZE)
        world_grid[x][y] = cell_type;
}

void update_pheromone(int x, int y, int food_strength, int nest_strength) {
    (void)nest_strength;

    if (x >= 0 && x < WORLD_SIZE && y >= 0 && y < WORLD_SIZE) {
        // Just mirror the simulation value.
        // 0 clears the cell, positive values draw stronger trail.
        pheromone_food[x][y] = food_strength;
    }
}



// ---------------------------------------------------------------------
// Main render entry point
// ---------------------------------------------------------------------

void render_world(void) {
    if (!screen) return;

    clear_screen(0);
    draw_world_grid();

    // 1) Draw pheromone field
    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int y = 0; y < WORLD_SIZE; y++) {
            if (pheromone_food[x][y] > 0)
                draw_pheromone(x, y, pheromone_food[x][y]);
        }
    }

    // 2) Draw nest, food, etc.
    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int y = 0; y < WORLD_SIZE; y++) {
            switch (world_grid[x][y]) {
            case 1: draw_cell(x, y, 0x32CD32); break; // food
            case 2: draw_cell(x, y, 0x8B4513); break; // nest
            case 3: draw_cell(x, y, 0x696969); break; // wall (unused now)
            default: break;
            }
        }
    }

    // 3) Draw ants as little ant sprites centered in the grid cells
    for (int i = 0; i < ant_count; i++) {
        int gx = (int)(ants[i].x * WORLD_SIZE / 128.0f);
        int gy = (int)(ants[i].y * WORLD_SIZE / 128.0f);

        if (gx < 0) gx = 0;
        if (gx >= WORLD_SIZE) gx = WORLD_SIZE - 1;
        if (gy < 0) gy = 0;
        if (gy >= WORLD_SIZE) gy = WORLD_SIZE - 1;

        int cx = world_to_screen_x(gx) + CELL_SIZE/2;
        int cy = world_to_screen_y(gy) + CELL_SIZE/2;

        int r = ants[i].base_r;
        int g = ants[i].base_g;
        int b = ants[i].base_b;

        if (ants[i].carrying_food) {
            r = (r + 255) / 2;
            g = (g + 215) / 2;
            b = (b + 0)   / 2;
        }

        TPixel body_main  = tigrRGB(r, g, b);
        TPixel body_dark  = tigrRGB(r/2, g/2, b/2);
        TPixel head_color = tigrRGB(20, 20, 20);

        float cos_dir = cosf(ants[i].dir);
        float sin_dir = sinf(ants[i].dir);
        float px      = -sin_dir;
        float py      =  cos_dir;

        int abdomen_x = cx - (int)(cos_dir * 5.0f);
        int abdomen_y = cy - (int)(sin_dir * 5.0f);
        int thorax_x  = cx;
        int thorax_y  = cy;
        int head_x    = cx + (int)(cos_dir * 5.0f);
        int head_y    = cy + (int)(sin_dir * 5.0f);

        draw_circle_pix(abdomen_x, abdomen_y, 5, body_dark);
        draw_circle_pix(thorax_x,  thorax_y,  4, body_main);
        draw_circle_pix(head_x,    head_y,    3, head_color);

        TPixel leg_color = tigrRGB(30, 30, 30);
        for (int k = -1; k <= 1; k++) {
            float offset = (float)k * 2.0f;
            float spread = 3.0f + fabsf((float)k) * 1.0f;

            int base_x = thorax_x + (int)(cos_dir * offset);
            int base_y = thorax_y + (int)(sin_dir * offset);

            int lx1 = base_x + (int)(px * spread);
            int ly1 = base_y + (int)(py * spread);
            int lx2 = lx1 + (int)(cos_dir * 5.0f);
            int ly2 = ly1 + (int)(sin_dir * 5.0f);

            int rx1 = base_x - (int)(px * spread);
            int ry1 = base_y - (int)(py * spread);
            int rx2 = rx1 + (int)(cos_dir * 5.0f);
            int ry2 = ry1 + (int)(sin_dir * 5.0f);

            draw_line_pix(lx1, ly1, lx2, ly2, leg_color);
            draw_line_pix(rx1, ry1, rx2, ry2, leg_color);
        }

        TPixel antenna_color = tigrRGB(10, 10, 10);
        for (int side = -1; side <= 1; side += 2) {
            float s = (float)side;
            int base_x = head_x + (int)(px * s * 2.0f);
            int base_y = head_y + (int)(py * s * 2.0f);

            int tip_x  = base_x + (int)(cos_dir * 6.0f + px * s * 3.0f);
            int tip_y  = base_y + (int)(sin_dir * 6.0f + py * s * 3.0f);

            draw_line_pix(base_x, base_y, tip_x, tip_y, antenna_color);
        }
    }

    int food_count = 0;
    for (int x = 0; x < WORLD_SIZE; x++)
        for (int y = 0; y < WORLD_SIZE; y++)
            if (world_grid[x][y] == 1)
                food_count++;

    draw_status_info(ant_count, food_count);
    present_graphics();
}

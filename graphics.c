#include "graphics.h"
#include "tigr.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define SCREEN_W 1000
#define SCREEN_H 800
#define WORLD_SIZE 50
#define CELL_SIZE (SCREEN_W / WORLD_SIZE)  // Each cell is 20x20 pixels

static Tigr *screen = NULL;
static bool initialized = false;

// Ant structure for graphics representation
typedef struct {
    float x, y;           // Position in world coordinates
    float dir;            // Direction in radians
    int ant_id;
    bool carrying_food;
    int state;            // 0=exploring, 1=returning, 2=following_trail
    int color_r, color_g, color_b;
} GraphicsAnt;

static GraphicsAnt ants[100];  // Support up to 100 ants
static int ant_count = 0;

// World representation
static int world_grid[WORLD_SIZE][WORLD_SIZE];  // 0=empty, 1=food, 2=nest, 3=wall, 4=tunnel
static int pheromone_food[WORLD_SIZE][WORLD_SIZE];  // Pheromone strength (0-10)
static int pheromone_nest[WORLD_SIZE][WORLD_SIZE];  // Pheromone strength (0-10)

void init_graphics(void) {
    if (initialized) return;
    
    printf("🐜 Initializing TeenyAnts Graphics System...\n");
    
    screen = tigrWindow(SCREEN_W, SCREEN_H, "TeenyAnts Colony Simulation", TIGR_FIXED);
    if (!screen) {
        printf("❌ Failed to create TeenyAnts window\n");
        return;
    }
    
    // Initialize world grid to empty
    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int y = 0; y < WORLD_SIZE; y++) {
            world_grid[x][y] = 0;  // Empty
            pheromone_food[x][y] = 0;
            pheromone_nest[x][y] = 0;
        }
    }
    
    // Create nest in center (3x3 area)
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            world_grid[25 + dx][25 + dy] = 2;  // Nest
        }
    }
    
    // Spawn random food with messages
    spawn_random_food();
    
    ant_count = 0;
    initialized = true;
    printf("✅ TeenyAnts graphics initialized (%dx%d, %dx%d cells)\n", 
           SCREEN_W, SCREEN_H, WORLD_SIZE, WORLD_SIZE);
}

void cleanup_graphics(void) {
    if (screen) {
        tigrFree(screen);
        screen = NULL;
    }
    initialized = false;
    printf("🎮 TeenyAnts graphics cleaned up\n");
}

int graphics_active(void) {
    return screen && !tigrClosed(screen) && !tigrKeyDown(screen, TK_ESCAPE);
}

// Add missing spawn_random_food function
void spawn_random_food(void) {
    // Scatter food randomly with spawn messages
    for (int i = 0; i < 5; i++) {
        int fx, fy;
        do {
            fx = rand() % WORLD_SIZE;
            fy = rand() % WORLD_SIZE;
        } while (world_grid[fx][fy] != 0);  // Find empty spot
        
        world_grid[fx][fy] = 1;  // Food
        printf("🍎 Food spawned at (%d, %d)\n", fx, fy);
    }
}

// Add missing get_world_cell function
int get_world_cell(int x, int y) {
    if (x >= 0 && x < WORLD_SIZE && y >= 0 && y < WORLD_SIZE) {
        return world_grid[x][y];
    }
    return 0;
}

// Convert world coordinates to screen coordinates
int world_to_screen_x(int world_x) {
    return world_x * CELL_SIZE;
}

int world_to_screen_y(int world_y) {
    return world_y * CELL_SIZE;
}

// Add or update an ant in the graphics system
void add_ant(int ant_id, int x, int y, int carrying_food, int state) {
    // Find existing ant or add new one
    int ant_index = -1;
    for (int i = 0; i < ant_count; i++) {
        if (ants[i].ant_id == ant_id) {
            ant_index = i;
            break;
        }
    }
    
    if (ant_index == -1 && ant_count < 100) {
        ant_index = ant_count++;
        ants[ant_index].ant_id = ant_id;
        ants[ant_index].dir = 0.0f;
    }
    
    if (ant_index >= 0) {
        // Update ant position smoothly
        float new_x = (float)x;
        float new_y = (float)y;
        
        // Calculate direction based on movement
        float dx = new_x - ants[ant_index].x;
        float dy = new_y - ants[ant_index].y;
        if (dx != 0 || dy != 0) {
            ants[ant_index].dir = atan2(dy, dx);
        }
        
        ants[ant_index].x = new_x;
        ants[ant_index].y = new_y;
        ants[ant_index].carrying_food = carrying_food;
        ants[ant_index].state = state;
        
        // Set ant colors based on state
        if (carrying_food) {
            ants[ant_index].color_r = 255;  // Gold
            ants[ant_index].color_g = 215;
            ants[ant_index].color_b = 0;
        } else {
            switch (state) {
                case 0: // Exploring
                    ants[ant_index].color_r = 255;  // Red
                    ants[ant_index].color_g = 68;
                    ants[ant_index].color_b = 68;
                    break;
                case 1: // Returning/Following trail
                    ants[ant_index].color_r = 255;  // Orange
                    ants[ant_index].color_g = 140;
                    ants[ant_index].color_b = 0;
                    break;
                default:
                    ants[ant_index].color_r = 139;  // Brown
                    ants[ant_index].color_g = 69;
                    ants[ant_index].color_b = 19;
            }
        }
    }
}

// Draw a realistic ant (MUCH BIGGER and more visible)
void draw_realistic_ant(int screen_x, int screen_y, float direction, int r, int g, int b) {
    TPixel ant_color = tigrRGB(r, g, b);
    TPixel dark_color = tigrRGB(r/2, g/2, b/2);
    TPixel bright_color = tigrRGB(255, 255, 255);  // White highlight for visibility
    
    // Make ants MUCH bigger - draw a 5x5 body instead of 3x3
    
    // Main body (5x5 oval)
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -2; dy <= 2; dy++) {
            int px = screen_x + dx;
            int py = screen_y + dy;
            if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H) {
                // Create oval shape
                if (dx*dx + dy*dy <= 4) {  // Circular body
                    if (dx == 0 && dy == 0) {
                        // Center - bright ant color
                        tigrPlot(screen, px, py, ant_color);
                    } else if (abs(dx) <= 1 && abs(dy) <= 1) {
                        // Inner area - ant color
                        tigrPlot(screen, px, py, ant_color);
                    } else {
                        // Edges - darker outline
                        tigrPlot(screen, px, py, dark_color);
                    }
                }
            }
        }
    }
    
    // Direction indicator (larger white dot showing where ant is facing)
    float cos_dir = cos(direction);
    float sin_dir = sin(direction);
    
    // Head (3 pixels forward)
    for (int i = 1; i <= 3; i++) {
        int head_x = screen_x + (int)(cos_dir * i);
        int head_y = screen_y + (int)(sin_dir * i);
        
        if (head_x >= 0 && head_x < SCREEN_W && head_y >= 0 && head_y < SCREEN_H) {
            tigrPlot(screen, head_x, head_y, bright_color);  // White direction indicator
        }
    }
    
    // Antennae/legs (more visible)
    float leg_angle1 = direction + 1.57f;  // 90 degrees
    float leg_angle2 = direction - 1.57f;  // -90 degrees
    
    for (int i = 1; i <= 2; i++) {
        int leg1_x = screen_x + (int)(cos(leg_angle1) * i);
        int leg1_y = screen_y + (int)(sin(leg_angle1) * i);
        int leg2_x = screen_x + (int)(cos(leg_angle2) * i);
        int leg2_y = screen_y + (int)(sin(leg_angle2) * i);
        
        if (leg1_x >= 0 && leg1_x < SCREEN_W && leg1_y >= 0 && leg1_y < SCREEN_H) {
            tigrPlot(screen, leg1_x, leg1_y, dark_color);
        }
        if (leg2_x >= 0 && leg2_x < SCREEN_W && leg2_y >= 0 && leg2_y < SCREEN_H) {
            tigrPlot(screen, leg2_x, leg2_y, dark_color);
        }
    }
}


// ONLY CHANGE: Make food smaller
void draw_cell(int x, int y, uint32_t color) {
    if (!screen) return;
    
    int screen_x = world_to_screen_x(x);
    int screen_y = world_to_screen_y(y);
    
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    
    TPixel pixel = tigrRGB(r, g, b);
    
    // Check if this is food (green color)
    if (color == 0x32CD32) {  // Food color
        // Draw small food pellet instead of full cell
        int center_x = screen_x + CELL_SIZE/2;
        int center_y = screen_y + CELL_SIZE/2;
        
        // 5x5 food pellet
        for (int dx = -2; dx <= 2; dx++) {
            for (int dy = -2; dy <= 2; dy++) {
                if (abs(dx) + abs(dy) <= 2) {  // Diamond shape
                    int px = center_x + dx;
                    int py = center_y + dy;
                    if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H) {
                        tigrPlot(screen, px, py, pixel);
                    }
                }
            }
        }
    } else {
        // Draw full cell for nest, walls, etc.
        tigrFill(screen, screen_x, screen_y, CELL_SIZE, CELL_SIZE, pixel);
    }
}

// Enhanced pheromone drawing - create trailing effect
void draw_pheromone(int x, int y, int strength) {
    if (!screen || strength <= 0) return;
    
    int screen_x = world_to_screen_x(x) + CELL_SIZE/2;
    int screen_y = world_to_screen_y(y) + CELL_SIZE/2;
    
    // Pheromone color - stronger = more orange, weaker = more yellow
    int intensity = (strength * 255) / 100;  // Assuming max strength is 100
    if (intensity > 255) intensity = 255;
    
    TPixel pheromone;
    if (strength > 50) {
        pheromone = tigrRGB(255, 100, 0);  // Strong orange
    } else if (strength > 20) {
        pheromone = tigrRGB(255, 150, 0);  // Medium orange
    } else {
        pheromone = tigrRGB(255, 200, 100);  // Faint yellow
    }
    
    // Draw pheromone as connected dots (trail-like)
    int radius = 1 + (strength / 20);  // Size based on strength
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            if (dx*dx + dy*dy <= radius*radius) {
                int px = screen_x + dx;
                int py = screen_y + dy;
                if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H) {
                    tigrPlot(screen, px, py, pheromone);
                }
            }
        }
    }
}

void clear_screen(uint32_t color) {
    if (!screen) return;
    
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    
    tigrClear(screen, tigrRGB(r, g, b));
}

void draw_world_grid() {
    if (!screen) return;
    
    // Draw subtle grid lines
    TPixel grid_color = tigrRGB(40, 50, 60);
    
    // Vertical lines
    for (int x = 0; x <= WORLD_SIZE; x++) {
        int screen_x = world_to_screen_x(x);
        for (int y = 0; y < SCREEN_H; y++) {
            if (x % 10 == 0) {  // Every 10th line is more visible
                tigrPlot(screen, screen_x, y, tigrRGB(60, 70, 80));
            } else if (x % 5 == 0) {  // Every 5th line is slightly visible
                tigrPlot(screen, screen_x, y, grid_color);
            }
        }
    }
    
    // Horizontal lines
    for (int y = 0; y <= WORLD_SIZE; y++) {
        int screen_y = world_to_screen_y(y);
        for (int x = 0; x < SCREEN_W; x++) {
            if (y % 10 == 0) {  // Every 10th line is more visible
                tigrPlot(screen, x, screen_y, tigrRGB(60, 70, 80));
            } else if (y % 5 == 0) {  // Every 5th line is slightly visible
                tigrPlot(screen, x, screen_y, grid_color);
            }
        }
    }
}

void draw_ant(int x, int y, uint32_t color) {
    if (!screen) return;
    
    int screen_x = world_to_screen_x(x) + CELL_SIZE/2;
    int screen_y = world_to_screen_y(y) + CELL_SIZE/2;
    
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    
    // Find the ant to get its direction
    float direction = 0.0f;
    for (int i = 0; i < ant_count; i++) {
        if ((int)ants[i].x == x && (int)ants[i].y == y) {
            direction = ants[i].dir;
            break;
        }
    }
    
    draw_realistic_ant(screen_x, screen_y, direction, r, g, b);
}

void draw_status_info(int ant_count_display, int food_count) {
    if (!screen) return;
    
    // Background for status
    tigrFill(screen, 0, 0, SCREEN_W, 60, tigrRGB(20, 25, 35));
    
    char status[200];
    sprintf(status, "TeenyAnts Colony: %d Ants | %d Food Remaining | ESC to quit", 
            ant_count_display, food_count);
    
    TPixel white = tigrRGB(255, 255, 255);
    tigrPrint(screen, tfont, 10, 10, white, status);
    
    // Legend
    tigrPrint(screen, tfont, 10, 25, tigrRGB(139, 69, 19), "Brown=Nest");
    tigrPrint(screen, tfont, 120, 25, tigrRGB(50, 205, 50), "Green=Food");
    tigrPrint(screen, tfont, 220, 25, tigrRGB(255, 68, 68), "Red=Exploring");
    tigrPrint(screen, tfont, 340, 25, tigrRGB(255, 215, 0), "Gold=Carrying");
    tigrPrint(screen, tfont, 460, 25, tigrRGB(255, 140, 0), "Orange=Trails");
    tigrPrint(screen, tfont, 580, 25, tigrRGB(101, 67, 33), "Brown=Tunnels");
    
    // Instructions
    tigrPrint(screen, tfont, 10, 40, tigrRGB(180, 180, 180), 
              "Watch ants create pheromone trails and organize the colony!");
}

void present_graphics() {
    if (!screen) return;
    tigrUpdate(screen);
}

// Update world state (called from main.cpp)
void update_world_cell(int x, int y, int cell_type) {
    if (x >= 0 && x < WORLD_SIZE && y >= 0 && y < WORLD_SIZE) {
        world_grid[x][y] = cell_type;
    }
}

void update_pheromone(int x, int y, int food_strength, int nest_strength) {
    if (x >= 0 && x < WORLD_SIZE && y >= 0 && y < WORLD_SIZE) {
        pheromone_food[x][y] = food_strength;
        pheromone_nest[x][y] = nest_strength;
    }
}

// Main rendering function - REMOVE EXCESSIVE DEBUG
void render_world() {
    if (!screen) return;
    
    // Clear with soil-like background
    clear_screen(0x0a0f1a);
    
    // Draw grid
    draw_world_grid();
    
    // Draw pheromone trails first (underneath everything)
    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int y = 0; y < WORLD_SIZE; y++) {
            if (pheromone_food[x][y] > 0) {
                draw_pheromone(x, y, pheromone_food[x][y]);
            }
        }
    }
    
    // Draw world cells (food will now be smaller due to draw_cell change)
    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int y = 0; y < WORLD_SIZE; y++) {
            switch (world_grid[x][y]) {
                case 1: // Food - now draws smaller due to draw_cell modification
                    draw_cell(x, y, 0x32CD32);  // Lime green
                    break;
                case 2: // Nest
                    draw_cell(x, y, 0x8B4513);  // Saddle brown
                    break;
                case 3: // Wall
                    draw_cell(x, y, 0x696969);  // Dim gray
                    break;
                case 4: // Tunnel
                    draw_cell(x, y, 0x654321);  // Dark brown
                    break;
            }
        }
    }
    
    // Draw all ants - REDUCED DEBUG (only print count, not individual positions)
    if (ant_count > 0) {
        printf("Rendering %d ants\n", ant_count);  // Less spam
    }
    
    for (int i = 0; i < ant_count; i++) {
        int screen_x = world_to_screen_x((int)ants[i].x) + CELL_SIZE/2;
        int screen_y = world_to_screen_y((int)ants[i].y) + CELL_SIZE/2;
        
        draw_realistic_ant(screen_x, screen_y, ants[i].dir, 
                          ants[i].color_r, ants[i].color_g, ants[i].color_b);
    }
    
    // Count food for status
    int food_count = 0;
    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int y = 0; y < WORLD_SIZE; y++) {
            if (world_grid[x][y] == 1) food_count++;
        }
    }
    
    // Draw status with ant count
    draw_status_info(ant_count, food_count);
    
    present_graphics();
}
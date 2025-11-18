#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>

// Core graphics functions
void init_graphics(void);
void cleanup_graphics(void);
int graphics_active(void);
void present_graphics(void);

// World rendering
void clear_screen(uint32_t color);
void draw_world_grid(void);
void draw_cell(int x, int y, uint32_t color);
void draw_pheromone(int x, int y, int strength);
void draw_ant(int x, int y, uint32_t color);
void draw_status_info(int ant_count, int food_count);
void draw_food(int screen_x, int screen_y);  // Add this line

// TeenyAnts specific functions
void add_ant(int ant_id, int x, int y, int carrying_food, int state);
void update_world_cell(int x, int y, int cell_type);
void update_pheromone(int x, int y, int food_strength, int nest_strength);
void render_world(void);
void spawn_random_food(void);
int get_world_cell(int x, int y);

#endif
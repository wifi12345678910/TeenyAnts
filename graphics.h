#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Graphics system initialization and cleanup
void init_graphics(void);
void cleanup_graphics(void);

// Graphics system status
bool graphics_active(void);
void update_graphics(void);

// Key visualization functions
void set_key_color(char keycode, int r, int g, int b);
void set_key_pressed(char keycode, bool pressed);

// Input functions
char get_key_input(void);

#ifdef __cplusplus
}
#endif

#endif // GRAPHICS_H
#include "tigr.h"
#include "graphics.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define SCREEN_W 800
#define SCREEN_H 400  // Slightly taller for better proportions
#define KEY_W 35
#define KEY_H 35
#define MAX_KEYS 50

static Tigr *screen = NULL;
static Key keys[MAX_KEYS];
static int keyCount = 0;
static bool initialized = false;
static char last_key_detected = 0;
static int last_key_time = 0;
static int frame_counter = 0;

void init_graphics(void) {
    if (initialized) return;
    
    screen = tigrWindow(SCREEN_W, SCREEN_H, "Leroy's Piano System", TIGR_FIXED);
    if (!screen) {
        printf("Failed to create window\n");
        return;
    }
    
    keyCount = 0;
    
    // PERFECTLY CENTERED keyboard layout
    int key_spacing = KEY_W + 3;
    int start_y = 160;  // Start lower to center vertically
    
    // Row 1: Numbers - PERFECTLY CENTERED
    int row1_x = (SCREEN_W - (10 * key_spacing - 3)) / 2;  // -3 to account for last spacing
    const char *row1[] = {"1","2","3","4","5","6","7","8","9","0"};
    const char row1_keys[] = {'1','2','3','4','5','6','7','8','9','0'};
    for (int i = 0; i < 10; i++) {
        addKey(row1[i], row1_keys[i], row1_x + i * key_spacing, start_y, KEY_W, false);
    }
    
    // Row 2: QWERTY - PERFECTLY CENTERED
    int row2_x = (SCREEN_W - (10 * key_spacing - 3)) / 2;
    const char *row2_labels[] = {"Q","W","E","R","T","Y","U","I","O","P"};
    const char row2_keys[] = {'q','w','e','r','t','y','u','i','o','p'};
    for (int i = 0; i < 10; i++) {
        addKey(row2_labels[i], row2_keys[i], row2_x + i * key_spacing, start_y + 45, KEY_W, true);
    }
    
    // Row 3: ASDF - PERFECTLY CENTERED
    int row3_x = (SCREEN_W - (9 * key_spacing - 3)) / 2;
    const char *row3_labels[] = {"A","S","D","F","G","H","J","K","L"};
    const char row3_keys[] = {'a','s','d','f','g','h','j','k','l'};
    for (int i = 0; i < 9; i++) {
        addKey(row3_labels[i], row3_keys[i], row3_x + i * key_spacing, start_y + 90, KEY_W, true);
    }
    
    // Row 4: ZXCV - PERFECTLY CENTERED
    int row4_x = (SCREEN_W - (7 * key_spacing - 3)) / 2;
    const char *row4_labels[] = {"Z","X","C","V","B","N","M"};
    const char row4_keys[] = {'z','x','c','v','b','n','m'};
    for (int i = 0; i < 7; i++) {
        addKey(row4_labels[i], row4_keys[i], row4_x + i * key_spacing, start_y + 135, KEY_W, true);
    }
    
    initialized = true;
    printf("Graphics initialized (%d keys)\n", keyCount);
}

void cleanup_graphics(void) {
    if (screen) {
        tigrFree(screen);
        screen = NULL;
    }
    initialized = false;
    printf("Graphics cleaned up\n");
}

bool graphics_active(void) {
    return screen && !tigrClosed(screen);
}

void set_key_color(char keycode, int r, int g, int b) {
    for (int i = 0; i < keyCount; i++) {
        if (keys[i].keycode == keycode) {
            keys[i].r = r;
            keys[i].g = g;
            keys[i].b = b;
            return;
        }
    }
}

void set_key_pressed(char keycode, bool pressed) {
    for (int i = 0; i < keyCount; i++) {
        if (keys[i].keycode == keycode) {
            keys[i].pressed = pressed;
            return;
        }
    }
}

void update_graphics(void) {
    if (!screen) return;
    
    // Beautiful gradient background
    tigrClear(screen, tigrRGB(45, 55, 75));
    
    // PERFECTLY CENTERED text layout
    int center_x = SCREEN_W / 2;
    
    // Main title - perfectly centered
    const char* title = "Leroy's Piano System";
    int title_width = strlen(title) * 6;  // tigr font is 6 pixels wide per char
    tigrPrint(screen, tfont, center_x - title_width/2, 40, tigrRGB(255, 255, 255), title);
    
    // Subtitle - perfectly centered
    const char* subtitle = "TeenyAT Assembly Platform";
    int subtitle_width = strlen(subtitle) * 6;
    tigrPrint(screen, tfont, center_x - subtitle_width/2, 65, tigrRGB(180, 180, 180), subtitle);
    
    // Instruction - perfectly centered
    const char* instruction = "Press keys to play musical notes!";
    int instruction_width = strlen(instruction) * 6;
    tigrPrint(screen, tfont, center_x - instruction_width/2, 95, tigrRGB(150, 255, 150), instruction);
    
    // Show last key - perfectly centered
    if (last_key_detected != 0) {
        char msg[50];
        sprintf(msg, "Last Key: %c", last_key_detected);
        int msg_width = strlen(msg) * 6;
        tigrPrint(screen, tfont, center_x - msg_width/2, 120, tigrRGB(255, 255, 100), msg);
    }
    
    // Draw all keys with proper highlighting
    for (int i = 0; i < keyCount; i++) {
        TPixel keyColor, borderColor, textColor;
        
        if (keys[i].pressed) {
            // HIGHLIGHTED state - use custom colors
            keyColor = tigrRGB(keys[i].r, keys[i].g, keys[i].b);
            borderColor = tigrRGB(255, 255, 255);
            textColor = tigrRGB(0, 0, 0);
        } else {
            // NORMAL state
            if (keys[i].is_piano) {
                keyColor = tigrRGB(70, 80, 100);    // Piano keys (darker)
                borderColor = tigrRGB(120, 130, 150);
            } else {
                keyColor = tigrRGB(50, 55, 65);     // Number keys (lighter)
                borderColor = tigrRGB(90, 95, 105);
            }
            textColor = tigrRGB(220, 220, 220);
        }
        
        // Draw key background
        tigrFill(screen, keys[i].x, keys[i].y, keys[i].w, keys[i].h, keyColor);
        
        // Draw key border
        tigrRect(screen, keys[i].x, keys[i].y, keys[i].w, keys[i].h, borderColor);
        
        // Center text perfectly on key
        int text_x = keys[i].x + (keys[i].w - 6) / 2;
        int text_y = keys[i].y + (keys[i].h - 8) / 2;
        tigrPrint(screen, tfont, text_x, text_y, textColor, keys[i].label);
    }
    
    // Footer - perfectly centered
    const char* footer = "Program sounds with TeenyAT assembly language!";
    int footer_width = strlen(footer) * 6;
    tigrPrint(screen, tfont, center_x - footer_width/2, SCREEN_H - 25, tigrRGB(120, 130, 140), footer);
    
    tigrUpdate(screen);
}
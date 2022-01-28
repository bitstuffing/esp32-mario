#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>

#define DEG2RAD 0.0174532925
#define PI 3.1415926535

#define DRAW_STYLE_SOLID 0
#define DRAW_STYLE_DOTTED 1

// Some color definitions
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

void draw_line_with_background(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, short style, uint16_t color, uint16_t background_color);
void draw_line(int x1, int y1, int x2, int y2, short style, uint16_t color);
void draw_rectangle(uint8_t x, uint8_t y, uint8_t end_x, uint8_t end_y, short style, uint16_t color);
void draw_circle(int x, int y, int r, short style, uint16_t color);
void draw_triangle(uint8_t x, uint8_t y, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, short style, uint16_t color);

void drawString(uint16_t x, uint16_t y, const char* text, uint16_t color);
void draw_string_rotated_90(uint16_t x, uint16_t y, const char* text, uint16_t color);
void draw_string_rotated_270(uint16_t x, uint16_t y, const char* text, uint16_t color);
void draw_string_rotated_180(uint16_t x, uint16_t y, const char* text, uint16_t color);

uint16_t* rotated_bitmap_270(uint8_t w, uint8_t h,uint16_t *bitmap);
uint16_t* rotated_bitmap_180(uint8_t w, uint8_t h,uint16_t *bitmap);
uint16_t* rotated_bitmap_90(uint8_t w, uint8_t h,uint16_t *bitmap);
uint16_t* mirror_bitmap_vertical(uint8_t w, uint8_t h,uint16_t *bitmap);
uint16_t* mirror_bitmap_horizontal(uint8_t w, uint8_t h,uint16_t *bitmap);

void circle_midpoint(int xCenter, int yCenter, int radius, uint8_t pix);
void circle_points_radious(int xCenter, int yCenter, int radius, uint16_t pix);

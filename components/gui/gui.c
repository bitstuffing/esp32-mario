/**
* This class is generated for basic drawing forms proposals and
* display bmps on screen.
* It's split from display.c because is not his main function, but
* in this rev needs that because this library draws in screen with
* ili9341_write_frame_rectangleLE
* Dev. by @bitstuffing with love
*/
#include "gui.h"

#include "../hardware/display.h" //TODO find a better way to include

#include "esp_system.h"
#include "esp_event.h"
#include "font.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

uint16_t buffer[11*320*2];

void draw_line_with_background(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, short style, uint16_t color, uint16_t background_color){

    short dx, dy;

    dx = abs(x2 - x1);
    dy = abs(y2 - y1);

    dx = dx>0?dx:5;
    dy = dy>0?dy:5;

    uint16_t* newBitmap = (uint16_t*)malloc(dx*dy*sizeof(uint16_t));
    uint16_t data = background_color;
    if (style == DRAW_STYLE_DOTTED) {
      bool dotted = false;
      for (short y = 0; y < y2; y++){
        for (short x = 0; x < x2; x++){
            newBitmap[(x)+(y*x2)] = data;
            //(x1 - x2)(y - y1) == (y1 - y2)(x - x1)
            if ( (x1 - x2)*(y - y1) == (y1 - y2)*(x - x1) ){ //if is in vector
              if(dotted){ //skip first one, for dotted effect
                newBitmap[(x)+(y*x2)] = color;
              }
              dotted = !dotted;
            }
        }
      }
    } else {
        for (short y = 0; y < y2; y++){
          for (short x = 0; x < x2; x++){
              newBitmap[(x)+(y*x2)] = data;
              //(x1 - x2)(y - y1) == (y1 - y2)(x - x1)
              if ( (x1 - x2)*(y - y1) == (y1 - y2)*(x - x1) ){ //if is in vector
                newBitmap[(x)+(y*x2)] = color;
              }
          }
        }
    }

    ili9341_write_frame_rectangleLE(x1,y1,x2,y2,newBitmap);
    //free(newBitmap);
}

static uint16_t *newBitmap = NULL;

void draw_line(int x1, int y1, int x2, int y2, short style, uint16_t color){
    printf("(%d,%d) -p- (%d,%d)",x1,y1,x2,y2);

    int dx, dy;

    dx = abs(x2 - x1);
    dy = abs(y2 - y1);

    dx = dx>0?dx:1;
    dy = dy>0?dy:1;


    if (style == DRAW_STYLE_DOTTED) {
      bool dotted = false;
      if(abs(x1-x2)>0 && abs(y1-y2)>0){
        newBitmap = (uint16_t*)malloc(dx*sizeof(uint16_t));
        newBitmap[0] = color;
        for (short y = 0; y < y2; y++){
          for (short x = 0; x < x2; x++){
              //(x1 - x2)(y - y1) == (y1 - y2)(x - x1)
              if ( (x1 - x2)*(y - y1) == (y1 - y2)*(x - x1) ){ //if is in vector
                if(dotted){ //skip first one, for dotted effect
                  ili9341_write_frame_rectangleLE(x+x1,y+y1,1,1,newBitmap);
                }
                dotted = !dotted;
              }
          }
        }
      } //TODO else's time
    } else {
        newBitmap = (uint16_t*)malloc(dx*sizeof(uint16_t));
        newBitmap[0] = color;
        if(abs(x1-x2)>0 && abs(y1-y2)>0){
            int yme = y1<y2?y1:y2;
            int yma = y1>y2?y1:y2;
            int xme = x1<x2?x1:x2;
            int xma = x1>x2?x1:x2;
            for (int y = yme; y < yma; y++){
                for (int x = xme; x < xma; x++){
                    /*//(x1 - x2)(y - y1) == (y1 - y2)(x - x1)
                    if ( (x1 - x2)*(y - y1) == (y1 - y2)*(x - x1)){ //if is in vector
                      //ili9341_write_frame_rectangleLE(x+x1,y+y1,1,1,newBitmap);
                      ili9341_write_frame_rectangleLE(x,y,1,1,newBitmap);
                      //if((x1 - x2)*(y - y1) == (y1 - y2)*(x - x1))
                      //  break;
                    }*/

                    int dxc = x - x1;
                    int dyc = y - y1;

                    int dxl = x2 - x1;
                    int dyl = y2 - y1;

                    int cross = dxc * dyl - dyc * dxl;
                    if(abs(cross)< abs(dxl) || abs(cross)< abs(dyl)){ //should be cross==0 but... the pixels world
                      ili9341_write_frame_rectangleLE(x,y,1,1,newBitmap);
                    }

                }
            }
        } // it's faster call ili9341_write_frame_rectangleLE one time with a bigger buffer in case of horizontal and vertical lines
        else if(abs(x1-x2)>0){ //horizontal
            //for (int x = 0; x <= abs(x1-x2); x++){
            //  ili9341_write_frame_rectangleLE(x+x1,y1,1,1,newBitmap);
            //}
            newBitmap = (uint16_t*)malloc(dx*sizeof(uint16_t));
            for (short x = 0; x <= dx; x++){
                newBitmap[x] = color;
            }
            int x = x1<x2?x1:x2;
            int y = y1<y2?y1:y2;
            ili9341_write_frame_rectangleLE(x,y,dx+1,1,newBitmap);
            free(newBitmap);
        }else if(abs(y1-y2)>0){ //vertical
            //for (int y = 0; y <= abs(y1-y2); y++){
            //  ili9341_write_frame_rectangleLE(x1,y+y1,1,1,newBitmap);
            //}
            newBitmap = (uint16_t*)malloc(dy*sizeof(uint16_t));
            for (short y = 0; y <= dy; y++){
                newBitmap[y] = color;
            }
            int x = x1<x2?x1:x2;
            int y = y1<y2?y1:y2;
            ili9341_write_frame_rectangleLE(x,y,1,dy+1,newBitmap);
        }else{ //point
          ili9341_write_frame_rectangleLE(x1,y1,1,1,newBitmap);
        }
    }
}

void draw_rectangle(uint8_t x, uint8_t y, uint8_t x2, uint8_t y2, short style, uint16_t color){
    draw_line(x,y,x2,y,style,color);
    draw_line(x2,y,x2,y2,style,color);
    draw_line(x,y2,x2,y2,style,color);
    draw_line(x,y,x,y2,style,color);
}

void draw_triangle(uint8_t x, uint8_t y, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, short style, uint16_t color){
    draw_line(x,y,x3,y3,style,color);
    draw_line(x3,y3,x2,y2,style,color);
    draw_line(x2,y2,x,y,style,color);
}

void draw_point(uint8_t color,uint8_t x,uint8_t y){
    draw_line(x, y, x ,y, DRAW_STYLE_SOLID, color);
}

void draw_circle(int x, int y, int r, short style, uint16_t color){
      double i, angle, x1, y1;
      for(i = 0; i < 360; i += 0.1){
            angle = i;
            x1 = r * cos(angle * PI / 180);
            y1 = r * sin(angle * PI / 180);
            draw_line(x + x1, y + y1, x + x1,y + y1, style, color);
      }
}

void circle_points(int cx, int cy, int x, int y, uint16_t pix){
    if (x == 0) {
        draw_point(pix, cx, cy + y);
        draw_point(pix, cx, cy - y);
        draw_point(pix, cx + y, cy);
        draw_point(pix, cx - y, cy);
    } else if (x == y) {
        draw_point(pix, cx + x, cy + y);
        draw_point(pix, cx - x, cy + y);
        draw_point(pix, cx + x, cy - y);
        draw_point(pix, cx - x, cy - y);
    } else if (x < y) {
        draw_point(pix, cx + x, cy + y);
        draw_point(pix, cx - x, cy + y);
        draw_point(pix, cx + x, cy - y);
        draw_point(pix, cx - x, cy - y);
        draw_point(pix, cx + y, cy + x);
        draw_point(pix, cx - y, cy + x);
        draw_point(pix, cx + y, cy - x);
        draw_point(pix, cx - y, cy - x);
    }
}

void circle_points_radious(int xCenter, int yCenter, int radius, uint16_t pix){
    int x, y, r2;

    r2 = radius * radius;
    draw_point(pix, xCenter, yCenter + radius);
    draw_point(pix, xCenter, yCenter - radius);
    draw_point(pix, xCenter + radius, yCenter);
    draw_point(pix, xCenter - radius, yCenter);

    y = radius;
    x = 1;
    y = (int) (sqrt(r2 - 1) + 0.5);
    while (x < y) {
        draw_point(pix, xCenter + x, yCenter + y);
        draw_point(pix, xCenter + x, yCenter - y);
        draw_point(pix, xCenter - x, yCenter + y);
        draw_point(pix, xCenter - x, yCenter - y);
        draw_point(pix, xCenter + y, yCenter + x);
        draw_point(pix, xCenter + y, yCenter - x);
        draw_point(pix, xCenter - y, yCenter + x);
        draw_point(pix, xCenter - y, yCenter - x);
        x += 1;
        y = (int) (sqrt(r2 - x*x) + 0.5);
    }
    if (x == y) {
        draw_point(pix, xCenter + x, yCenter + y);
        draw_point(pix, xCenter + x, yCenter - y);
        draw_point(pix, xCenter - x, yCenter + y);
        draw_point(pix, xCenter - x, yCenter - y);
    }
}

void circle_midpoint(int xCenter, int yCenter, int radius, uint8_t pix){

    int x = 0;
    int y = radius;
    int p = (5 - radius*4)/4;

    circle_points(xCenter, yCenter, x, y, pix);
    while (x < y) {
        x++;
        if (p < 0) {
            p += 2*x+1;
        } else {
            y--;
            p += 2*(x-y)+1;
        }
        circle_points(xCenter, yCenter, x, y, pix);
    }
}

uint16_t* mirror_bitmap_vertical(uint8_t w, uint8_t h,uint16_t *bitmap){
	uint16_t data = 0;
  uint16_t* newBitmap = (uint16_t*)malloc(w*h*sizeof(uint16_t));
  for (short y = 0; y < h; y++){
    for (short x = 0; x < w; x++){
        data = bitmap[(x)+(y*w)];
        newBitmap[(w-1-x)+(y*w)] = data;
    }
  }
  free(bitmap);
  return newBitmap;
}

uint16_t* mirror_bitmap_horizontal(uint8_t w, uint8_t h,uint16_t *bitmap){
	uint16_t* newBitmap = rotated_bitmap_180(w,h,bitmap);
  free(bitmap);
	newBitmap = mirror_bitmap_vertical(w,h,newBitmap);
  return newBitmap;
}


uint16_t* rotated_bitmap_90(uint8_t w, uint8_t h,uint16_t *bitmap){
  uint16_t data = 0;
  uint16_t* newBitmap = (uint16_t*)malloc(w*h*sizeof(uint16_t));
  for (short y = 0; y < h; y++){
    for (short x = 0; x < w; x++){
        data = bitmap[(x)+(y*w)];
        newBitmap[(y)+(x*h)] = data;
    }
  }
	newBitmap = mirror_bitmap_vertical(h,w,newBitmap);
  return newBitmap;
}

uint16_t* rotated_bitmap_180(uint8_t w, uint8_t h,uint16_t *bitmap){
  uint16_t data = 0;
  uint16_t* newBitmap = (uint16_t*)malloc(w*h*sizeof(uint16_t));
  for (int y = 0; y < h; y++){
    for (int x = 0; x < w; x++){
        data = bitmap[(x)+(y*w)];
        newBitmap[(w*h-1)-((x)+(y*w))] = data;
    }
  }
  //free(bitmap);
  return newBitmap;
}

uint16_t* rotated_bitmap_270(uint8_t w, uint8_t h,uint16_t *bitmap){
  uint16_t* newBitmap = rotated_bitmap_90(w,h,bitmap);
  newBitmap = mirror_bitmap_horizontal(h,w,newBitmap);
  newBitmap = mirror_bitmap_vertical(h,w,newBitmap);
  return newBitmap;
}

void drawString(uint16_t x, uint16_t y, const char* text, uint16_t color){

	uint16_t background = 0xffff;

	uint16_t width = FntLineWidth(text, 256);
	uint16_t height = FntLineHeight();
	set_drawbuffer(buffer,  width, height);
	for(int i = 0; i<width*height*2; i++){
		buffer[i] = background;
	}
	draw_text(0,height-2,text,256,color);

	ili9341_write_frame_rectangleLE(x,y,width,height,buffer);
}

void draw_string_rotated_90(uint16_t x, uint16_t y, const char* text, uint16_t color){

	uint16_t background = 0xffff;

	uint16_t width = FntLineWidth(text, 256);
	uint16_t height = FntLineHeight();
	set_drawbuffer(buffer,  width, height);
	for(int i = 0; i<width*height*2; i++){
		buffer[i] = background;
	}
	draw_text(0,height-2,text,256,color);

	uint16_t* newBuffer = rotated_bitmap_90(width,height,buffer);
	ili9341_write_frame_rectangleLE(x,y,height,width,newBuffer);
  free(newBuffer);
}

void draw_string_rotated_180(uint16_t x, uint16_t y, const char* text, uint16_t color){

	uint16_t background = 0xffff;

	uint16_t width = FntLineWidth(text, 256);
	uint16_t height = FntLineHeight();
	set_drawbuffer(buffer,  width, height);
	for(int i = 0; i<width*height*2; i++){
		buffer[i] = background;
	}
	draw_text(0,height-2,text,256,color);

	uint16_t* newBuffer = rotated_bitmap_180(width,height,buffer);
	ili9341_write_frame_rectangleLE(x,y,width,height,newBuffer);
  free(newBuffer);
}

void draw_string_rotated_270(uint16_t x, uint16_t y, const char* text, uint16_t color){

	uint16_t background = 0xffff;

	uint16_t width = FntLineWidth(text, 256);
	uint16_t height = FntLineHeight();
	set_drawbuffer(buffer,  width, height);
	for(int i = 0; i<width*height*2; i++){
		buffer[i] = background;
	}
	draw_text(0,height-2,text,256,color);

	uint16_t* newBuffer = rotated_bitmap_270(width,height,buffer);
	ili9341_write_frame_rectangleLE(x,y,height,width,newBuffer);
  free(newBuffer);
}

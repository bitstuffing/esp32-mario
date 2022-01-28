/*
   ESP32-Mario Brick

   Dev. by @bitstuffing

   More information at bitstuffing.github.io

   All the code in all files are licensed under CC 4.0
*/
#include <stdio.h>
#include <math.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "gui.h"

//hardware
#include "display.h"
#include "images.h"

//#define pgm_read_byte(addr)   (*(const unsigned char *)(addr))

void logic(){
  printf("Hello world!\n");

  /* Print chip information */
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  char tempstring[512];
  sprintf(tempstring, "This is %s chip with %d CPU core(s), WiFi%s%s, ",
          CONFIG_IDF_TARGET,
          chip_info.cores,
          (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
          (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

  //drawString(20,50,tempstring,0x0000); //TODO sdkconfig????
  sprintf(tempstring, "silicon revision %d, ", chip_info.revision);
  drawString(20,80,tempstring,0x0000);


  sprintf(tempstring, "%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
          (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
  drawString(20,110,tempstring,0x0000);

  sprintf(tempstring, "Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
  drawString(20,140,tempstring,0x0000);

  for (int i = 3; i >= 0; i--) {
      sprintf(tempstring, "Delay for %d seconds...\n", i);
      drawString(20,200,tempstring,0x0000);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

}

void tests(){

      //write an example image
      uint16_t *new_image = NULL;

      //ili9341_write_frame_rectangleLE(170,0,107,165,image_left_eye);

      //ili9341_write_frame_rectangleLE(0,0,107,165,image_left_eye);


      //draw_rectangle(100,100, 200, 200, DRAW_STYLE_SOLID, WHITE);
      //draw_line(50,50,50,80,DRAW_STYLE_SOLID,WHITE);

      char tempstring[60];

      for(int i=0;i<100;i++){
        sprintf(tempstring, "this is an eye %d",i);
        //sprintf(tempstring, "this is an eye");
        drawString(100,200,tempstring,BLACK);
        draw_string_rotated_270(20,20,tempstring,BLACK);
        draw_string_rotated_180(140,20,tempstring,BLACK);
        draw_string_rotated_90(240,60,tempstring,BLACK);

        ili9341_write_frame_rectangleLE(0,0,107,165,image_left_eye);

        new_image = rotated_bitmap_90(107,165,image_left_eye);
        ili9341_write_frame_rectangleLE(0,0,165,107,new_image);

        //uint16_t *new_image2 = rotated_bitmap_90(165,107,new_image);
        uint16_t *new_image2 = rotated_bitmap_180(165,107,image_left_eye);
        free(new_image);
        ili9341_write_frame_rectangleLE(0,110,107,165,new_image2);

        //uint16_t *new_image3 = rotated_bitmap_90(107,165,new_image2);
        uint16_t *new_image3 = rotated_bitmap_270(107,165,image_left_eye);
        free(new_image2);
        ili9341_write_frame_rectangleLE(0,110,165,107,new_image3);


        //ili9341_write_frame_rectangleLE(155,110,165,107,new_image);

        free(new_image3);
      }



      //draw_circle(150,150,45,DRAW_STYLE_SOLID,WHITE);
      draw_rectangle(50,50,230,230, DRAW_STYLE_SOLID, BLUE);

      for (short i=0; i<100; i++){
        //draw_line(100, 100, 100, 100, DRAW_STYLE_SOLID, WHITE);

        draw_triangle(50+i, 100, 100, 50+i, 100+i, 100+i, DRAW_STYLE_SOLID, MAGENTA);

        //draw_triangle(50, 100, 100, 50, 100, 100, DRAW_STYLE_SOLID, MAGENTA);

        //circleMidpoint(25+i,25+i,5,WHITE);
        //circlePointsRadious(100+i,100+i,25,WHITE);
      }
}

void app_main(void){
    //init hardware
    nvs_flash_init();

    // Display
    ili9341_init();
    ili9341_prepare();

    ili9341_clear(BLACK); //fill with black

    //turn on blue led
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 1);



    logic();

    tests();



    //logic

    ili9341_poweroff();
    gpio_set_level(GPIO_NUM_2, 0);
    printf("Done!\n");
    fflush(stdout);
}

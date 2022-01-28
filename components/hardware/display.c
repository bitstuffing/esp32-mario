/**
* This file is bassicaly and SPI display controller for ESP-IDF based controllers.
* It has been adapted from
* https://github.com/espressif/esp-idf/blob/master/examples/peripherals/spi_master/lcd/main/spi_master_example_main.c
* In a summary, you're able to draw buffers in a ILI9341 display line by line.
* It has some limitations but it has a good performance for framebuffers and ESP32.
* Dev. by @bitstuffing with love
*/
#include "display.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/rtc_io.h"

#include <string.h>

#define SPI_TRANSACTION_COUNT (4)
static spi_transaction_t trans[SPI_TRANSACTION_COUNT];
static spi_device_handle_t spi;

#define LINE_BUFFERS (2)
#define LINE_COUNT (5)
uint16_t* line[LINE_BUFFERS];
QueueHandle_t spi_queue;
QueueHandle_t line_buffer_queue;
SemaphoreHandle_t line_semaphore;
SemaphoreHandle_t spi_empty;
SemaphoreHandle_t spi_count_semaphore;
int firsttime = 1;

bool isBackLightIntialized = false;

static uint16_t* line_buffer_get(){
    uint16_t* buffer;
    if (xQueueReceive(line_buffer_queue, &buffer, 1000 / portTICK_RATE_MS) != pdTRUE){
        abort();
    }

    return buffer;
}

void line_buffer_put(uint16_t* buffer){
    if (xQueueSend(line_buffer_queue, &buffer, 1000 / portTICK_RATE_MS) != pdTRUE){
        abort();
    }
}

static void spi_task(void *arg){
    printf("%s: Entered.\n", __func__);

    while(1){
        // Ensure only LCD transactions are pulled
        if(xSemaphoreTake(spi_count_semaphore, portMAX_DELAY) == pdTRUE ){
            spi_transaction_t* t;

            esp_err_t ret = spi_device_get_trans_result(spi, &t, portMAX_DELAY);
            assert(ret==ESP_OK);

            int dc = (int)t->user & 0x80;
            if(dc){
                xSemaphoreGive(line_semaphore);
                line_buffer_put(t->tx_buffer);
            }

            if(xQueueSend(spi_queue, &t, portMAX_DELAY) != pdPASS){
                abort();
            }

            if(uxQueueMessagesWaiting(spi_queue) >= SPI_TRANSACTION_COUNT){
                xSemaphoreGive(spi_empty);
            }
        }else{
            printf("%s: xSemaphoreTake failed.\n", __func__);
        }
    }

    printf("%s: Exiting.\n", __func__);
    vTaskDelete(NULL);

    while (1) {}
}

static void spi_initialize(){
    spi_queue = xQueueCreate(SPI_TRANSACTION_COUNT, sizeof(void*));
    if(!spi_queue) abort();

    line_buffer_queue = xQueueCreate(LINE_BUFFERS, sizeof(void*));
    if(!line_buffer_queue) abort();

    line_semaphore = xSemaphoreCreateCounting(LINE_BUFFERS, LINE_BUFFERS);
    if (!line_semaphore) abort();

    spi_empty = xSemaphoreCreateBinary();
    if(!spi_empty) abort();

    xSemaphoreGive(spi_empty);

    spi_count_semaphore = xSemaphoreCreateCounting(SPI_TRANSACTION_COUNT, 0);
    if (!spi_count_semaphore) abort();

    xTaskCreatePinnedToCore(&spi_task, "spi_task", 1024 + 768, NULL, 5, NULL, 1);
}

static spi_transaction_t* spi_get_transaction(){
    spi_transaction_t* t;
    xQueueReceive(spi_queue, &t, portMAX_DELAY);

    memset(t, 0, sizeof(*t));

    return t;
}

static void spi_put_transaction(spi_transaction_t* t){
    t->rx_buffer = NULL;
    t->rxlength = t->length;

    if (t->flags & SPI_TRANS_USE_TXDATA){
        t->flags |= SPI_TRANS_USE_RXDATA;
    }

    if (uxSemaphoreGetCount(spi_empty) > 0){
        if(xSemaphoreTake(spi_empty, portMAX_DELAY) != pdTRUE){
            abort();
        }
    }

    esp_err_t ret = spi_device_queue_trans(spi, t, portMAX_DELAY);
    assert(ret==ESP_OK);

    xSemaphoreGive(spi_count_semaphore);
}


//Send a command to the ILI9341. Uses spi_device_transmit, which waits until the transfer is complete.
static void ili_cmd(const uint8_t cmd){
    spi_transaction_t* t = spi_get_transaction();

    t->length = 8;                     //Command is 8 bits
    t->tx_data[0] = cmd;               //The data is the cmd itself
    t->user = (void*)0;                //D/C needs to be set to 0
    t->flags = SPI_TRANS_USE_TXDATA;

    spi_put_transaction(t);
}

//Send data to the ILI9341. Uses spi_device_transmit, which waits until the transfer is complete.
static void ili_data(const uint8_t *data, int len){
    if (len){
        spi_transaction_t* t = spi_get_transaction();

        if (len < 5){
            for (int i = 0; i < len; ++i)
            {
                t->tx_data[i] = data[i];
            }
            t->length = len * 8;                 //Len is in bytes, transaction length is in bits.
            t->user = (void*)1;                //D/C needs to be set to 1
            t->flags = SPI_TRANS_USE_TXDATA;
        }else{
            t->length = len * 8;                 //Len is in bytes, transaction length is in bits.
            t->tx_buffer = data;               //Data
            t->user = (void*)1;                //D/C needs to be set to 1
            t->flags = 0; //SPI_TRANS_USE_TXDATA;
        }

        spi_put_transaction(t);
    }
}

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
static void ili_spi_pre_transfer_callback(spi_transaction_t *t){
    int dc=(int)t->user & 0x01;
    gpio_set_level(LCD_PIN_NUM_DC, dc);
}

//Initialize the display
static void ili_init(){
    int cmd = 0;

    //Initialize non-SPI GPIOs
    gpio_set_direction(LCD_PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(LCD_PIN_NUM_BCKL, GPIO_MODE_OUTPUT);

    //Send all the commands
    while (ili_init_cmds[cmd].databytes != 0xff){
        ili_cmd(ili_init_cmds[cmd].cmd);

        int len = ili_init_cmds[cmd].databytes & 0x7f;
        if (len) ili_data(ili_init_cmds[cmd].data, len);

        if (ili_init_cmds[cmd].databytes & 0x80){
            vTaskDelay(100 / portTICK_RATE_MS);
        }

        cmd++;
    }
}


void send_reset_drawing(int left, int top, int width, int height){
    ili_cmd(0x2A);

    const uint8_t data1[] = { (left) >> 8, (left) & 0xff, (left + width - 1) >> 8, (left + width - 1) & 0xff };
    ili_data(data1, 4);

    ili_cmd(0x2B);          //Page address set

    const uint8_t data2[] = { top >> 8, top & 0xff, (top + height - 1) >> 8, (top + height - 1) & 0xff };
    ili_data(data2, 4);

    ili_cmd(0x2C);           //memory write
}

void send_continue_wait(){
    if(xSemaphoreTake(spi_empty, 1000 / portTICK_RATE_MS) != pdTRUE ){
        abort();
    }
}

void send_continue_line(uint16_t *line, int width, int lineCount){
    spi_transaction_t* t;

    t = spi_get_transaction();

    t->tx_data[0] = 0x3C;   //memory write continue
    t->length = 8;
    t->user = (void*)0;
    t->flags = SPI_TRANS_USE_TXDATA;

    spi_put_transaction(t);

    t = spi_get_transaction();

    t->length = width * 2 * lineCount * 8;
    t->tx_buffer = line;
    t->user = (void*)0x81;
    t->flags = 0;

    spi_put_transaction(t);
}

/*
* Could be in a different file but... it's a display with a backlight, it logic
* all is inside the same driver, but it's not ussual.
* Note: In esp-idf v3.0, settings clock speed to 80Mhz will causes
* the LCD controller to malfunction after a soft-reset.
*/
static void backlight_init(){

    // (duty range is 0 ~ ((2**bit_num)-1)

    //configure timer0
    ledc_timer_config_t ledc_timer;
    memset(&ledc_timer, 0, sizeof(ledc_timer));

    ledc_timer.bit_num = LEDC_TIMER_13_BIT; //set timer counter bit number
    ledc_timer.freq_hz = 5000;              //set frequency of pwm
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;   //timer mode,
    ledc_timer.timer_num = LEDC_TIMER_0;    //timer index

    ledc_timer_config(&ledc_timer);

    //set the configuration
    ledc_channel_config_t ledc_channel;
    memset(&ledc_channel, 0, sizeof(ledc_channel));

    //set LEDC channel 0
    ledc_channel.channel = LEDC_CHANNEL_0;
    //set the duty for initialization.(duty range is 0 ~ ((2**bit_num)-1)
    ledc_channel.duty = (LCD_BACKLIGHT_ON_VALUE) ? 0 : DUTY_MAX;
    //GPIO number
    ledc_channel.gpio_num = LCD_PIN_NUM_BCKL;
    //GPIO INTR TYPE, as an example, we enable fade_end interrupt here.
    ledc_channel.intr_type = LEDC_INTR_FADE_END;
    //set LEDC mode, from ledc_mode_t
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    //set LEDC timer source, if different channel use one timer,
    //the frequency and bit_num of these channels should be the same
    ledc_channel.timer_sel = LEDC_TIMER_0;

    ledc_channel_config(&ledc_channel);

    //initialize fade service.
    ledc_fade_func_install(0);

    // duty range is 0 ~ ((2**bit_num)-1)
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (LCD_BACKLIGHT_ON_VALUE) ? DUTY_MAX : 0, 500);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);

    isBackLightIntialized = true;
}

void backlight_percentage_set(int value){
    int duty = DUTY_MAX * (value * 0.01f);
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, 500);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
}

void ili9341_init(){
    // Init
    spi_initialize();

    // Line buffers
    const size_t lineSize = DISPLAY_WIDTH * LINE_COUNT * sizeof(uint16_t);
    for (int x = 0; x < LINE_BUFFERS; x++){
        line[x] = heap_caps_malloc(lineSize, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (!line[x]) abort();

        printf("%s: line_buffer_put(%p)\n", __func__, line[x]);
        line_buffer_put(line[x]);
    }

    // Initialize transactions
    for (int x = 0; x < SPI_TRANSACTION_COUNT; x++){
        void* param = &trans[x];
        xQueueSend(spi_queue, &param, portMAX_DELAY);
    }

    // Initialize SPI
    esp_err_t ret;
    //spi_device_handle_t spi;
    spi_bus_config_t buscfg;
		memset(&buscfg, 0, sizeof(buscfg));

    buscfg.miso_io_num = SPI_PIN_NUM_MISO;
    buscfg.mosi_io_num = SPI_PIN_NUM_MOSI;
    buscfg.sclk_io_num = SPI_PIN_NUM_CLK;
    buscfg.quadwp_io_num=-1;
    buscfg.quadhd_io_num=-1;

    spi_device_interface_config_t devcfg;
		memset(&devcfg, 0, sizeof(devcfg));

    devcfg.clock_speed_hz = LCD_SPI_CLOCK_RATE;
    devcfg.mode = 0; //SPI mode 0
    devcfg.spics_io_num = LCD_PIN_NUM_CS; //CS pin
    devcfg.queue_size = 7; //I want to be able to queue 7 transactions at a time
    devcfg.pre_cb = ili_spi_pre_transfer_callback;  //Specify pre-transfer callback to handle D/C line
    devcfg.flags = SPI_DEVICE_NO_DUMMY; //SPI_DEVICE_HALFDUPLEX;

    //Initialize the SPI bus
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret==ESP_OK);

    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    assert(ret==ESP_OK);

    //Initialize the LCD
	  printf("LCD: calling ili_init.\n");
    ili_init();

	  printf("LCD: calling backlight_init.\n");
    backlight_init();

    printf("LCD Initialized (%d Hz).\n", LCD_SPI_CLOCK_RATE);
}

void ili9341_poweroff(){
    esp_err_t err = ESP_OK;

    // fade off backlight, it's prettier like turn off directly
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (LCD_BACKLIGHT_ON_VALUE) ? 0 : DUTY_MAX, 100);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_WAIT_DONE);

    // Disable LCD panel
    int cmd = 0;
    while (ili_sleep_cmds[cmd].databytes != 0xff){

        ili_cmd(ili_sleep_cmds[cmd].cmd);
        ili_data(ili_sleep_cmds[cmd].data, ili_sleep_cmds[cmd].databytes & 0x7f);
        if (ili_sleep_cmds[cmd].databytes & 0x80)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        cmd++;
    }

    err = rtc_gpio_init(LCD_PIN_NUM_BCKL);
    if (err != ESP_OK){
        abort();
    }
    err = rtc_gpio_set_direction(LCD_PIN_NUM_BCKL, RTC_GPIO_MODE_OUTPUT_ONLY);
    if (err != ESP_OK){
        abort();
    }
    err = rtc_gpio_set_level(LCD_PIN_NUM_BCKL, LCD_BACKLIGHT_ON_VALUE ? 0 : 1);
    if (err != ESP_OK){
        abort();
    }
}

void ili9341_prepare(){
    // Return use of backlight pin
    esp_err_t err = rtc_gpio_deinit(LCD_PIN_NUM_BCKL);
    if (err != ESP_OK){
        abort();
    }
}

void ili9341_write_frame_rectangle(short left, short top, short width, short height, uint16_t* buffer){
    short y;

    if (left < 0 || top < 0) abort();
    if (width < 1 || height < 1) abort();

    //xTaskToNotify = xTaskGetCurrentTaskHandle();

    send_reset_drawing(left, top, width, height);

    if (buffer == NULL){
        // clear the buffer
        for (int i = 0; i < LINE_BUFFERS; ++i){
            memset(line[i], 0, DISPLAY_WIDTH * sizeof(uint16_t) * LINE_COUNT);
        }

        // clear the screen
        send_reset_drawing(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

        for (y = 0; y < DISPLAY_HEIGHT; y += LINE_COUNT){
            uint16_t* line_buffer = line_buffer_get();
            send_continue_line(line_buffer, DISPLAY_WIDTH, LINE_COUNT);
        }
    }else{
        uint16_t* line_buffer = line_buffer_get();

        for (y = 0; y < height; y++){
            memcpy(line_buffer, buffer + y * width, width * sizeof(uint16_t));
            send_continue_line(line_buffer, width, 1);
        }
    }

    send_continue_wait();
}

void ili9341_clear(uint16_t color){

    send_reset_drawing(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // clear the buffer
    for (int i = 0; i < LINE_BUFFERS; ++i){
        for (int j = 0; j < DISPLAY_WIDTH * LINE_COUNT; ++j){
            line[i][j] = color;
        }
    }

    // clear the screen
    send_reset_drawing(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    for (int y = 0; y < DISPLAY_HEIGHT; y += LINE_COUNT){
        uint16_t* line_buffer = line_buffer_get();
        send_continue_line(line_buffer, DISPLAY_WIDTH, LINE_COUNT);
    }

    send_continue_wait();
}

void ili9341_write_frame_rectangleLE(short left, short top, short width, short height, uint16_t* buffer){
    if (left < 0 || top < 0) abort();
    if (width < 1 || height < 1) abort();

    //xTaskToNotify = xTaskGetCurrentTaskHandle();

    send_reset_drawing(left, top, width, height);

    if (buffer == NULL){
        // clear the buffer
        for (int i = 0; i < LINE_BUFFERS; ++i){
            memset(line[i], 0, DISPLAY_WIDTH * sizeof(uint16_t) * LINE_COUNT);
        }

        // clear the screen
        send_reset_drawing(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

        for (short y = 0; y < DISPLAY_HEIGHT; y += LINE_COUNT){
            uint16_t* line_buffer = line_buffer_get();
            send_continue_line(line_buffer, DISPLAY_WIDTH, LINE_COUNT);
        }
    }else{
        for (short y = 0; y < height; y++){
            uint16_t* line_buffer = line_buffer_get();

            for (int i = 0; i < width; ++i){
                uint16_t pixel = buffer[y * width + i];
                line_buffer[i] = pixel << 8 | pixel >> 8;
            }
            send_continue_line(line_buffer, width, 1);
        }
    }

    send_continue_wait();
}

int is_backlight_initialized(){
    return isBackLightIntialized;
}

void display_drain_spi(){
    if(xSemaphoreTake(spi_empty, 1000 / portTICK_RATE_MS) != pdTRUE ){
       abort();
    }
}

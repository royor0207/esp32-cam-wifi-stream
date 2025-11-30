/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define SCL_IO_PIN CONFIG_I2C_MASTER_SCL
#define SDA_IO_PIN CONFIG_I2C_MASTER_SDA
#define MASTER_FREQUENCY CONFIG_I2C_MASTER_FREQUENCY
#define PORT_NUMBER -1
#define LENGTH 48

/*---------------------------------------------------------------------------------------------*/
/*-----------------------------------------flash-test------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
#include "driver/ledc.h"
#include "esp_mac.h"

/*#define LED_GPIO GPIO_NUM_4

void flash_led_init()
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_10_BIT,              //0 ~ 1023
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LED_GPIO,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

void flash_set_brightness(int duty)                         //0 ~ 1023
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void flash_task(){
    int intensity=0;
    int flag=0;
    while (1) {
        flash_set_brightness(intensity);
        vTaskDelay(pdMS_TO_TICKS(15));


        if(flag==0){
            if(intensity==600){
                flag=1;
                continue;
            }
            intensity++;
        }else{
            if(intensity==0){
                flag=0;
                continue;
            }
            intensity--;
        }
    }
}*/

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------camera-initiation-------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
//modified  from  https://github.com/espressif/esp32-camera/blob/master/README.md

#include "esp_camera.h"

#define CAM_PIN_PWDN    32  //power down is not used
#define CAM_PIN_RESET   -1  //software reset will be performed
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

static camera_config_t camera_config = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_1,
    .ledc_channel = LEDC_CHANNEL_1,

    .pixel_format = PIXFORMAT_JPEG,             //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_UXGA,               //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12,                         //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 2,                              //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY         //CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

esp_err_t camera_init(){
    
    //power up the camera if PWDN pin is defined
    if(CAM_PIN_PWDN != -1){
        gpio_set_direction(CAM_PIN_PWDN, GPIO_MODE_OUTPUT);
        gpio_set_pull_mode(CAM_PIN_PWDN, GPIO_PULLDOWN_ONLY);
    }

    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE("Camera", "Camera Init Failed");
        return err;
    }
    
    return ESP_OK;
}

#include "sys/socket.h"
#include "netinet/in.h"
#include "TCP_Server.h"

void camera_task(void* arg){
    if(ESP_OK != camera_init()) {
        return;
    }

    int sock=TCP_init();

    while (1)
    {
        ESP_LOGI("Camera_TASK", "Taking picture...");
        camera_fb_t *pic = esp_camera_fb_get();

        send_image(sock,pic->buf,pic->len);

        ESP_LOGI("Camera_TASK", "Picture taken! Its size was: %zu bytes", pic->len);
        esp_camera_fb_return(pic);

        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/

/*static void disp_buf(uint8_t *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}*/

/*---------------------------------------------------------------------------------------------*/
/*-------------------------------------------main----------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/

#include "esp_heap_caps.h"

//check psram free size
void print_psram_free_size(void){
    ESP_LOGI("Heap_caps", "Heap_caps info: DRAM free: %u, PSRAM free: %u",
         heap_caps_get_free_size(MALLOC_CAP_8BIT),
         heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void app_main(void)
{

    /*flash_led_init();
    TaskHandle_t Flash_TaskHandle;
    xTaskCreate(flash_task,"FLASH_TASK",1024,NULL,5,&Flash_TaskHandle);
    vTaskSuspend(Flash_TaskHandle);*/

    print_psram_free_size();
    
    TaskHandle_t Camera_TaskHandle;
    xTaskCreate(camera_task,"Camera_TASK",8192,NULL,3,&Camera_TaskHandle);
    print_psram_free_size();

}


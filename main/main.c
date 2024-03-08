/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "led_strip.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "led.h"
#include "espnow.h"
#include "config.h"

static const char *TAG = "main";
static uint8_t s_button_state = 1;
static uint8_t is_receiver = 0;


void setup_pins() {
    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_reset_pin(RECEIVER_GPIO);
    gpio_set_direction(RECEIVER_GPIO, GPIO_MODE_INPUT);
}

void init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}


void app_main(void) {
    esp_err_t ret;
    uint8_t base_mac_addr[6] = {0};

    configure_led();
    setup_pins();
    init_nvs();

    is_receiver = !gpio_get_level(RECEIVER_GPIO);

    ESP_ERROR_CHECK(esp_read_mac(base_mac_addr, ESP_MAC_WIFI_STA));

    ESP_LOGI(TAG, "Device is %s", is_receiver ? "receiver" : "sender");
    ESP_LOGI(TAG, "MAC: %x%x%x%x%x%x", base_mac_addr[0], base_mac_addr[1], base_mac_addr[2], base_mac_addr[3], base_mac_addr[4], base_mac_addr[5]);

    if (WL_PROTO == WL_PROTO_ESPNOW) {
        if (espnow_init() != 0) {
            ESP_LOGE(TAG, "Error initializing");
            led_set_fault();
            return;
        }
    } else {
        ESP_LOGE(TAG, "Unsupported wireless protocol");
        return;
    }

    ESP_LOGI(TAG, "Initialized");
    led_set_ready(is_receiver);

    while (1) {
        if (gpio_get_level(BUTTON_GPIO) != s_button_state) {
            if (gpio_get_level(BUTTON_GPIO)) {
                led_set_ready();
            } else {
                led_set_active();
                espnow_send();
                led_set_ready();
            }
            s_button_state = gpio_get_level(BUTTON_GPIO);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

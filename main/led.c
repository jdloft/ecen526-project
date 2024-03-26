#include "esp_log.h"
#include "led_strip.h"
#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

static const char *TAG = "led handler";
static led_strip_handle_t led_strip;
enum led_state current_led_state = LED_OFF;
uint8_t led_rgb_state[3] = {0, 0, 0};

void configure_led(void)
{
    ESP_LOGI(TAG, "LED configured");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_reset();
}

void led_reset()
{
    led_strip_clear(led_strip);
    led_strip_refresh(led_strip);
}

void set_state(uint32_t red, uint32_t green, uint32_t blue)
{
    led_strip_set_pixel(led_strip, 0, red, green, blue);
    led_rgb_state[0] = red;
    led_rgb_state[1] = green;
    led_rgb_state[2] = blue;
    led_strip_refresh(led_strip);
}

void led_set_ready(int is_receiver)
{
    ESP_LOGD(TAG, "LED ready state");
    current_led_state = LED_READY;
    if (is_receiver)
    {
        set_state(0, 16, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        set_state(0, 0, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        set_state(0, 16, 0);
    }
    else
    {
        set_state(0, 16, 0);
    }
}

void led_set_active()
{
    ESP_LOGD(TAG, "LED active state");
    current_led_state = LED_RECV;
    set_state(16, 14, 0);
}

void led_set_fault()
{
    ESP_LOGD(TAG, "LED fault state");
    current_led_state = LED_FAULT;
    set_state(16, 0, 0);
}

void blink_led()
{
    led_strip_set_pixel(led_strip, 0, 0, 0, 0);
    led_strip_refresh(led_strip);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    set_state(led_rgb_state[0], led_rgb_state[1], led_rgb_state[2]);
    led_strip_refresh(led_strip);
}

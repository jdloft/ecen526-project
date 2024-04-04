#include "esp_log.h"
#include "led_strip.h"
#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

static const char *TAG = "led handler";
static led_strip_handle_t led_strip;
static esp_timer_handle_t blink_timer;
static int blink_led_state = 0;

wirelesscomm_led_state_t current_led_state = LED_OFF;
uint8_t led_rgb_state[3] = {0, 0, 0};

static void set_state(uint32_t red, uint32_t green, uint32_t blue)
{
    led_strip_set_pixel(led_strip, 0, red, green, blue);
    led_rgb_state[0] = red;
    led_rgb_state[1] = green;
    led_rgb_state[2] = blue;
    led_strip_refresh(led_strip);
}

static void toggle_led()
{
    if (blink_led_state)
    {
        led_strip_set_pixel(led_strip, 0, 0, 0, 0);
        blink_led_state = 0;
    }
    else
    {
        led_strip_set_pixel(led_strip, 0, led_rgb_state[0], led_rgb_state[1], led_rgb_state[2]);
        blink_led_state = 1;
    }
    led_strip_refresh(led_strip);
}

void configure_led(void)
{
    ESP_LOGI(TAG, "Configuring LED");
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_reset();

    // blink timer
    const esp_timer_create_args_t timer_args = {
        .callback = toggle_led,
        .name = "led_blink_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &blink_timer));
}

const char *to_string_led_state(wirelesscomm_led_state_t state)
{
    switch (state)
    {
    case LED_OFF:
        return "LED_OFF";
    case LED_INIT:
        return "LED_INIT";
    case LED_READY:
        return "LED_READY";
    case LED_ACTIVE:
        return "LED_ACTIVE";
    case LED_FAULT:
        return "LED_FAULT";
    default:
        return "UNKNOWN";
    }
}

void led_reset()
{
    led_strip_clear(led_strip);
    led_strip_refresh(led_strip);
}

static void led_set_ready(int is_receiver)
{
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

void led_set(wirelesscomm_led_state_t state)
{
    esp_timer_stop(blink_timer);
    switch (state)
    {
    case LED_OFF:
        led_reset();
        break;
    case LED_INIT:
        set_state(16, 14, 0);
        blink_led_state = 1;
        ESP_ERROR_CHECK(esp_timer_start_periodic(blink_timer, 500000));
        break;
    case LED_READY:
        led_set_ready(0);
        break;
    case LED_ACTIVE:
        set_state(16, 14, 0);
        break;
    case LED_FAULT:
        set_state(16, 0, 0);
        break;
    default:
        ESP_LOGE(TAG, "Unknown LED state");
        return;
    }
    ESP_LOGD(TAG, "LED state set to %s", to_string_led_state(state));
    current_led_state = state;
}

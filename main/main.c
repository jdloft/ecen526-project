#include <stdio.h>
#include <sys/time.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "config.h"
#include "espnow.h"
#include "led.h"
#include "state.h"
#include "timesync.h"

static const char *TAG = "main";
static uint8_t is_receiver = 0;
volatile enum prog_state transition_state = STATE_INIT;


void setup_pins() {
    ESP_ERROR_CHECK(gpio_reset_pin(BUTTON_GPIO));
    ESP_ERROR_CHECK(gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_reset_pin(RECEIVER_GPIO));
    ESP_ERROR_CHECK(gpio_set_direction(RECEIVER_GPIO, GPIO_MODE_INPUT));
}

void init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void handle_usrbutton_isr(void* arg) {
    int gpio_num = (int)arg;
    int state = !gpio_get_level(gpio_num); // pull up: button connects to ground
    switch (transition_state) {
        case STATE_READY:
            if (state) {
                transition_state = STATE_SENDRECV;
            }
            break;
        default:
            break;
    }
}

const char *state_str(enum prog_state state) {
    switch (state) {
        case STATE_INIT:
            return "INIT";
        case STATE_READY:
            return "READY";
        case STATE_SENDRECV:
            return "SENDRECV";
        default:
            return "UNKNOWN";
    }
}

void app_main(void) {
    esp_err_t ret;
    uint8_t base_mac_addr[6] = {0};

    esp_log_level_set("led handler", ESP_LOG_DEBUG);
    configure_led();
    led_set(LED_INIT);
    setup_pins();
    init_nvs();

    is_receiver = !gpio_get_level(RECEIVER_GPIO);

    // ISRs
    ESP_LOGI(TAG, "ISR setup");
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_NEGEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_GPIO, handle_usrbutton_isr, (void *)BUTTON_GPIO));

    // time sync
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (tv.tv_sec < 60) {
        ESP_LOGI(TAG, "Time not set, syncing");
        if (wirelesscomm_time_init() != ESP_OK) {
            ESP_LOGE(TAG, "Error syncing time");
            led_set(LED_FAULT);
            return;
        }
    } else {
        ESP_LOGI(TAG, "Time already set: %lld.%06ld", tv.tv_sec, tv.tv_usec);
    }

    // wireless
    if (WL_PROTO == WL_PROTO_ESPNOW) {
        if (wirelesscomm_espnow_init() != 0) {
            ESP_LOGE(TAG, "Error initializing");
            led_set(LED_FAULT);
            return;
        }
    } else {
        ESP_LOGE(TAG, "Unsupported wireless protocol");
        return;
    }

    // output MAC addr
    ESP_ERROR_CHECK(esp_read_mac(base_mac_addr, ESP_MAC_WIFI_STA));
    ESP_LOGI(TAG, "Device is %s", is_receiver ? "receiver" : "sender");
    ESP_LOGI(TAG, "MAC: %x%x%x%x%x%x", base_mac_addr[0], base_mac_addr[1], base_mac_addr[2], base_mac_addr[3], base_mac_addr[4], base_mac_addr[5]);

    // initialization complete, do stuff
    ESP_LOGI(TAG, "Initialized");
    static enum prog_state current_state = STATE_INIT;
    transition_state = STATE_READY;

    while (1) {
        // handle state change
        enum prog_state cycle_state = transition_state; // we need to read once in case state changes during handle
        if (cycle_state != current_state) {
            ESP_LOGI(TAG, "Handling state transition %s -> %s", state_str(current_state), state_str(cycle_state));
            current_state = cycle_state;
            switch (cycle_state) {
                case STATE_READY:
                    led_set(LED_READY);
                    break;
                case STATE_SENDRECV:
                    led_set(LED_ACTIVE);
                    wirelesscomm_espnow_send();
                    ESP_LOGI(TAG, "done");
                    break;
                default:
                    break;
            }
        }
        transition_state = STATE_READY;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

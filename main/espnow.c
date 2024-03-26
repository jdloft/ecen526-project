#include "esp_log.h"
#include "espnow.h"
#include <unistd.h>
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_mac.h"

static const char *TAG = "espnow";

#define RECV_MAC = {}
#define CHANNEL 1

static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
// static uint8_t recv_mac[ESP_NOW_ETH_ALEN] = {};

int espnow_init()
{
    ESP_LOGI(TAG, "Initializing Wi-Fi");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE));
    // ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_AP, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR));

    ESP_LOGI(TAG, "Initializing ESP-NOW");
    ESP_ERROR_CHECK(esp_now_init());
    return 0;
}

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "Sent data to %x%x%x%x%x%x", MAC2STR(mac_addr));
    } else {
        ESP_LOGE(TAG, "Failed to send data to %x%x%x%x%x%x", MAC2STR(mac_addr));
    }
}

static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    ESP_LOGI(TAG, "Received data");

}

int espnow_send()
{
    ESP_LOGI(TAG, "Sending data via ESP-NOW");
    return 0;
}

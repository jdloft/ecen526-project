#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "espnow.h"
#include "led.h"

static const char *TAG = "espnow";

__unused static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t recv_mac[ESP_NOW_ETH_ALEN];

static void wirelesscomm_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
static void wirelesscomm_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len);

static void wifi_init() {
    ESP_LOGI(TAG, "Initializing Wi-Fi");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE));
    // long range?
    // ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_AP, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR));
}

static inline void mac_to_string(const uint8_t *mac, char *buf) {
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static inline void string_to_mac(const char *str, uint8_t *mac) {
    sscanf(str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

int wirelesscomm_espnow_init()
{
    wifi_init();
    ESP_LOGI(TAG, "Initializing ESP-NOW");
    ESP_ERROR_CHECK(esp_now_init());

    // set recv mac
    uint8_t peer1[ESP_NOW_ETH_ALEN];
    uint8_t peer2[ESP_NOW_ETH_ALEN];
    string_to_mac(CONFIG_PEER_MAC1, &peer1);
    string_to_mac(CONFIG_PEER_MAC2, &peer2);

    uint8_t my_mac[ESP_NOW_ETH_ALEN] = {0};
    ESP_ERROR_CHECK(esp_read_mac(my_mac, ESP_MAC_WIFI_STA));
    if (memcmp(my_mac, peer1, ESP_NOW_ETH_ALEN) == 0) {
        ESP_LOGI(TAG, "Setting peer to %s", CONFIG_PEER_MAC2);
        memcpy(recv_mac, peer2, ESP_NOW_ETH_ALEN);
    } else {
        ESP_LOGI(TAG, "Setting peer to %s", CONFIG_PEER_MAC1);
        memcpy(recv_mac, peer1, ESP_NOW_ETH_ALEN);
    }

    esp_now_peer_info_t *peer_info = malloc(sizeof(esp_now_peer_info_t));
    memset(peer_info, 0, sizeof(esp_now_peer_info_t));
    peer_info->channel = CONFIG_WIFI_CHANNEL;
    peer_info->ifidx = ESP_IF_WIFI_STA;
    peer_info->encrypt = false;
    memcpy(peer_info->peer_addr, recv_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer_info));

    ESP_ERROR_CHECK(esp_now_register_send_cb(wirelesscomm_espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(wirelesscomm_espnow_recv_cb));
    return 0;
}

static void wirelesscomm_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "Sent data to %x%x%x%x%x%x", MAC2STR(mac_addr));
    } else {
        ESP_LOGE(TAG, "Failed to send data to %x%x%x%x%x%x", MAC2STR(mac_addr));
    }
}

static void wirelesscomm_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    led_set(LED_ACTIVE);
    struct timeval recv_tv = *(struct timeval *)data;
    ESP_LOGI(TAG, "Received data. Time: %lld.%06ld", tv.tv_sec, tv.tv_usec);
    ESP_LOGI(TAG, "Elapsed time: %lld.%06ld", tv.tv_sec - recv_tv.tv_sec, tv.tv_usec - recv_tv.tv_usec);
    led_set(LED_READY);
}

int wirelesscomm_espnow_send()
{
    ESP_LOGI(TAG, "Sending data via ESP-NOW");
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ESP_ERROR_CHECK(esp_now_send(recv_mac, (uint8_t *)&tv, sizeof(tv)));
    return 0;
}

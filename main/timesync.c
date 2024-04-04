#include <sys/time.h>
#include <time.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_MAX_RETRIES   5

static const char *TAG = "time";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    static const char *TAG = "wifi event";
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi started");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi connected");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi disconnected");
        if (s_retry_num < WIFI_MAX_RETRIES) {
            esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying wifi connection");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void connect_wifi() {
    ESP_LOGI(TAG, "Connecting to Wi-Fi for time sync");
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_TIME_WIFI_SSID,
            .password = CONFIG_TIME_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    // wait for connection
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to Wi-Fi");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to Wi-Fi");
    }

    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
}

static void disconnect_wifi() {
    ESP_LOGI(TAG, "Disconnecting from Wi-Fi");
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
}

esp_err_t wirelesscomm_time_init() {
    connect_wifi();
    ESP_LOGI(TAG, "Initializing SNTP");
    struct timeval tv;

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_TIME_NTP_SERVER);
    ESP_ERROR_CHECK(esp_netif_sntp_init(&config));
    int retry = 0;
    esp_err_t ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000));
    while (ret != ESP_OK && retry++ < 5) {
        ESP_LOGI(TAG, "Failed to update system time");
        ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000));
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update system time");
        return ESP_FAIL;
    }

    gettimeofday(&tv, NULL);
    ESP_LOGI(TAG, "Time synchronized: %lld.%06ld", tv.tv_sec, tv.tv_usec);

    esp_netif_sntp_deinit();
    disconnect_wifi();

    return ESP_OK;
}

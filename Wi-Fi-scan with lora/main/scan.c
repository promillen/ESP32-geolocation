#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "iot_button.h"
#include "esp_mac.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "ttn.h"

#define DEFAULT_SCAN_LIST_SIZE CONFIG_EXAMPLE_SCAN_LIST_SIZE
#define NVS_NAMESPACE "wifi_scan"
#define DEEP_SLEEP_DURATION 30 // Deep sleep duration in seconds

#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32H2 || CONFIG_IDF_TARGET_ESP32C6
#define BOOT_BUTTON_NUM         9
#else
#define BOOT_BUTTON_NUM         0
#endif
#define BUTTON_ACTIVE_LEVEL     0

static const char *TAG_SCAN = "scan";
static const char *TAG_LORA = "LoRa";
static const char *TAG_STATUS = "\033[38;5;129mstatus\033[0m";
static const char *TAG_BUTTON = "\033[38;5;214mbutton\033[0m";

const char *button_event_table[] = {
    "BUTTON_PRESS_DOWN",
    "BUTTON_PRESS_UP",
    "BUTTON_PRESS_REPEAT",
    "BUTTON_PRESS_REPEAT_DONE",
    "BUTTON_SINGLE_CLICK",
    "BUTTON_DOUBLE_CLICK",
    "BUTTON_MULTIPLE_CLICK",
    "BUTTON_LONG_PRESS_START",
    "BUTTON_LONG_PRESS_HOLD",
    "BUTTON_LONG_PRESS_UP",
};

// Define a global variable to store the latest scan results
static wifi_ap_record_t latest_ap_info[DEFAULT_SCAN_LIST_SIZE];
static uint16_t latest_ap_count = 0;
static bool wifi_initialized = false;
bool TTN_is_joined = false;

static TimerHandle_t countdown_timer;
static TaskHandle_t countdown_task_handle = NULL;

// TTN variables
const char *appEui = "xxxxxxxxxxxxxxxx"; // AppEUI (sometimes called JoinEUI)
const char *devEui = "xxxxxxxxxxxxxxxx";
const char *appKey = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

#define TTN_SPI_HOST      SPI2_HOST
#define TTN_SPI_DMA_CHAN  SPI_DMA_DISABLED
#define TTN_PIN_SPI_SCLK  5
#define TTN_PIN_SPI_MOSI  27
#define TTN_PIN_SPI_MISO  19
#define TTN_PIN_NSS       18
#define TTN_PIN_RXTX      TTN_NOT_CONNECTED
#define TTN_PIN_RST       14
#define TTN_PIN_DIO0      26
#define TTN_PIN_DIO1      35

static uint8_t msgData[] = "Hello, world";

// Function prototypes
static void wifi_scan(void);
static void print_latest_scan(void);
static void save_scan_results_to_nvs(void);
static void load_scan_results_from_nvs(void);
static void button_event_cb(void *arg, void *data);
static void countdown_timer_callback(TimerHandle_t xTimer);
static void countdown_task(void *pvParameters);
static void send_wifi_scan(void);
static void erase_nvs(void);

static void button_event_cb(void *arg, void *data)
{
    button_event_t event = (button_event_t)data;
    ESP_LOGI(TAG_BUTTON, "Button event %s", button_event_table[event]);

    if (event == BUTTON_SINGLE_CLICK) {
        wifi_scan(); // Example action for single click
    } else if (event == BUTTON_DOUBLE_CLICK) {
        print_latest_scan(); // Example action for double click
    } else if (event == BUTTON_LONG_PRESS_START) {
        ESP_LOGI(TAG_BUTTON, "Erasing NVS...");
        erase_nvs(); // Call the function to erase NVS
    }

    // Reset the countdown timer on button press
    if (countdown_timer) {
        xTimerReset(countdown_timer, 0);
    }

    // Reset the countdown task
    if (countdown_task_handle) {
        vTaskDelete(countdown_task_handle);
        countdown_task_handle = NULL;
    }

    xTaskCreate(countdown_task, "CountdownTask", 2048, NULL, 5, &countdown_task_handle);
}

void button_init(uint32_t button_num)
{
    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = button_num,
            .active_level = BUTTON_ACTIVE_LEVEL
        },
    };
    button_handle_t btn = iot_button_create(&btn_cfg);
    assert(btn);
    esp_err_t err = iot_button_register_cb(btn, BUTTON_SINGLE_CLICK, button_event_cb, (void *)BUTTON_SINGLE_CLICK);
    ESP_ERROR_CHECK(err);
    err = iot_button_register_cb(btn, BUTTON_DOUBLE_CLICK, button_event_cb, (void *)BUTTON_DOUBLE_CLICK);
    ESP_ERROR_CHECK(err);
    err = iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, button_event_cb, (void *)BUTTON_LONG_PRESS_START);
    ESP_ERROR_CHECK(err);
}

/* Initialize Wi-Fi as sta and set scan method */
static void wifi_scan(void)
{
    if (!wifi_initialized) {
        // Initialize NVS, event loop, and Wi-Fi only once
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
        assert(sta_netif);

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        wifi_initialized = true;
    }

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    memset(latest_ap_info, 0, sizeof(latest_ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_scan_start(NULL, true);
    ESP_LOGI(TAG_SCAN, "Max AP number ap_info can hold = %u", number);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&latest_ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, latest_ap_info));
    ESP_LOGI(TAG_SCAN, "Total APs scanned = %u, actual AP number ap_info holds = %u", latest_ap_count, number);

    save_scan_results_to_nvs();
}

static void save_scan_results_to_nvs(void)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_SCAN, "Failed to open NVS handle");
        return;
    }

    // Store the number of APs
    err = nvs_set_u16(my_handle, "ap_count", latest_ap_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_SCAN, "Failed to save AP count");
        nvs_close(my_handle);
        return;
    }

    // Store the AP records
    err = nvs_set_blob(my_handle, "ap_records", latest_ap_info, sizeof(latest_ap_info));
    if (err != ESP_OK) {
        ESP_LOGE(TAG_SCAN, "Failed to save AP records");
    }

    nvs_close(my_handle);
}

static void load_scan_results_from_nvs(void)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_SCAN, "Failed to open NVS handle");
        return;
    }

    // Load the number of APs
    size_t required_size = sizeof(latest_ap_count);
    err = nvs_get_u16(my_handle, "ap_count", &latest_ap_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_SCAN, "Failed to load AP count");
        nvs_close(my_handle);
        return;
    }

    // Load the AP records
    required_size = sizeof(latest_ap_info);
    err = nvs_get_blob(my_handle, "ap_records", latest_ap_info, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_SCAN, "Failed to load AP records");
    }

    nvs_close(my_handle);
}

static void print_latest_scan(void)
{
    if (latest_ap_count == 0) {
        ESP_LOGI(TAG_SCAN, "No APs found in the latest scan.");
        return;
    }

    ESP_LOGI(TAG_SCAN, "Latest Scan Results:");
    for (int i = 0; i < latest_ap_count; i++) {
        ESP_LOGI(TAG_SCAN, "SSID \t\t%s", latest_ap_info[i].ssid);
        ESP_LOGI(TAG_SCAN, "RSSI \t\t%d", latest_ap_info[i].rssi);
        ESP_LOGI(TAG_SCAN, "MAC \t\t" MACSTR, MAC2STR(latest_ap_info[i].bssid));
    }
}

static void countdown_task(void *pvParameters)
{
    for (int i = 20; i > 0; i--) {
        ESP_LOGI(TAG_STATUS, "%d", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for 1 second
    }

    // Start deep sleep after countdown
    countdown_timer_callback(NULL);
    vTaskDelete(NULL); // End task when done
}

static void countdown_timer_callback(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG_STATUS, "Entering deep sleep now");

    // Configure deep sleep with timer and GPIO wake-up sources
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION * 1000000); // Duration in microseconds
    esp_sleep_enable_ext0_wakeup(BOOT_BUTTON_NUM, BUTTON_ACTIVE_LEVEL); // GPIO wake-up
    esp_deep_sleep_start();
}

void sendMessages(void* pvParameter)
{
    // Send the message
    ESP_LOGI(TAG_LORA, "Sending message...");
    ttn_response_code_t res = ttn_transmit_message(msgData, sizeof(msgData) - 1, 1, false);
    if(res == TTN_SUCCESSFUL_TRANSMISSION) {
        ESP_LOGI(TAG_LORA, "Message sent.\n");
    } else {
        ESP_LOGI(TAG_LORA, "Transmission failed.\n");
    }
    vTaskDelete(NULL);
}

void messageReceived(const uint8_t* message, size_t length, ttn_port_t port)
{
    printf("Message of %d bytes received on port %d:", length, port);
    for (int i = 0; i < length; i++)
        printf(" %02x", message[i]);
    printf("\n");
}

static void erase_nvs(void)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        err = nvs_erase_all(my_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG_STATUS, "NVS erased successfully");
        } else {
            ESP_LOGE(TAG_STATUS, "Failed to erase NVS: %s", esp_err_to_name(err));
        }
        nvs_close(my_handle);
    } else {
        ESP_LOGE(TAG_STATUS, "Failed to open NVS handle for erasing");
    }
}

static void send_wifi_scan(void)
{
    // Send the new message
    ESP_LOGI(TAG_LORA, "Sending message with new Wi-Fi scan...");
    ttn_response_code_t res = ttn_transmit_message(msgData, sizeof(msgData) - 1, 1, false);
    if (res == TTN_SUCCESSFUL_TRANSMISSION) {
        ESP_LOGI(TAG_LORA, "Message sent.");
        // Erase the saved scan results from NVS if the message was sent successfully
        nvs_handle_t my_handle;
        esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
        if (err == ESP_OK) {
            nvs_erase_all(my_handle);
            nvs_close(my_handle);
        } else {
            ESP_LOGE(TAG_SCAN, "Failed to open NVS handle for erasing");
        }
    } else {
        ESP_LOGI(TAG_LORA, "Transmission failed.");
        // Save the new scan results in NVS if the message failed
        save_scan_results_to_nvs();
    }
}

void app_main(void)
{
    esp_err_t ret;

    // Initialize the GPIO ISR handler service
    ret = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    ESP_ERROR_CHECK(ret);

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize SPI bus
    spi_bus_config_t spi_bus_config = {
        .miso_io_num = TTN_PIN_SPI_MISO,
        .mosi_io_num = TTN_PIN_SPI_MOSI,
        .sclk_io_num = TTN_PIN_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    }; 
    ret = spi_bus_initialize(TTN_SPI_HOST, &spi_bus_config, TTN_SPI_DMA_CHAN);
    ESP_ERROR_CHECK(ret);

    // Initialize TTN
    ttn_init();

    // Configure the SX127x pins
    ttn_configure_pins(TTN_SPI_HOST, TTN_PIN_NSS, TTN_PIN_RXTX, TTN_PIN_RST, TTN_PIN_DIO0, TTN_PIN_DIO1);

    // The below line can be commented after the first run as the data is saved in NVS
    ttn_provision(devEui, appEui, appKey);

    // Register callback for received messages
    ttn_on_message(messageReceived);

    // Log wake-up message
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG_STATUS, "Woke up from deep sleep due to timer");
    } else if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        ESP_LOGI(TAG_STATUS, "Woke up from deep sleep due to button press");
    }

    // Initialize button handling
    button_init(BOOT_BUTTON_NUM);

    // Check NVS for previous scan results
    load_scan_results_from_nvs();

    if (latest_ap_count > 0) {
        // NVS has scan results
        ESP_LOGI(TAG_STATUS, "NVS contains saved scan results");
        ESP_LOGI(TAG_STATUS, "Sending join request...");
        if (ttn_join()) {
            ESP_LOGI(TAG_STATUS, "Network joined successfully");
            TTN_is_joined = true;

            // Create a task to send saved messages
            xTaskCreate(sendMessages, "SendMessagesTask", 2048, NULL, 5, NULL);
        } else {
            ESP_LOGE(TAG_STATUS, "Failed to join network");
            TTN_is_joined = false;
        }
    } else {
        // Perform new scan
        wifi_scan();

        ESP_LOGI(TAG_STATUS, "Sending join request...");
        if (ttn_join()) {
            ESP_LOGI(TAG_STATUS, "Network joined successfully");
            TTN_is_joined = true;

            // Send the new scan result
            send_wifi_scan();
        } else {
            ESP_LOGE(TAG_STATUS, "Failed to join network");
            TTN_is_joined = false;
        }
    }

    // Initialize countdown timer
    countdown_timer = xTimerCreate("CountdownTimer", pdMS_TO_TICKS(DEEP_SLEEP_DURATION * 1000), pdFALSE, 0, countdown_timer_callback);
    if (countdown_timer == NULL) {
        ESP_LOGE(TAG_STATUS, "Failed to create countdown timer");
        return;
    }

    // Start countdown task
    xTaskCreate(countdown_task, "CountdownTask", 2048, NULL, 5, &countdown_task_handle);
}

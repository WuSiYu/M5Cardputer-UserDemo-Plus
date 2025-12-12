/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "hal.h"
#include "hal_config.h"
#include <apps/utils/audio/audio.h>
#include <mooncake_log.h>
#include <M5Unified.hpp>
#include <esp_mac.h>
#include <memory>

static std::unique_ptr<Hal> _hal_instance;
static const std::string _tag = "HAL";

#define CPU_USAGE_MONITORING_ENABLED 1

#ifdef CPU_USAGE_MONITORING_ENABLED
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
// adapted from https://github.com/espressif/esp-idf/blob/master/examples/system/freertos/real_time_stats/main/real_time_stats_example_main.c
#define ARRAY_SIZE_OFFSET   5   //Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE
/**
 * @brief   Function to print the CPU usage of tasks over a given duration.
 *
 * This function will measure and print the CPU usage of tasks over a specified
 * number of ticks (i.e. real time stats). This is implemented by simply calling
 * uxTaskGetSystemState() twice separated by a delay, then calculating the
 * differences of task run times before and after the delay.
 *
 * @note    If any tasks are added or removed during the delay, the stats of
 *          those tasks will not be printed.
 * @note    This function should be called from a high priority task to minimize
 *          inaccuracies with delays.
 * @note    When running in dual core mode, each core will correspond to 50% of
 *          the run time.
 *
 * @param   xTicksToWait    Period of stats measurement
 *
 * @return
 *  - ESP_OK                Success
 *  - ESP_ERR_NO_MEM        Insufficient memory to allocated internal arrays
 *  - ESP_ERR_INVALID_SIZE  Insufficient array size for uxTaskGetSystemState. Trying increasing ARRAY_SIZE_OFFSET
 *  - ESP_ERR_INVALID_STATE Delay duration too short
 */
static esp_err_t print_real_time_stats(TickType_t xTicksToWait)
{
    TaskStatus_t *start_array = NULL, *end_array = NULL;
    UBaseType_t start_array_size, end_array_size;
    configRUN_TIME_COUNTER_TYPE start_run_time, end_run_time;
    esp_err_t ret;
    uint32_t total_elapsed_time;

    //Allocate array to store current task states
    start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    start_array = (TaskStatus_t *)malloc(sizeof(TaskStatus_t) * start_array_size);
    if (start_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    //Get current task states
    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
    if (start_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    vTaskDelay(xTicksToWait);

    //Allocate array to store tasks states post delay
    end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    end_array = (TaskStatus_t *)malloc(sizeof(TaskStatus_t) * end_array_size);
    if (end_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    //Get post delay task states
    end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
    if (end_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    //Calculate total_elapsed_time in units of run time stats clock period.
    total_elapsed_time = (end_run_time - start_run_time);
    if (total_elapsed_time == 0) {
        ret = ESP_ERR_INVALID_STATE;
        goto exit;
    }

    printf("\n|                 Task | Run Time | Percentage\n");
    //Match each task in start_array to those in the end_array
    for (int i = 0; i < start_array_size; i++) {
        int k = -1;
        for (int j = 0; j < end_array_size; j++) {
            if (start_array[i].xHandle == end_array[j].xHandle) {
                k = j;
                //Mark that task have been matched by overwriting their handles
                start_array[i].xHandle = NULL;
                end_array[j].xHandle = NULL;
                break;
            }
        }
        //Check if matching task found
        if (k >= 0) {
            uint32_t task_elapsed_time = end_array[k].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
            uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * CONFIG_FREERTOS_NUMBER_OF_CORES);
            printf("| %20s | %8"PRIu32" | %"PRIu32"%%\n", start_array[i].pcTaskName, task_elapsed_time, percentage_time);
        }
    }

    //Print unmatched tasks
    for (int i = 0; i < start_array_size; i++) {
        if (start_array[i].xHandle != NULL) {
            printf("| %20s | Deleted\n", start_array[i].pcTaskName);
        }
    }
    for (int i = 0; i < end_array_size; i++) {
        if (end_array[i].xHandle != NULL) {
            printf("| %20s | Created\n", end_array[i].pcTaskName);
        }
    }
    ret = ESP_OK;

exit:    //Common return path
    free(start_array);
    free(end_array);
    return ret;
}

static void system_monitor_task(void *pvParameters)
{
    while (1) {
        if (auto ret = print_real_time_stats(pdMS_TO_TICKS(1000)); ret != ESP_OK) {
            mclog::tagError("system_monitor", "print_real_time_stats failed: {}", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void Hal::startSystemMonitorTask()
{
    xTaskCreatePinnedToCore(system_monitor_task, "system_monitor", 4096, NULL, 1, NULL, APP_CPU_NUM);
}
#endif // CPU_USAGE_MONITORING_ENABLED

Hal& GetHAL()
{
    if (!_hal_instance) {
        mclog::tagInfo(_tag, "creating hal instance");
        _hal_instance = std::make_unique<Hal>();
    }
    return *_hal_instance.get();
}

void Hal::init()
{
    mclog::tagInfo(_tag, "init");

    M5.begin();
    M5.Display.setBrightness(0);
    M5.Speaker.begin();  // Codec takes some time to initialize

    display_init();
    i2c_scan();
    keyboard_init();
    setting_init();
    spi_init();

#if CPU_USAGE_MONITORING_ENABLED
    startSystemMonitorTask();
#endif
}

void Hal::update()
{
    M5.update();
    keyboard.update();
    capLora868.update();
}

void Hal::feedTheDog()
{
    vTaskDelay(1);
}

std::vector<uint8_t> Hal::getDeviceMac()
{
    std::vector<uint8_t> mac(6);
    esp_read_mac(mac.data(), ESP_MAC_EFUSE_FACTORY);
    return mac;
}

std::string Hal::getDeviceMacString()
{
    auto mac = getDeviceMac();
    return fmt::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/* -------------------------------------------------------------------------- */
/*                                  Dispplay                                  */
/* -------------------------------------------------------------------------- */
void Hal::display_init()
{
    mclog::tagInfo(_tag, "display init");

    canvas.createSprite(204, 109);
    canvasKeyboardBar.createSprite(display.width() - canvas.width(), display.height());
    canvasSystemBar.createSprite(canvas.width(), display.height() - canvas.height());
}

/* -------------------------------------------------------------------------- */
/*                                     I2C                                    */
/* -------------------------------------------------------------------------- */
void Hal::i2c_scan()
{
    mclog::tagInfo(_tag, "i2c scan");

    bool ret[128] = {false};
    M5.In_I2C.scanID(ret);

    uint8_t address;
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
    for (int i = 0; i < 128; i += 16) {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++) {
            fflush(stdout);
            address = i + j;
            if (ret[address]) {
                printf("%02x ", address);
            } else {
                printf("-- ");
            }
        }
        printf("\r\n");
    }
}

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
// https://github.com/78/xiaozhi-esp32/blob/main/main/main.cc

void Hal::setting_init()
{
    mclog::tagInfo(_tag, "setting init");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        mclog::tagWarn(_tag, "erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    _settings = new Settings("cardputer", true);
}

/* -------------------------------------------------------------------------- */
/*                                  Keyboard                                  */
/* -------------------------------------------------------------------------- */
void Hal::keyboard_init()
{
    mclog::tagInfo(_tag, "keyboard init");

    if (!keyboard.init()) {
        mclog::tagError(_tag, "keyboard init failed");
        return;
    }
}

/* -------------------------------------------------------------------------- */
/*                                    WiFI                                    */
/* -------------------------------------------------------------------------- */
// https://github.com/espressif/esp-idf/blob/v5.3.3/examples/wifi/scan/main/scan.c
// https://github.com/espressif/esp-idf/blob/v5.4.2/examples/wifi/getting_started/station/main/station_example_main.c
// http://github.com/espressif/esp-idf/blob/v5.4.2/examples/protocols/sntp/main/sntp_example_main.c
#include <esp_wifi.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_event.h>
#include <lwip/err.h>
#include <lwip/sys.h>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include <esp_sntp.h>

void Hal::wifiScan(std::vector<ScanResult_t>& scanResult)
{
    // Keep blocking API for compatibility, but use the async path internally
    mclog::tagInfo(_tag, "wifi scan (compat, blocking)");
    wifiScanStartAsync();
    while (!_is_wifi_scan_done) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    wifiScanCollect(scanResult);
}

bool Hal::wifiScanStartAsync()
{
    mclog::tagInfo(_tag, "wifi scan start async");

    if (!_is_wifi_inited) {
        mclog::tagError(_tag, "wifi not inited");
        return false;
    }

    if (_is_wifi_scan_in_progress) {
        mclog::tagWarn(_tag, "wifi scan already in progress");
        return false;
    }

    _is_wifi_scan_in_progress = true;
    _is_wifi_scan_done        = false;

    wifi_scan_config_t config = {};
    config.ssid               = nullptr;
    config.bssid              = nullptr;
    config.channel            = 0;
    config.show_hidden        = false;
    config.scan_type          = WIFI_SCAN_TYPE_ACTIVE;

    esp_err_t ret = esp_wifi_scan_start(&config, false);  // non-blocking
    if (ret != ESP_OK) {
        mclog::tagError(_tag, "failed to start wifi scan async: {}", esp_err_to_name(ret));
        _is_wifi_scan_in_progress = false;
        return false;
    }
    return true;
}

bool Hal::wifiScanCollect(std::vector<ScanResult_t>& scanResult)
{
    if (!_is_wifi_scan_done) {
        return false;
    }

    mclog::tagInfo(_tag, "wifi scan collect");

    scanResult.clear();

    uint16_t ap_count = 0;
    esp_err_t ret = esp_wifi_scan_get_ap_num(&ap_count);
    if (ret != ESP_OK) {
        mclog::tagError(_tag, "failed to get AP number: {}", esp_err_to_name(ret));
        _is_wifi_scan_in_progress = false;
        _is_wifi_scan_done        = false;
        return false;
    }

    if (ap_count == 0) {
        mclog::tagInfo(_tag, "no AP found");
        _is_wifi_scan_in_progress = false;
        _is_wifi_scan_done        = false;
        return true;
    }

    // 动态分配内存
    wifi_ap_record_t* ap_info = new (std::nothrow) wifi_ap_record_t[ap_count];
    if (!ap_info) {
        mclog::tagError(_tag, "failed to allocate memory for AP records");
        _is_wifi_scan_in_progress = false;
        _is_wifi_scan_done        = false;
        return false;
    }

    uint16_t number = ap_count;
    ret = esp_wifi_scan_get_ap_records(&number, ap_info);
    if (ret != ESP_OK) {
        mclog::tagError(_tag, "failed to get AP records: {}", esp_err_to_name(ret));
        delete[] ap_info;
        _is_wifi_scan_in_progress = false;
        _is_wifi_scan_done        = false;
        return false;
    }

    for (int i = 0; i < number; i++) {
        std::string ssid = (char*)ap_info[i].ssid;
        int rssi         = ap_info[i].rssi;
        uint8_t channel  = ap_info[i].primary;

        if (ssid.empty()) {
            continue;
        }

        scanResult.push_back({rssi, channel, ssid});
    }

    // 释放内存
    delete[] ap_info;

    std::sort(scanResult.begin(), scanResult.end(),
              [](const ScanResult_t& a, const ScanResult_t& b) {
                  return a.rssi > b.rssi;
              });

    _is_wifi_scan_in_progress = false;
    _is_wifi_scan_done        = false;
    return true;
}

static EventGroupHandle_t s_wifi_event_group = NULL;
static const int WIFI_CONNECTED_BIT          = BIT0;
static const int WIFI_DISCONNECTED_BIT       = BIT1;
static const int WIFI_FAIL_BIT               = BIT2;
static const int WIFI_STARTED_BIT            = BIT3;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    const char* TAG = "wifi";

    // Wifi started
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_STARTED_BIT);
    }

    // Disconnected
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }

    // Connected
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        // Set connected flag and start SNTP
        if (_hal_instance) {
            if (_hal_instance->_is_wifi_background_pending) {
                _hal_instance->_is_wifi_background_connected = true;
            }
        }
    }

    // Scan done
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        if (_hal_instance) {
            _hal_instance->on_wifi_scan_done();
        }
    }
}

void Hal::wifiInit()
{
    mclog::tagInfo(_tag, "wifi init");

    if (_is_wifi_inited) {
        return;
    }

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    if (!s_wifi_event_group) {
        s_wifi_event_group = xEventGroupCreate();
    }

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, nullptr));
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr, nullptr));

    ESP_ERROR_CHECK(esp_wifi_start());
    _is_wifi_inited = true;
}

void Hal::wifiDeinit()
{
    mclog::tagInfo(_tag, "wifi deinit");

    if (!_is_wifi_inited) {
        return;
    }

    esp_wifi_stop();
    esp_wifi_deinit();
    _is_wifi_inited = false;
}

void Hal::wifiConnectBackground(const std::string& ssid, const std::string& password)
{
    mclog::tagInfo(_tag, "wifi connect (background) to ssid: {} password: {}", ssid, password);

    if (!_is_wifi_inited) {
        wifiInit();
    }

    wifiDisconnect();

    // Hold until wifi started
    xEventGroupWaitBits(s_wifi_event_group, WIFI_STARTED_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(3000));

    // Reset event status
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | WIFI_DISCONNECTED_BIT);

    // Set Wi-Fi config
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    _is_wifi_background_pending = true;
    _is_wifi_background_connected  = false;
}

bool Hal::wifiConnectBackgroundCheck() {
    if (_is_wifi_background_pending && _is_wifi_background_connected) {
        _is_wifi_background_pending = false;
        mclog::tagInfo(_tag, "wifi background connect succeeded");
        _is_wifi_connected = true;
        start_sntp();
        return true;
    }
    return false;
}

bool Hal::wifiConnect(const std::string& ssid, const std::string& password)
{
    // reset background connect flags
    _is_wifi_background_pending = false;
    _is_wifi_background_connected  = false;

    mclog::tagInfo(_tag, "wifi connect to ssid: {} password: {}", ssid, password);

    if (!_is_wifi_inited) {
        wifiInit();
    }

    wifiDisconnect();

    // Hold until wifi started
    xEventGroupWaitBits(s_wifi_event_group, WIFI_STARTED_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(3000));

    // Reset event status
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | WIFI_DISCONNECTED_BIT);

    // Set Wi-Fi config
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    // Wait for connection result
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE,
                                           pdMS_TO_TICKS(10000));

    if (bits & WIFI_CONNECTED_BIT) {
        mclog::tagInfo(_tag, "connected to SSID: {}", ssid);
        _is_wifi_connected = true;
        start_sntp();
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        mclog::tagError(_tag, "failed to connect to SSID: {}", ssid);
        return false;
    } else {
        mclog::tagError(_tag, "wifi connect timeout");
        return false;
    }
}

void Hal::wifiDisconnect()
{
    mclog::tagInfo(_tag, "wifi disconnect");

    if (!_is_wifi_inited) {
        return;
    }

    if (!_is_wifi_connected) {
        return;
    }

    // Hold until wifi started
    xEventGroupWaitBits(s_wifi_event_group, WIFI_STARTED_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(3000));

    // Disconnect old connection
    ESP_ERROR_CHECK(esp_wifi_disconnect());

    // Wait for disconnect result
    xEventGroupWaitBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(5000));

    // Reset event status
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | WIFI_DISCONNECTED_BIT);

    stop_sntp();

    _is_wifi_connected = false;
}

void Hal::start_sntp()
{
    mclog::tagInfo(_tag, "start sntp");

    if (!_is_wifi_connected) {
        mclog::tagError(_tag, "wifi not connected");
        return;
    }

    // Set timezone to UTC+8
    setenv("TZ", "CST-8", 1);
    tzset();

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}

void Hal::stop_sntp()
{
    mclog::tagInfo(_tag, "stop sntp");

    if (!_is_wifi_connected) {
        mclog::tagError(_tag, "wifi not connected");
        return;
    }

    esp_sntp_stop();
}

void Hal::on_wifi_scan_done()
{
    // mclog::tagInfo(_tag, "wifi scan done event");
    _is_wifi_scan_in_progress = false;
    _is_wifi_scan_done = true;
}

/* -------------------------------------------------------------------------- */
/*                                   EspNow                                   */
/* -------------------------------------------------------------------------- */
// https://github.com/espressif/esp-now/blob/master/examples/get-started/main/app_main.c
#include <esp_mac.h>
#include <espnow.h>
#include <espnow_storage.h>
#include <espnow_utils.h>
#include <esp_check.h>

static std::string _espnow_received_data;
static esp_err_t _handle_espnow_received(uint8_t* src_addr, void* data, size_t size, wifi_pkt_rx_ctrl_t* rx_ctrl)
{
    const char* TAG = "espnow";

    ESP_PARAM_CHECK(src_addr);
    ESP_PARAM_CHECK(data);
    ESP_PARAM_CHECK(size);
    ESP_PARAM_CHECK(rx_ctrl);

    static uint32_t count = 0;

    ESP_LOGI(TAG, "espnow_recv, <%" PRIu32 "> [" MACSTR "][%d][%d][%u]: %.*s", count++, MAC2STR(src_addr),
             rx_ctrl->channel, rx_ctrl->rssi, size, size, (char*)data);

    _espnow_received_data = std::string((char*)data, size);

    return ESP_OK;
}

void Hal::espNowInit()
{
    mclog::tagInfo(_tag, "esp now init");

    if (!_is_wifi_inited) {
        wifiInit();
    }

    if (_is_wifi_connected) {
        wifiDisconnect();
        espNowDeinit();
    }

    if (_is_esp_now_inited) {
        mclog::tagInfo(_tag, "esp now already inited");
        return;
    }

    // espnow_storage_init();

    espnow_config_t espnow_config = ESPNOW_INIT_CONFIG_DEFAULT();
    espnow_init(&espnow_config);
    espnow_set_config_for_data_type(ESPNOW_DATA_TYPE_DATA, true, _handle_espnow_received);

    _is_esp_now_inited = true;
}

void Hal::espNowDeinit()
{
    mclog::tagInfo(_tag, "esp now deinit");

    if (!_is_esp_now_inited) {
        mclog::tagInfo(_tag, "esp now not inited");
        return;
    }

    espnow_deinit();

    _is_esp_now_inited = false;
}

void Hal::espNowSend(const std::string& data)
{
    mclog::tagInfo(_tag, "esp now send: {}", data);

    if (!_is_esp_now_inited) {
        mclog::tagError(_tag, "esp now not inited");
        return;
    }

    espnow_frame_head_t frame_head = ESPNOW_FRAME_CONFIG_DEFAULT();
    auto ret = espnow_send(ESPNOW_DATA_TYPE_DATA, ESPNOW_ADDR_BROADCAST, data.c_str(), data.size(), &frame_head,
                           portMAX_DELAY);

    if (ret != ESP_OK) {
        mclog::tagError(_tag, "failed to send esp now: {}", esp_err_to_name(ret));
    }
}

bool Hal::espNowAvailable()
{
    return _espnow_received_data.size() > 0;
}

const std::string& Hal::espNowGetReceivedData()
{
    return _espnow_received_data;
}

void Hal::espNowClearReceivedData()
{
    _espnow_received_data.clear();
}

/* -------------------------------------------------------------------------- */
/*                                     IR                                     */
/* -------------------------------------------------------------------------- */
#include "utils/ir_nec/ir_helper.h"

void Hal::irInit()
{
    mclog::tagInfo(_tag, "ir init");

    if (_is_ir_inited) {
        mclog::tagInfo(_tag, "ir already inited");
        return;
    }

    ir_helper_init((gpio_num_t)HAL_PIN_IR_TX);

    _is_ir_inited = true;
}

void Hal::irSend(uint8_t addr, uint8_t cmd)
{
    mclog::tagInfo(_tag, "ir send: addr: {:02X}, cmd: {:02X}", addr, cmd);

    if (!_is_ir_inited) {
        mclog::tagError(_tag, "ir not inited");
        return;
    }

    ir_helper_send(addr, cmd);
}

/* -------------------------------------------------------------------------- */
/*                                     BLE                                    */
/* -------------------------------------------------------------------------- */
#include "utils/ble_hid_device/ble_hid_device_helper.h"

void Hal::bleKeyboardInit()
{
    if (_is_ble_keyboard_inited) {
        mclog::tagWarn(_tag, "ble keyboard already initialized");
        return;
    }

    mclog::tagInfo(_tag, "ble keyboard init");

    // Initialize BLE HID device
    ble_hid_device_helper_init();

    // Register keyboard event callback to automatically forward keys
    _ble_keyboard_event_slot_id = keyboard.onKeyEvent.connect(
        [this](const Keyboard::KeyEvent_t& keyEvent) { handle_ble_keyboard_event(keyEvent); });

    _is_ble_keyboard_inited = true;
    mclog::tagInfo(_tag, "ble keyboard init done, auto-forwarding enabled");
}

bool Hal::bleKeyboardIsConnected() const
{
    if (!_is_ble_keyboard_inited) {
        return false;
    }

    auto state = ble_hid_device_helper_get_state();
    return (state == BLE_HID_DEVICE_STATE_CONNECTED);
}

void Hal::handle_ble_keyboard_event(const Keyboard::KeyEvent_t& keyEvent)
{
    // Only forward if BLE keyboard is connected
    if (!bleKeyboardIsConnected()) {
        return;
    }

    // Create HID buffer (8 bytes: modifier, reserved, keycode1-6)
    uint8_t buffer[8] = {0};

    // Handle key press/release
    if (keyEvent.state) {
        // Get current modifier state from keyboard
        uint8_t modifierMask = keyboard.getModifierMask();

        // Set modifier byte
        buffer[0] = modifierMask;

        // For modifier keys themselves, don't set keycode
        if (keyEvent.isModifier) {
            buffer[2] = 0;  // No keycode for pure modifier keys
        } else {
            buffer[2] = keyEvent.keyCode;
        }

        // Send key press
        ble_hid_device_helper_send(buffer);
        mclog::tagDebug(_tag, "ble keyboard sent key: {} (code: {}, modifier: 0x{:02x})",
                        keyEvent.keyName ? keyEvent.keyName : "special", (int)keyEvent.keyCode, modifierMask);
    } else {
        // Key released - for modifier keys, update modifier state and send
        if (keyEvent.isModifier) {
            uint8_t modifierMask = keyboard.getModifierMask();
            buffer[0]            = modifierMask;
            buffer[2]            = 0;
        } else {
            // For non-modifier keys, send empty buffer (all keys up)
            memset(buffer, 0, sizeof(buffer));
        }
        ble_hid_device_helper_send(buffer);
        mclog::tagDebug(_tag, "ble keyboard key released");
    }
}

/* -------------------------------------------------------------------------- */
/*                                     USB                                    */
/* -------------------------------------------------------------------------- */
// https://github.com/espressif/esp-idf/blob/v5.4.2/examples/peripherals/usb/device/tusb_hid
#include "utils/tusb_hid_device/tusb_hid_device_helper.h"

void Hal::usbKeyboardInit()
{
    if (_is_usb_keyboard_inited) {
        mclog::tagWarn(_tag, "usb keyboard already initialized");
        return;
    }

    mclog::tagInfo(_tag, "usb keyboard init");

    delay(200);

    tusb_hid_device_helper_init();

    _usb_keyboard_event_slot_id = keyboard.onKeyEvent.connect(
        [this](const Keyboard::KeyEvent_t& keyEvent) { handle_usb_keyboard_event(keyEvent); });

    _is_usb_keyboard_inited = true;
}

bool Hal::usbKeyboardIsConnected() const
{
    if (!_is_usb_keyboard_inited) {
        return false;
    }

    return tusb_hid_device_helper_is_mounted();
}

void Hal::handle_usb_keyboard_event(const Keyboard::KeyEvent_t& keyEvent)
{
    // Only forward if USB keyboard is connected
    if (!usbKeyboardIsConnected()) {
        return;
    }

    // Handle key press/release
    if (keyEvent.state) {
        uint8_t keycode[6] = {keyEvent.keyCode};
        tusb_hid_device_helper_report(GetHAL().keyboard.getModifierMask(), keycode);
        mclog::tagDebug(_tag, "usb keyboard sent key: {} (code: {})", keyEvent.keyName ? keyEvent.keyName : "special",
                        (int)keyEvent.keyCode);
    } else {
        tusb_hid_device_helper_report(0, NULL);
        mclog::tagDebug(_tag, "usb keyboard key released");
    }
}

/* -------------------------------------------------------------------------- */
/*                                     SPI                                    */
/* -------------------------------------------------------------------------- */
#include <driver/spi_master.h>
#include <driver/sdspi_host.h>
#include <driver/sdmmc_host.h>

static bool _spi_bus_initialized = false;

void Hal::spi_init()
{
    mclog::tagInfo(_tag, "spi init");

    esp_err_t ret;

    // spi_host_device_t host_id = SPI2_HOST;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = HAL_PIN_SPI_MOSI,
        .miso_io_num     = HAL_PIN_SPI_MISO,
        .sclk_io_num     = HAL_PIN_SPI_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4000,
    };

    // Initialize SPI bus only if not already initialized
    if (!_spi_bus_initialized) {
        ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
        if (ret != ESP_OK) {
            mclog::tagError(_tag, "failed to initialize SPI bus");
            return;
        }
        _spi_bus_initialized = true;
        mclog::tagInfo(_tag, "spi bus initialized");
    } else {
        mclog::tagWarn(_tag, "spi bus already initialized, reusing");
    }
}

/* -------------------------------------------------------------------------- */
/*                                   SD Card                                  */
/* -------------------------------------------------------------------------- */
// https://github.com/espressif/esp-idf/blob/v5.3.3/examples/storage/sd_card/sdspi
// https://github.com/m5stack/M5PaperS3-UserDemo/blob/main/main/hal/hal.h
#include <driver/spi_master.h>
#include <driver/sdspi_host.h>
#include <driver/sdmmc_host.h>
#include <sdmmc_cmd.h>
#include <esp_vfs_fat.h>

#define MOUNT_POINT "/sdcard"

static sdmmc_card_t* _sd_card = nullptr;

void Hal::sd_card_init()
{
    mclog::tagInfo(_tag, "sd card init");

    if (!_spi_bus_initialized) {
        spi_init();
    }

    // If already mounted successfully, return
    if (_is_sd_card_mounted) {
        mclog::tagInfo(_tag, "sd card already mounted");
        return;
    }

    esp_err_t ret;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    // Options for mounting the filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, .max_files = 5, .allocation_unit_size = 16 * 1024};

    const char mount_point[] = MOUNT_POINT;
    mclog::tagInfo(_tag, "initializing SD card");

    // Initialize SD card slot
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs               = HAL_PIN_SD_CARD_CS;
    slot_config.host_id               = (spi_host_device_t)host.slot;

    mclog::tagInfo(_tag, "mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &_sd_card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            mclog::tagError(_tag, "failed to mount filesystem");
        } else {
            mclog::tagError(_tag, "failed to initialize the card, make sure SD card lines have pull-up resistors");
        }

        // Don't clean up SPI bus on failure - leave it for retry
        mclog::tagInfo(_tag, "sd card init failed, but spi bus remains initialized for retry");
        return;
    }

    mclog::tagInfo(_tag, "filesystem mounted successfully");

    sdmmc_card_print_info(stdout, _sd_card);

    _is_sd_card_mounted = true;
}

Hal::SdCardProbeResult_t Hal::sdCardProbe()
{
    SdCardProbeResult_t result;

    if (!_is_sd_card_mounted) {
        sd_card_init();
        if (!_is_sd_card_mounted) {
            result.is_mounted = false;
            result.size       = "Not Found";
            return result;
        }
    }

    result.is_mounted = true;

    // Try write to sd card
    FILE* fp = fopen(MOUNT_POINT "/test.txt", "w");
    if (fp) {
        fwrite("Hello, World!", 1, 13, fp);
        fclose(fp);

        result.size =
            fmt::format("Size: {:.1f} GB",
                        ((float)((uint64_t)_sd_card->csd.capacity) * _sd_card->csd.sector_size) / (1024 * 1024 * 1024));
    } else {
        result.size = "Write Failed";
    }

    result.type = "Type: ";
    if (_sd_card->is_sdio) {
        result.type += "SDIO";
    } else if (_sd_card->is_mmc) {
        result.type += "MMC";
    } else {
        result.type += (_sd_card->ocr & (1 << 30)) ? "SDHC/SDXC" : "SDSC";
    }

    result.name = fmt::format("Name: {}", std::string(_sd_card->cid.name));

    return result;
}

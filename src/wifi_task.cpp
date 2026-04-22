/**
 * @file wifi_task.cpp
 * @brief Wi-Fi management using FreeRTOS tasks and the Arduino framework.
 * 
 * This library implements a singleton WiFi task responsible for handling
 * connection, reconnection, and shutdown logic. It uses an event group
 * to synchronize the state between the task and Wi-Fi event callbacks.
 */

#include "wifi_task.hpp"

WiFiTask& WiFiTask::get_instance()
{
    static WiFiTask instance;
    return instance;
}

void WiFiTask::wifi_start(const LoginData_t& config)
{
    if (wifi_task_handle == nullptr) {
        // Login data to be passed to the task, copied on the heap to avoid lifetime issues
        static const LoginData_t *s_login_data = new LoginData_t(config);

        // Reset all Wi-Fi state bits before starting
        xEventGroupClearBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT |
            WIFI_RECONNECT_BIT |
            WIFI_STOP_BIT |
            WIFI_FAIL_BIT);

        log_i("[Wi-Fi] Creating Wi-Fi task.");

        BaseType_t wifi_res = xTaskCreatePinnedToCore(
            wifi_task,
            "wifi_task",
            WIFI_TASK_STACK_SIZE,
            (void *) &s_login_data,
            WIFI_TASK_PRIORITY,
            &wifi_task_handle,
            WIFI_TASK_CORE_ID
        );
        if (wifi_res != pdPASS) {
            log_w("[Wi-Fi] Failed to create Wi-Fi task.");

            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
    } else {
        log_w("[Wi-Fi] Wi-Fi has already been started.");
    }
}

void WiFiTask::wifi_stop()
{
    if (wifi_task_handle != nullptr) {
        log_i("[Wi-Fi] Requested to stop Wi-Fi task.");

        xEventGroupSetBits(wifi_event_group, WIFI_STOP_BIT);
    } else {
        log_w("[Wi-Fi] Wi-Fi has not been started yet.");
    }
}

EventGroupHandle_t WiFiTask::get_event_group() const 
{
    return wifi_event_group;
}

WiFiTask::WiFiTask() : 
    wifi_task_handle(nullptr),
    wifi_event_group(xEventGroupCreate()),
    wifi_retry_number(0)
{

}

WiFiTask::~WiFiTask()
{
    wifi_stop();
}

void WiFiTask::wifi_task(void *arg)
{
    log_i("[Wi-Fi] Wi-Fi task successfully created.");

    const LoginData_t *login_data = (const LoginData_t *)arg;

    EventBits_t wifi_event_bits;
    EventGroupHandle_t wifi_event_group = WiFiTask::get_instance().get_event_group();

    // Reset retry counter on task start
    WiFiTask::get_instance().wifi_retry_number = 0;

    // Configure Wi-Fi settings
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.persistent(false);
    WiFi.setHostname(login_data->hostname);

    // Register event handler and initiate connection
    WiFi.onEvent(wifi_event);
    WiFi.begin(login_data->ssid, login_data->password);

    for (;;) {
        // Wait for relevant Wi-Fi events
        wifi_event_bits = xEventGroupWaitBits(
            wifi_event_group,
            WIFI_RECONNECT_BIT | WIFI_STOP_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

        if (wifi_event_bits & WIFI_FAIL_BIT) {
            log_w("[Wi-Fi] Failed to connect to Wi-Fi.");
            break;
        }

        if (wifi_event_bits & WIFI_STOP_BIT) {
            log_i("[Wi-Fi] Disconnecting from Wi-Fi.");

            WiFi.disconnect();
            break;
        }

        if (wifi_event_bits & WIFI_RECONNECT_BIT) {
            xEventGroupClearBits(wifi_event_group, WIFI_RECONNECT_BIT);
            if (WiFiTask::get_instance().wifi_retry_number >= WIFI_MAX_RETRIES) {
                xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);

                log_w("[Wi-Fi] Failed to connect to Wi-Fi (maximum number of retries reached).");
                break;
            } else {
                // Exponential backoff delay
                uint32_t delay_ms = WIFI_INITIAL_RETRY_DELAY_MS * pow(2.0, WiFiTask::get_instance().wifi_retry_number);
                WiFiTask::get_instance().wifi_retry_number++;

                log_i("[Wi-Fi] Waiting %d ms before the next connection attempt.", delay_ms);

                // Wait for delay or interruption (stop/fail bits)
                EventBits_t wifi_drop_bits = xEventGroupWaitBits(
                    wifi_event_group,
                    WIFI_STOP_BIT | WIFI_FAIL_BIT,
                    pdFALSE,
                    pdFALSE,
                    pdMS_TO_TICKS(delay_ms));
                    
                if (!(wifi_drop_bits & WIFI_STOP_BIT) && !(wifi_drop_bits & WIFI_FAIL_BIT)) {
                    log_i("[Wi-Fi] Attempting to connect to Wi-Fi.");

                    WiFi.reconnect();  
                }                              
            }
        }
    }

    log_i("[Wi-Fi] Deleting Wi-Fi task.");

    WiFiTask::get_instance().wifi_task_handle = nullptr;
    vTaskDelete(nullptr);
}

void WiFiTask::wifi_event(WiFiEvent_t event, WiFiEventInfo_t info)
{
    EventGroupHandle_t wifi_event_group = WiFiTask::get_instance().get_event_group();
    EventBits_t wifi_event_bits = xEventGroupGetBits(wifi_event_group);

    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);

            log_i("[Wi-Fi] Wi-Fi disconnected.");

            // Trigger reconnect unless a stop was requested
            if (!(wifi_event_bits & WIFI_STOP_BIT)) {
                switch (info.wifi_sta_disconnected.reason) {
                    case WIFI_REASON_AUTH_FAIL:
                        log_w("[Wi-Fi] Incorrect password.");

                        xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
                        break;
                    default:
                        xEventGroupSetBits(wifi_event_group, WIFI_RECONNECT_BIT);
                        break;
                }
            }
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            // Reset retry counter on successful connection
            WiFiTask::get_instance().wifi_retry_number = 0;

            xEventGroupClearBits(wifi_event_group, WIFI_RECONNECT_BIT);
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);

            log_i("[Wi-Fi] Connected to Wi-Fi with IP: %s.", WiFi.localIP().toString());
            break;
        default:
            break;
    }
}

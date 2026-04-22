#include <Arduino.h>

#include "wifi_task.hpp"

LoginData_t login_data;
EventGroupHandle_t event_group;

void setup()
{
    Serial.begin(115200);

    // Login data needs to be provided through a LoginData_t struct
    login_data = {
        .hostname = "esp32-demo",
        .ssid = "YOUR_SSID",
        .password = "YOUR_PASSWORD"
    };

    // The FreeRTOS event group is used to communicate the Wi-Fi status
    event_group = WiFiTask::get_instance().get_event_group();
}

void loop()
{
    // Start the Wi-Fi task
    WiFiTask::get_instance().wifi_start(login_data);

    // Wait until the connection attempt succeeds or fails
    EventBits_t bits = xEventGroupWaitBits(
        event_group,
        WiFiTask::WIFI_CONNECTED_BIT | WiFiTask::WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    // Check whether the connection succeded or failed
    if (bits & WiFiTask::WIFI_CONNECTED_BIT) {
        Serial.println("Wi-Fi connected.");

        // Stop the task and end the connection
        WiFiTask::get_instance().wifi_stop();
    } else if (bits & WiFiTask::WIFI_FAIL_BIT) {
        Serial.println("Failed to connect.");
    }

    // Wait 30 seconds
    vTaskDelay(pdMS_TO_TICKS(30000));
}

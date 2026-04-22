/**
 * @file wifi_task.hpp
 * @brief Wi-Fi management using FreeRTOS tasks and the Arduino framework.
 *
 * This library implements a singleton WiFi task responsible for handling
 * connection, reconnection, and shutdown logic. It uses an event group
 * to synchronize the state between the task and Wi-Fi event callbacks.
 */

#pragma once

#include <WiFi.h>

/** @brief Stack size for the Wi-Fi task (in bytes). */
#define WIFI_TASK_STACK_SIZE 4096

/** @brief Priority of the Wi-Fi task. */
#define WIFI_TASK_PRIORITY 1

/** @brief CPU core ID where the Wi-Fi task is pinned. */
#define WIFI_TASK_CORE_ID 0

/** @brief Initial delay for exponential backoff (ms). */
#define WIFI_INITIAL_RETRY_DELAY_MS 1000

/** @brief Maximum number of reconnect attempts before failure. */
#define WIFI_MAX_RETRIES 5

/**
 * @brief Wi-Fi login credentials container.
 */
typedef struct
{
    const char *hostname; /**< Device hostname */
    const char *ssid;     /**< Wi-Fi SSID */
    const char *password; /**< Wi-Fi password */
} LoginData_t;

/**
 * @brief Wi-Fi task manager (singleton).
 *
 * This class encapsulates the lifecycle and behavior of a FreeRTOS-based
 * Wi-Fi task. It uses an event group to communicate connection state
 * and control signals between the task and event callbacks.
 */
class WiFiTask
{
public:
    /**
     * @brief Get singleton instance.
     *
     * @return Reference to the static WiFiTask instance.
     */
    static WiFiTask& get_instance();

    // Disable copy and assignment
    WiFiTask(WiFiTask&) = delete;
    WiFiTask(const WiFiTask&) = delete;
    WiFiTask& operator=(WiFiTask&) = delete;
    WiFiTask& operator=(const WiFiTask&) = delete;

    /**
    * @brief Start the Wi-Fi task.
    *
    * Creates the Wi-Fi FreeRTOS task if it is not already running and
    * initializes connection parameters.
    *
    * @param[in] arg Pointer to LoginData_t structure.
    *
    * @note If the task is already running, a warning is logged and no action is taken.
    */
    void wifi_start(const LoginData_t& config);

    /**
     * @brief Request Wi-Fi task shutdown.
     *
     * Signals the Wi-Fi task to stop via the event group.
     */
    void wifi_stop();

    /**
     * @brief Get the Wi-Fi event group handle.
     *
     * @return FreeRTOS event group used for Wi-Fi state signaling.
     */
    EventGroupHandle_t get_event_group() const;

    /** @brief Bit indicating successful connection. */
    static const uint32_t WIFI_CONNECTED_BIT = BIT0;

    /** @brief Bit indicating a reconnect request. */
    static const uint32_t WIFI_RECONNECT_BIT = BIT1;

    /** @brief Bit indicating a stop request. */
    static const uint32_t WIFI_STOP_BIT = BIT2;

    /** @brief Bit indicating a failure condition. */
    static const uint32_t WIFI_FAIL_BIT = BIT3;

private:
    /**
     * @brief Construct a new WiFiTask object.
     *
     * Initializes internal state and creates the event group.
     */
    WiFiTask();

    /**
     * @brief Destroy the WiFiTask object.
     *
     * Ensures the Wi-Fi task is stopped before destruction.
     */
    ~WiFiTask();

    /**
     * @brief FreeRTOS task implementing Wi-Fi connection logic.
     *
     * Handles connection, reconnection with exponential backoff,
     * and graceful shutdown based on event group signals.
     *
     * @param[in] arg Pointer to LoginData_t structure.
     */
    static void wifi_task(void *);

    /**
     * @brief Wi-Fi event callback handler.
     *
     * Processes system Wi-Fi events and updates the event group accordingly.
     *
     * @param[in] event Wi-Fi event ID.
     * @param[in] info  Event-specific information.
     */
    static void wifi_event(WiFiEvent_t, WiFiEventInfo_t);

    TaskHandle_t wifi_task_handle;          /**< Handle of the Wi-Fi task */
    EventGroupHandle_t wifi_event_group;    /**< Event group for Wi-Fi state */
    uint8_t wifi_retry_number;              /**< Current reconnect attempt count */
};

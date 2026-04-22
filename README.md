# WiFiTask

A simple helper library for managing the ESP32 Wi-Fi using FreeRTOS tasks and the Arduino framework.

## Features

- **Task-based Wi-Fi Management**: The Wi-Fi connection process is managed by a FreeRTOS task to ensure non-blocking operations and better multitasking.
- **Automatic Reconnection**: The task will attempt to reconnect to Wi-Fi with an exponential backoff strategy if the connection fails.
- **Graceful Shutdown**: The Wi-Fi task can be stopped gracefully, ensuring clean disconnection and resource management.
- **Event Group Communication**: Uses FreeRTOS event groups to synchronize Wi-Fi status across different parts of the application.

## Installation

### PlatformIO

Download the repository and add it to the `\lib` folder of your project.

## Usage

See the [example](/examples/wifi_basic/src/main.cpp).

## Key Functions

- **wifi_start(const LoginData_t& config)**: Starts the Wi-Fi task and initiates the connection process.
- **wifi_stop()**: Stops the Wi-Fi task and disconnects.
- **get_event_group()**: Returns the event group for synchronizing Wi-Fi status.

## License

BSD-3-Clause License. See the [LICENSE](/LICENSE) file for details.

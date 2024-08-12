Project build upon ESP-IDF 'scan' example and TTN-ESP library 'hello world' example

| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- |

# Wi-Fi Scanner with TTN Integration
This project is a simple Wi-Fi scanner implemented on an ESP32 device. It initializes the Wi-Fi module and scans for available Wi-Fi Access Points (APs) whenever a button connected to the device is pressed. The results of the scan, including the SSID, RSSI, and MAC address of each AP, are printed to the console.

## Features
- **Button-Triggered Wi-Fi Scan**: A button press initiates a Wi-Fi scan, and the scan results are logged to the console.
- **Configurable Button Input**: The GPIO pin used for the button input can be configured based on the ESP32 variant being used.
- **Logging**: The application uses the ESP-IDF logging module to print scan results and button events.

## How to use example
Before project configuration and build, be sure to set the correct chip target using `idf.py set-target <chip_name>`.

### Hardware Required
* A development board with ESP32/ESP32-S2/ESP32-C3 SoC (e.g., ESP32-DevKitC, ESP-WROVER-KIT, etc.).
* A USB cable for Power supply and programming.

### Configure the project
Open the project configuration menu (`idf.py menuconfig`).

In the `Example Configuration` menu:

* Set the Example configuration.
    * Use `Max size of scan list` to set the maximum number of access points in the list.
    * Use `TTN LoRa frequency / region` to set LoRa region

### Build and Flash

Build the project and flash it to the board, then run the monitor tool to view the serial output:

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for all the steps to configure and use the ESP-IDF to build projects.

* [ESP-IDF Getting Started Guide on ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
* [ESP-IDF Getting Started Guide on ESP32-S2](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html)
* [ESP-IDF Getting Started Guide on ESP32-C3](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/get-started/index.html)
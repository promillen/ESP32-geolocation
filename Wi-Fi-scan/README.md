Project build upon ESP-IDF 'scan' example

| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- |

# Wi-Fi Scanner with Button Trigger
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

## Example Output

As you run the example, you will see the following log:

```
I (443) wifi:wifi firmware version: 6bff005
I (443) wifi:wifi certification version: v7.0
I (443) wifi:config NVS flash: enabled
I (443) wifi:config nano formating: disabled
I (453) wifi:Init data frame dynamic rx buffer num: 32
I (453) wifi:Init management frame dynamic rx buffer num: 32
I (463) wifi:Init management short buffer num: 32
I (463) wifi:Init dynamic tx buffer num: 32
I (473) wifi:Init static tx FG buffer num: 2
I (473) wifi:Init static rx buffer size: 1600
I (473) wifi:Init static rx buffer num: 10
I (483) wifi:Init dynamic rx buffer num: 32
I (483) wifi_init: rx ba win: 6
I (493) wifi_init: tcpip mbox: 32
I (493) wifi_init: udp mbox: 6
I (493) wifi_init: tcp mbox: 6
I (503) wifi_init: tcp tx win: 5744
I (503) wifi_init: tcp rx win: 5744
I (513) wifi_init: tcp mss: 1440
I (513) wifi_init: WiFi IRAM OP enabled
I (513) wifi_init: WiFi RX IRAM OP enabled
I (533) phy_init: phy_version 300,6e46ba7,Jan 25 2021
I (683) wifi:set rx active PTI: 0, rx ack PTI: 0, and default PTI: 0
I (683) wifi:mode : sta (7c:df:a1:40:23:84)
I (683) wifi:enable tsf
I (2783) scan: Total APs scanned = 17
I (2783) scan: SSID 		IoTNetwork
I (2783) scan: RSSI 		-50
I (2783) scan: Authmode 	WIFI_AUTH_WPA2_PSK
I (2783) scan: Pairwise Cipher 	WIFI_CIPHER_TYPE_CCMP
I (2793) scan: Group Cipher 	WIFI_CIPHER_TYPE_CCMP
I (2793) scan: Channel 		5

I (2883) scan: SSID 		TP-Link_6872
I (2883) scan: RSSI 		-70
I (2883) scan: Authmode 	WIFI_AUTH_WPA_WPA2_PSK
I (2893) scan: Pairwise Cipher 	WIFI_CIPHER_TYPE_CCMP
I (2893) scan: Group Cipher 	WIFI_CIPHER_TYPE_CCMP
I (2903) scan: Channel 		11
...
```